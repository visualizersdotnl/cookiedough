/*
	BASS internet radio example
	Copyright (c) 2002-2021 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <stdlib.h>
#include <string.h>
#include "bass.h"

// HLS definitions (copied from BASSHLS.H)
#define BASS_SYNC_HLS_SEGMENT	0x10300
#define BASS_TAG_HLS_EXTINF		0x14000

#define UIFILE "netradio.ui"
GtkBuilder *builder;

GtkWidget *win;
DWORD req;	// request number/counter
HSTREAM chan;	// stream handle

const char *urls[10] = { // preset stream URLs
	"http://stream-dc1.radioparadise.com/rp_192m.ogg", "http://www.radioparadise.com/m3u/mp3-32.m3u",
	"http://somafm.com/secretagent.pls", "http://somafm.com/secretagent32.pls",
	"http://somafm.com/suburbsofgoa.pls", "http://somafm.com/suburbsofgoa32.pls",
	"http://bassdrive.com/bassdrive.m3u", "http://bassdrive.com/bassdrive3.m3u",
	"http://sc6.radiocaroline.net:8040/listen.pls", "http://sc2.radiocaroline.net:8010/listen.pls"
};

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

#define GetWidget(id) GTK_WIDGET(gtk_builder_get_object(builder,id))

void WindowDestroy(GtkObject *obj, gpointer data)
{
	gtk_main_quit();
}

void gtk_label_set_text_8859(GtkLabel *label, const gchar *text)
{
	gsize s;
	char *utf = g_convert(text, -1, "UTF-8", "ISO-8859-1", NULL, &s, NULL);
	if (utf) {
		gtk_label_set_text(label, utf);
		g_free(utf);
	} else
		gtk_label_set_text(label, text);
}

// update stream title from metadata
void DoMeta()
{
	GtkLabel *label = GTK_LABEL(GetWidget("status1"));
	const char *meta = BASS_ChannelGetTags(chan, BASS_TAG_META);
	if (meta) { // got Shoutcast metadata
		char *p = strstr(meta, "StreamTitle='");
		if (p) {
			p = strdup(p + 13);
			strchr(p, ';')[-1] = 0;
			gtk_label_set_text_8859(label, p);
			free(p);
		}
	} else {
		meta = BASS_ChannelGetTags(chan, BASS_TAG_OGG);
		if (meta) { // got Icecast/OGG tags
			const char *artist = NULL, *title = NULL, *p = meta;
			for (; *p; p += strlen(p) + 1) {
				if (!strncasecmp(p, "artist=", 7)) // found the artist
					artist = p + 7;
				if (!strncasecmp(p, "title=", 6)) // found the title
					title = p + 6;
			}
			if (title) {
				if (artist) {
					char text[100];
					snprintf(text, sizeof(text), "%s - %s", artist, title);
					gtk_label_set_text(label, text);
				} else
					gtk_label_set_text(label, title);
			}
		} else {
			meta = BASS_ChannelGetTags(chan, BASS_TAG_HLS_EXTINF);
			if (meta) { // got HLS segment info
				const char *p = strchr(meta, ',');
				if (p) gtk_label_set_text(label, p + 1);
			}
		}
	}
}

void CALLBACK MetaSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	gdk_threads_enter();
	DoMeta();
	gdk_threads_leave();
}

gboolean BufferTimerProc(gpointer data)
{
	// monitor buffering progress
	DWORD active = BASS_ChannelIsActive(chan);
	if (active == BASS_ACTIVE_STALLED) {
		char text[32];
		sprintf(text, "buffering... %d%%", 100 - (int)BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFERING));
		gtk_label_set_text(GTK_LABEL(GetWidget("status2")), text);
		return TRUE; // continue monitoring
	}
	if (active) {
		gtk_label_set_text(GTK_LABEL(GetWidget("status2")), "playing");
		{ // get the stream name and URL
			const char *icy = BASS_ChannelGetTags(chan, BASS_TAG_ICY);
			if (!icy) icy = BASS_ChannelGetTags(chan, BASS_TAG_HTTP); // no ICY tags, try HTTP
			if (icy) {
				for (; *icy; icy += strlen(icy) + 1) {
					if (!strncasecmp(icy, "icy-name:", 9))
						gtk_label_set_text_8859(GTK_LABEL(GetWidget("status2")), icy + 9);
					if (!strncasecmp(icy, "icy-url:", 8))
						gtk_label_set_text_8859(GTK_LABEL(GetWidget("status3")), icy + 8);
				}
			}
		}
		// get the stream title
		DoMeta();
	}
	return FALSE; // stop monitoring
}

void CALLBACK StallSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	if (!data) { // stalled
		g_timeout_add(50, BufferTimerProc, NULL); // start buffer monitoring
	}
}

void CALLBACK FreeSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	chan = 0;
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(GetWidget("status1")), "");
	gtk_label_set_text(GTK_LABEL(GetWidget("status2")), "not playing");
	gtk_label_set_text(GTK_LABEL(GetWidget("status3")), "");
	gdk_threads_leave();
}

void CALLBACK StatusProc(const void *buffer, DWORD length, void *user)
{
	if (buffer && !length && (uintptr_t)user == req) { // got HTTP/ICY tags, and this is still the current request
		gdk_threads_enter();
		gtk_label_set_text(GTK_LABEL(GetWidget("status3")), buffer); // display connection status
		gdk_threads_leave();
	}
}

void *OpenURL(void *url)
{
	G_LOCK_DEFINE_STATIC(req);
	DWORD c, r;
	G_LOCK(req); // make sure only 1 thread at a time can do the following
	r = ++req; // increment the request counter for this request
	G_UNLOCK(req);
	if (chan) BASS_StreamFree(chan); // close old stream
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(GetWidget("status1")), "");
	gtk_label_set_text(GTK_LABEL(GetWidget("status2")), "connecting...");
	gtk_label_set_text(GTK_LABEL(GetWidget("status3")), "");
	gdk_threads_leave();
	c = BASS_StreamCreateURL(url, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, StatusProc, (void*)(uintptr_t)r);
	free(url); // free temp URL buffer
	G_LOCK(req);
	if (r != req) { // there is a newer request, discard this stream
		G_UNLOCK(req);
		if (c) BASS_StreamFree(c);
		return NULL;
	}
	chan = c; // this is now the current stream
	G_UNLOCK(req);
	if (!chan) {
		gdk_threads_enter();
		gtk_label_set_text(GTK_LABEL(GetWidget("status2")), "not playing");
		Error("Can't play the stream");
		gdk_threads_leave();
	} else {
		// set syncs for stream title updates
		BASS_ChannelSetSync(chan, BASS_SYNC_META, 0, MetaSync, 0); // Shoutcast
		BASS_ChannelSetSync(chan, BASS_SYNC_OGG_CHANGE, 0, MetaSync, 0); // Icecast/OGG
		BASS_ChannelSetSync(chan, BASS_SYNC_HLS_SEGMENT, 0, MetaSync, 0); // HLS
		// set sync for stalling/buffering
		BASS_ChannelSetSync(chan, BASS_SYNC_STALL, 0, StallSync, 0);
		// set sync for end of stream (when freed due to AUTOFREE)
		BASS_ChannelSetSync(chan, BASS_SYNC_FREE, 0, FreeSync, 0);
		// play it!
		BASS_ChannelPlay(chan, FALSE);
		// start buffer monitoring
		g_timeout_add(50, BufferTimerProc, NULL);
	}
	return NULL;
}

void PresetClicked(GtkButton *obj, gpointer data)
{
	const char *url;
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	if (!strcmp(objname, "customopen")) { // play a custom URL
		url = gtk_entry_get_text(GTK_ENTRY(GetWidget("customurl"))); // get the URL
	} else { // play a preset
		int preset = atoi(objname + 6) - 1; // get preset from button name ("presetX")
		url = urls[preset];
	}
	if (GTK_TOGGLE_BUTTON(GetWidget("proxydirect"))->active)
		BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, NULL); // disable proxy
	else
		BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, gtk_entry_get_text(GTK_ENTRY(GetWidget("proxyurl")))); // set proxy server
	// open URL in a new thread (so that main thread is free)
#if GLIB_CHECK_VERSION(2,32,0)
	{
		GThread *thread = g_thread_new(NULL, OpenURL, strdup(url));
		g_thread_unref(thread);
	}
#else
	g_thread_create(OpenURL, strdup(url), FALSE, NULL);
#endif
}

int main(int argc, char* argv[])
{
#if !GLIB_CHECK_VERSION(2,32,0)
	g_thread_init(NULL);
#endif
	gdk_threads_init();
	gtk_init(&argc, &argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize default output device
	if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing

	BASS_PluginLoad("libbass_aac.so", 0); // load BASS_AAC (if present) for AAC support
	BASS_PluginLoad("libbassflac.so", 0); // load BASSFLAC (if present) for FLAC support
	BASS_PluginLoad("libbasshls.so", 0); // load BASSHLS (if present) for HLS support

	// initialize GUI
	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, UIFILE, NULL)) {
		char path[PATH_MAX];
		readlink("/proc/self/exe", path, sizeof(path));
		strcpy(strrchr(path, '/') + 1, UIFILE);
		if (!gtk_builder_add_from_file(builder, path, NULL)) {
			Error("Can't load UI");
			return 0;
		}
	}
	win = GetWidget("window1");
	gtk_builder_connect_signals(builder, NULL);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	BASS_Free();
	BASS_PluginFree(0);

	return 0;
}

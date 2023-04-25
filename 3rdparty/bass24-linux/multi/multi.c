/*
	BASS multiple output example
	Copyright (c) 2001-2021 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "bass.h"

#define UIFILE "multi.ui"
GtkBuilder *builder;

GtkWidget *win;
GtkWidget *filesel;

DWORD outdev[2] = { 1, 0 };	// output devices
DWORD chan[2];		// channel handles

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

void DeviceChanged(GtkComboBox *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int devn = atoi(objname + 6) - 1; // get device number from button name ("deviceX")
	int sel = gtk_combo_box_get_active(obj); // get the selection
	if (outdev[devn] == sel) return;
	if (!BASS_Init(sel, 44100, 0, NULL, NULL)) { // initialize new device
		Error("Can't initialize device");
		gtk_combo_box_set_active(obj, outdev[devn]);
	} else {
		if (chan[devn]) BASS_ChannelSetDevice(chan[devn], sel); // move channel to new device
		BASS_SetDevice(outdev[devn]); // set context to old device
		BASS_Free(); // free it
		outdev[devn] = sel;
	}
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data, info->filename, 0, NULL, 0);
}

void OpenClicked(GtkButton *obj, gpointer data)
{
	int resp = gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp == GTK_RESPONSE_ACCEPT) {
		char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
		int devn = atoi(objname + 4) - 1; // get device number from button name ("openX")
		BASS_ChannelFree(chan[devn]); // free the old channel
		BASS_SetDevice(outdev[devn]); // set the device to create new channel on
		if (!(chan[devn] = BASS_StreamCreateFile(FALSE, file, 0, 0, BASS_SAMPLE_LOOP))
			&& !(chan[devn] = BASS_MusicLoad(FALSE, file, 0, 0, BASS_MUSIC_RAMPS | BASS_SAMPLE_LOOP, 1))) {
			gtk_button_set_label(obj, "Open file...");
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj, strrchr(file, '/') + 1);
			BASS_ChannelPlay(chan[devn], FALSE);
		}
		g_free(file);
	}
}

// Cloning DSP function
void CALLBACK CloneDSP(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	BASS_StreamPutData((uintptr_t)user, buffer, length); // user = clone
}

void CloneClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int devn = atoi(objname + 5) - 1; // get device number from button name ("cloneX")
	BASS_CHANNELINFO chaninfo;
	if (!BASS_ChannelGetInfo(chan[devn ^ 1], &chaninfo)) {
		Error("Nothing to clone");
	} else {
		BASS_ChannelFree(chan[devn]); // free the old channel
		BASS_SetDevice(outdev[devn]); // set the device to create clone on
		if (!(chan[devn] = BASS_StreamCreate(chaninfo.freq, chaninfo.chans, chaninfo.flags, STREAMPROC_PUSH, 0))) { // create a "push" stream
			char oname[10];
			sprintf(oname, "open%d", devn + 1);
			gtk_button_set_label(GTK_BUTTON(GetWidget(oname)), "Open file...");
			Error("Can't create clone");
		} else {
			BASS_INFO info;
			BASS_GetInfo(&info); // get latency info
			BASS_ChannelLock(chan[devn ^ 1], TRUE); // lock source stream to synchonise buffer contents
			BASS_ChannelSetDSP(chan[devn ^ 1], CloneDSP, (void*)(uintptr_t)chan[devn], 0); // set DSP to feed data to clone
			{ // copy buffered data to clone
				DWORD d = BASS_ChannelSeconds2Bytes(chan[devn], info.latency / 1000.f); // playback delay
				DWORD c = BASS_ChannelGetData(chan[devn ^ 1], 0, BASS_DATA_AVAILABLE);
				BYTE *buf = (BYTE*)malloc(c);
				c = BASS_ChannelGetData(chan[devn ^ 1], buf, c);
				if (c > d) BASS_StreamPutData(chan[devn], buf + d, c - d);
				free(buf);
			}
			BASS_ChannelLock(chan[devn ^ 1], FALSE); // unlock source stream
			BASS_ChannelPlay(chan[devn], FALSE); // play clone
			{
				char oname[10];
				sprintf(oname, "open%d", devn + 1);
				gtk_button_set_label(GTK_BUTTON(GetWidget(oname)), "clone");
			}
		}
	}
}

void SwapClicked(GtkButton *obj, gpointer data)
{
	{ // swap handles
		HSTREAM temp = chan[0];
		chan[0] = chan[1];
		chan[1] = temp;
	}
	{ // swap text
		GtkButton *open1 = GTK_BUTTON(GetWidget("open1")), *open2 = GTK_BUTTON(GetWidget("open2"));
		char *temp = strdup(gtk_button_get_label(open1));
		gtk_button_set_label(open1, gtk_button_get_label(open2));
		gtk_button_set_label(open2, temp);
		free(temp);
	}
	// update the channel devices
	BASS_ChannelSetDevice(chan[0], outdev[0]);
	BASS_ChannelSetDevice(chan[1], outdev[1]);
}

int main(int argc, char* argv[])
{
	regex_t fregex;

	gtk_init(&argc, &argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

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

	{ // get list of output devices
		int c;
		BASS_DEVICEINFO di;
		for (c = 0; BASS_GetDeviceInfo(c, &di); c++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(GetWidget("device1")), di.name);
			if (c == outdev[0]) gtk_combo_box_set_active(GTK_COMBO_BOX(GetWidget("device1")), c);
			gtk_combo_box_append_text(GTK_COMBO_BOX(GetWidget("device2")), di.name);
			if (c == outdev[1]) gtk_combo_box_set_active(GTK_COMBO_BOX(GetWidget("device2")), c);
		}
	}
	// initialize the output devices
	if (!BASS_Init(outdev[0], 44100, 0, NULL, NULL) || !BASS_Init(outdev[1], 44100, 0, NULL, NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "Playable files");
		regcomp(&fregex, "\\.(mo3|xm|mod|s3m|it|umx|mp[1-3]|ogg|wav|aif)$", REG_ICASE | REG_NOSUB | REG_EXTENDED);
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, FileExtensionFilter, &fregex, NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
	}

	gtk_widget_show(win);
	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	// release both devices
	BASS_SetDevice(outdev[0]);
	BASS_Free();
	BASS_SetDevice(outdev[1]);
	BASS_Free();

	return 0;
}

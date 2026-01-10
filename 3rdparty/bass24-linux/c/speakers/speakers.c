/*
	BASS multi-speaker example
	Copyright (c) 2003-2024 Un4seen Developments Ltd.
*/

#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_3_0
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "bass.h"

#define UIFILE "speakers.ui"
GtkBuilder *builder;

GtkWidget *win;
GtkWidget *filesel;

DWORD chan[4];	// channel handles

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

#define GetWidget(id) GTK_WIDGET(gtk_builder_get_object(builder,id))

void WindowDestroy(GtkWidget *obj, gpointer data)
{
	gtk_main_quit();
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
		int speaker = atoi(objname + 4) - 1; // get speaker pair number from button name ("openX")
		BASS_ChannelFree(chan[speaker]); // free the old channel
		if (!(chan[speaker] = BASS_StreamCreateFile(0, file, 0, 0, BASS_SPEAKER_N(speaker + 1) | BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT))
			&& !(chan[speaker] = BASS_MusicLoad(0, file, 0, 0, BASS_SPEAKER_N(speaker + 1) | BASS_MUSIC_RAMPS | BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT, 1))) {
			gtk_button_set_label(obj, "Open file...");
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj, strrchr(file, '/') + 1);
			BASS_ChannelPlay(chan[speaker], FALSE);
		}
		g_free(file);
	}
}

void SwapClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int speaker = atoi(objname + 4) - 1; // get speaker pair number from button name ("swapX")
	{ // swap handles
		HSTREAM temp = chan[speaker];
		chan[speaker] = chan[speaker + 1];
		chan[speaker + 1] = temp;
	}
	{ // swap text
		GtkButton *open1, *open2;
		char bname[10], *temp;
		sprintf(bname, "open%d", 1 + speaker);
		open1 = GTK_BUTTON(GetWidget(bname));
		sprintf(bname, "open%d", 1 + speaker + 1);
		open2 = GTK_BUTTON(GetWidget(bname));
		temp = strdup(gtk_button_get_label(open1));
		gtk_button_set_label(open1, gtk_button_get_label(open2));
		gtk_button_set_label(open2, temp);
		free(temp);
	}
	// update speaker flags
	BASS_ChannelFlags(chan[speaker], BASS_SPEAKER_N(speaker + 1), BASS_SPEAKER_FRONT);
	BASS_ChannelFlags(chan[speaker + 1], BASS_SPEAKER_N(speaker + 2), BASS_SPEAKER_FRONT);
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

	// initialize default device
	if (!BASS_Init(-1, 44100, BASS_DEVICE_SPEAKERS, NULL, NULL)) {
		Error("Can't initialize device");
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

	{ // check how many speakers the device supports
		BASS_INFO i;
		BASS_GetInfo(&i);
		if (i.speakers < 8) {
			gtk_widget_set_sensitive(GetWidget("open4"), FALSE);
			gtk_widget_set_sensitive(GetWidget("swap3"), FALSE);
		}
		if (i.speakers < 6) {
			gtk_widget_set_sensitive(GetWidget("open3"), FALSE);
			gtk_widget_set_sensitive(GetWidget("swap2"), FALSE);
		}
		if (i.speakers < 4) {
			gtk_widget_set_sensitive(GetWidget("open2"), FALSE);
			gtk_widget_set_sensitive(GetWidget("swap1"), FALSE);
		}
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

	BASS_Free();

	return 0;
}

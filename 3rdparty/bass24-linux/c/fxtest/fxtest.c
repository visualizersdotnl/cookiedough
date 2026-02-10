/*
	BASS effects example
	Copyright (c) 2001-2024 Un4seen Developments Ltd.
*/

#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_3_0
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "bass.h"

#define UIFILE "fxtest.ui"
GtkBuilder *builder;

GtkWidget *win;
GtkWidget *filesel;

DWORD chan;		// channel handle
DWORD fxchan;		// output stream handle
HFX fx[4];		// 3 eq bands + reverb

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

void UpdateFX(const char *id)
{
	GtkRange *obj = GTK_RANGE(GetWidget(id));
	double v = gtk_range_get_value(obj);
	int b = atoi(id + 2) - 1;
	if (b < 3) { // EQ
		BASS_DX8_PARAMEQ p;
		BASS_FXGetParameters(fx[b], &p);
		p.fGain = v;
		BASS_FXSetParameters(fx[b], &p);
	} else if (b == 3) { // reverb
		BASS_DX8_REVERB p;
		BASS_FXGetParameters(fx[3], &p);
		p.fReverbMix = (v ? log(v) * 20 : -96);
		BASS_FXSetParameters(fx[3], &p);
	} else // volume
		BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, v);
}

void SetupFX()
{
	// setup the effects
	BASS_DX8_PARAMEQ p;
	DWORD ch = fxchan ? fxchan : chan; // set on output stream if enabled, else file stream
	fx[0] = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0);
	fx[1] = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0);
	fx[2] = BASS_ChannelSetFX(ch, BASS_FX_DX8_PARAMEQ, 0);
	fx[3] = BASS_ChannelSetFX(ch, BASS_FX_DX8_REVERB, 0);
	p.fGain = 0;
	p.fBandwidth = 18;
	p.fCenter = 125;
	BASS_FXSetParameters(fx[0], &p);
	p.fCenter = 1000;
	BASS_FXSetParameters(fx[1], &p);
	p.fCenter = 8000;
	BASS_FXSetParameters(fx[2], &p);
	UpdateFX("fx1");
	UpdateFX("fx2");
	UpdateFX("fx3");
	UpdateFX("fx4");
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
		BASS_ChannelFree(chan); // free the old channel
		if (!(chan = BASS_StreamCreateFile(0, file, 0, 0, BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT))
			&& !(chan = BASS_MusicLoad(0, file, 0, 0, BASS_MUSIC_RAMPS | BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT, 1))) {
			gtk_button_set_label(obj, "Open file...");
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj, strrchr(file, '/') + 1);
			if (!fxchan) SetupFX(); // set effects on file if not using output stream
			UpdateFX("fx5"); // set volume
			BASS_ChannelPlay(chan, FALSE);
		}
		g_free(file);
	}
}

void FXChanged(GtkRange *range, gpointer data)
{
	UpdateFX(gtk_buildable_get_name(GTK_BUILDABLE(range)));
}

void OutputToggled(GtkToggleButton *obj, gpointer data)
{
	// remove current effects
	BASS_FXFree(fx[0]);
	BASS_FXFree(fx[1]);
	BASS_FXFree(fx[2]);
	BASS_FXFree(fx[3]);
	if (gtk_toggle_button_get_active(obj))
		fxchan = BASS_StreamCreate(0, 0, 0, STREAMPROC_DEVICE, 0); // get device output stream
	else
		fxchan = 0; // stop using device output stream
	SetupFX();
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
	if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {
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
	gtk_scale_add_mark(GTK_SCALE(GetWidget("fx1")), 0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(GetWidget("fx2")), 0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(GetWidget("fx3")), 0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(GetWidget("fx5")), 1, GTK_POS_RIGHT, NULL);

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

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	BASS_Free();

	return 0;
}

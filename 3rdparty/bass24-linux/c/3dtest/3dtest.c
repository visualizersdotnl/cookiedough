/*
	BASS 3D test
	Copyright (c) 1999-2024 Un4seen Developments Ltd.
*/

#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_3_0
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <regex.h>
#include "bass.h"

#define UIFILE "3dtest.ui"
GtkBuilder *builder;

GtkWidget *win;
GtkWidget *filesel;

// channel (sample/music) info structure
typedef struct {
	DWORD channel;			// channel handle
	BASS_3DVECTOR pos, vel;	// position, velocity
} Channel;

Channel *chans;		// the channels
int chanc;			// number of channels
int chan = -1;		// selected channel

#define TIMERPERIOD	50		// timer period (ms)
#define MAXDIST		50		// maximum distance of the channels (m)

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

// Update the button states
void UpdateButtons()
{
	gtk_widget_set_sensitive(GetWidget("remove"), chan == -1 ? FALSE : TRUE);
	gtk_widget_set_sensitive(GetWidget("play"), chan == -1 ? FALSE : TRUE);
	gtk_widget_set_sensitive(GetWidget("stop"), chan == -1 ? FALSE : TRUE);
	gtk_widget_set_sensitive(GetWidget("movex"), chan == -1 ? FALSE : TRUE);
	gtk_widget_set_sensitive(GetWidget("movez"), chan == -1 ? FALSE : TRUE);
	gtk_widget_set_sensitive(GetWidget("movereset"), chan == -1 ? FALSE : TRUE);
	if (chan != -1) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(GetWidget("movex")), fabs(chans[chan].vel.x));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(GetWidget("movez")), fabs(chans[chan].vel.z));
	}
}

gboolean ListSelectionChange(GtkTreeSelection *treesel, gpointer data)
{ // the selected channel has (probably) changed
	GtkTreeModel *tm;
	GtkTreeIter it;
	if (gtk_tree_selection_get_selected(treesel, &tm, &it)) {
		char *rows = gtk_tree_model_get_string_from_iter(tm, &it);
		chan = atoi(rows);
		g_free(rows);
	} else
		chan = -1;
	UpdateButtons();
	return TRUE;
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data, info->filename, 0, NULL, 0);
}

void AddClicked(GtkButton *obj, gpointer data)
{ // add a channel
	int resp = gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp == GTK_RESPONSE_ACCEPT) {
		char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		DWORD newchan;
		if ((newchan = BASS_MusicLoad(0, file, 0, 0, BASS_MUSIC_RAMP | BASS_SAMPLE_LOOP | BASS_SAMPLE_3D, 1))
			|| (newchan = BASS_SampleLoad(0, file, 0, 0, 1, BASS_SAMPLE_LOOP | BASS_SAMPLE_3D | BASS_SAMPLE_MONO))) {
			Channel *c;
			chanc++;
			chans = (Channel*)realloc((void*)chans, chanc * sizeof(Channel));
			c = chans + chanc - 1;
			memset(c, 0, sizeof(Channel));
			c->channel = newchan;
			BASS_SampleGetChannel(newchan, 0); // initialize sample channel (ignored if MOD music)
			{ // add it to the list
				GtkListStore *tm = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(GetWidget("channels"))));
				GtkTreeIter it;
				gtk_list_store_append(tm, &it);
				gtk_list_store_set(tm, &it, 0, strrchr(file, '/') + 1, -1);
			}
		} else
			Error("Can't load the file");
		g_free(file);
	}
}

void RemoveClicked(GtkButton *obj, gpointer data)
{ // remove a channel
	GtkTreeModel *tm = gtk_tree_view_get_model(GTK_TREE_VIEW(GetWidget("channels")));
	GtkTreeIter it;
	if (gtk_tree_model_iter_nth_child(tm, &it, NULL, chan)) {
		Channel *c = chans + chan;
		// free both MOD and stream, it must be one of them! :)
		BASS_SampleFree(c->channel);
		BASS_MusicFree(c->channel);
		chanc--;
		memmove(c, c + 1, (chanc - chan) * sizeof(Channel));
		gtk_list_store_remove(GTK_LIST_STORE(tm), &it);
	}
}

void PlayClicked(GtkButton *obj, gpointer data)
{
	BASS_ChannelPlay(chans[chan].channel, FALSE);
}

void StopClicked(GtkButton *obj, gpointer data)
{
	BASS_ChannelPause(chans[chan].channel);
}

void MoveXChanged(GtkSpinButton *spinbutton, gpointer data)
{ // X velocity
	float value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(GetWidget("movex")));
	if (fabs(chans[chan].vel.x) != value) chans[chan].vel.x = value;
}

void MoveZChanged(GtkSpinButton *spinbutton, gpointer data)
{ // Y velocity
	float value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(GetWidget("movez")));
	if (fabs(chans[chan].vel.z) != value) chans[chan].vel.z = value;
}

void MoveResetClicked(GtkButton *obj, gpointer data)
{ // reset the position and velocity to 0
	memset(&chans[chan].pos, 0, sizeof(chans[chan].pos));
	memset(&chans[chan].vel, 0, sizeof(chans[chan].vel));
	UpdateButtons();
}

void RolloffChanged(GtkRange *range, gpointer data)
{
	// change the rolloff factor
	double value = gtk_range_get_value(range);
	BASS_Set3DFactors(-1, pow(2, (value - 10) / 5.0), -1);
}

void DopplerChanged(GtkRange *range, gpointer data)
{
	// change the doppler factor
	double value = gtk_range_get_value(range);
	BASS_Set3DFactors(-1, -1, pow(2, (value - 10) / 5.0));
}

gboolean DrawDisplayArea(GtkWidget *obj, cairo_t *cr, gpointer data)
{
	int c, x, y, w, h, cx, cy;

	w = gtk_widget_get_allocated_width(obj);
	h = gtk_widget_get_allocated_height(obj);
	cx = w / 2;
	cy = h / 2;

	cairo_save(cr);

	// clear the display
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);

	// Draw the listener
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_arc(cr, cx, cy, 4, 0, 2 * M_PI);
	cairo_fill(cr);

	for (c = 0; c < chanc; c++) {
		// If the channel's playing then update it's position
		if (BASS_ChannelIsActive(chans[c].channel) == BASS_ACTIVE_PLAYING) {
			// Check if channel has reached the max distance
			if (chans[c].pos.z >= MAXDIST || chans[c].pos.z <= -MAXDIST)
				chans[c].vel.z = -chans[c].vel.z;
			if (chans[c].pos.x >= MAXDIST || chans[c].pos.x <= -MAXDIST)
				chans[c].vel.x = -chans[c].vel.x;
			// Update channel position
			chans[c].pos.z += chans[c].vel.z * TIMERPERIOD / 1000;
			chans[c].pos.x += chans[c].vel.x * TIMERPERIOD / 1000;
			BASS_ChannelSet3DPosition(chans[c].channel, &chans[c].pos, NULL, &chans[c].vel);
		}
		// Draw the channel position indicator
		x = cx + (int)((cx - 10) * chans[c].pos.x / MAXDIST);
		y = cy - (int)((cy - 10) * chans[c].pos.z / MAXDIST);
		if (chan == c)
			cairo_set_source_rgb(cr, 1, 0, 0);
		else
			cairo_set_source_rgb(cr, 1, 0.75, 0.75);
		cairo_arc(cr, x, y, 4, 0, 2 * M_PI);
		cairo_fill(cr);
	}
	// Apply the 3D changes
	BASS_Apply3D();

	cairo_restore(cr);
	return FALSE;
}

gboolean TimerProc(gpointer data)
{
	gtk_widget_queue_draw(GetWidget("displayarea"));
	return TRUE;
}

int main(int argc, char* argv[])
{
	regex_t fregex[2];

	gtk_init(&argc, &argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// Initialize the output device with 3D support
	if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	{
		BASS_INFO i;
		BASS_GetInfo(&i);
		if (i.speakers > 2) {
			GtkWidget *dialog = gtk_message_dialog_new(NULL, 0,
				GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Multiple speakers were detected. Would you like to use them?");
			if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_NO)
				BASS_SetConfig(BASS_CONFIG_3DALGORITHM, BASS_3DALG_OFF);
			gtk_widget_destroy(dialog);
		}
	}

	// Use meters as distance unit, real world rolloff, real doppler effect
	BASS_Set3DFactors(1, 1, 1);

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
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(GetWidget("channels"))), "changed", G_CALLBACK(ListSelectionChange), NULL);

	{ // setup list
		GtkTreeView *list = GTK_TREE_VIEW(GetWidget("channels"));
		gtk_tree_view_append_column(list, gtk_tree_view_column_new_with_attributes("", gtk_cell_renderer_text_new(), "text", 0, NULL));
		GtkListStore *liststore = gtk_list_store_new(1, G_TYPE_STRING);
		gtk_tree_view_set_model(list, GTK_TREE_MODEL(liststore));
		g_object_unref(liststore);
	}

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "Streamable files (wav/aif/mp3/mp2/mp1/ogg)");
		regcomp(&fregex[0], "\\.(mp[1-3]|ogg|wav|aif)$", REG_ICASE | REG_NOSUB | REG_EXTENDED);
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, FileExtensionFilter, &fregex[0], NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "MOD music files (mo3/xm/mod/s3m/it/mtm/umx)");
		regcomp(&fregex[1], "\\.(mo3|xm|mod|s3m|it|umx)$", REG_ICASE | REG_NOSUB | REG_EXTENDED);
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, FileExtensionFilter, &fregex[1], NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
	}

	g_timeout_add(TIMERPERIOD, TimerProc, NULL);

	UpdateButtons();

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex[0]);
	regfree(&fregex[1]);

	BASS_Free();

	return 0;
}

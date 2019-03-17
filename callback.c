#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "callback.h"
#include "interface.h"
#include "support.h"
#include "v1720.h"
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <TTree.h>

#include <string>

using namespace std;

/* Soft Trigger */
void on_button6_clicked(GtkButton *button, gpointer user_data) {
	V1720_board *bd = (V1720_board *) user_data;
	g_print("Software Trigger \n");
	v1720GenerateSoftwareTrigger(bd->handle);
	//v1720AutoSetDCOffset(bd->handle);
}

/*START BUTTON*/
void on_button3_clicked(GtkButton *button, gpointer user_data) {
	/*Disable START again*/
	gtk_widget_set_sensitive((GtkWidget*) button, 0);
	GtkWidget *stop_button;

	/*Disable CONFIGURE */
	stop_button = lookup_widget((GtkWidget*) button, "button2");
	gtk_widget_set_sensitive(stop_button, 0);

	/*Enable STOP */

	stop_button = lookup_widget((GtkWidget*) button, "button4");
	gtk_widget_set_sensitive(stop_button, 1);

	/*DISABLE Quit */
	stop_button = lookup_widget((GtkWidget*) button, "button5");
	gtk_widget_set_sensitive(stop_button, 0);

	/*Enable SOFT TRIGGER */
	stop_button = lookup_widget((GtkWidget*) button, "button6");
	gtk_widget_set_sensitive(stop_button, 1);

	g_print("START ACQUISITION \n");
	V1720_board *bd;
	bd = (V1720_board *) user_data;
	v1720StartStopAcquisition(bd->handle, 1);
	bd->start_stop = 1;
}

/*Stop BUTTON*/
void on_button4_clicked(GtkButton *button, gpointer user_data) {

	/*Disable STOP again*/
	gtk_widget_set_sensitive((GtkWidget*) button, 0);
	/*Enable START */
	GtkWidget *start_button;
	start_button = lookup_widget((GtkWidget*) button, "button3");
	gtk_widget_set_sensitive(start_button, 1);

	/*Enable CONFIGURE */
	start_button = lookup_widget((GtkWidget*) button, "button2");
	gtk_widget_set_sensitive(start_button, 1);

	/*ENABLE Quit */
	start_button = lookup_widget((GtkWidget*) button, "button5");
	gtk_widget_set_sensitive(start_button, 1);

	/*Disable Soft Trigger */
	start_button = lookup_widget((GtkWidget*) button, "button6");
	gtk_widget_set_sensitive(start_button, 1);

	g_print("STOP ACQUISITION \n");
	V1720_board *bd;
	bd = (V1720_board *) user_data;
	v1720StartStopAcquisition(bd->handle, 0);
	v1720Clear(bd->handle);
	bd->start_stop = 0;
}

/*QUIT BUTTON. DATA ARE SAVED HERE */
/*BEFORE QUITTING, MAKE SURE THAT *write_buf has read all the data */
void on_button5_clicked(GtkButton *button, gpointer user_data)

{
	ofstream *file_txt1;
	TFile *file1;
	TTree *tree1;
	uint32_t **write_buf;
	uint32_t **read_buf;
	button5_data *data;
	data = (button5_data *) user_data;
	file1 = data->file;
	tree1 = data->tree;
	file_txt1 = data->file_txt;
	write_buf = data->write_buf;
	read_buf = data->read_buf;
	printf("Now wait until all data is copied.\n");
	sleep(10);

	tree1->Write();
	file1->Write();
	file_txt1->close();

	printf("Saving and quit now \n");
	gtk_main_quit();
	exit(1);
}

/*CONFIGURE BUTTON */
void on_button2_clicked(GtkButton *button, gpointer data) {
	GtkWidget *window2;
	GtkWidget *window1;
	button2_data *my_data;
	my_data = (button2_data *) data;
	window1 = my_data->window1T;
	window2 = my_data->window2T;

	gtk_widget_hide((GtkWidget*) window1);
	gtk_window_present((GtkWindow*) window2);

}

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
	g_print("delete event occurred\n");
	return FALSE;
	gtk_main_quit();
}

/*********************************************/
/*********************************************/
/********#DEBUG_LEVEL = -DDEBUG*************************************/
/***********CONFIGURE WINDOW******************/
/*********************************************/
/*********************************************/

void configure_enable_callback(GtkWidget *window, gpointer data) {

	window2_data *my_data;
	my_data = (window2_data *) data;
	V1720_board *bd;
	bd = my_data->board;
	GtkWidget *window1;
	window1 = my_data->window1T;

	GtkWidget *widget;
	int ii;
	char name[100];
	double val, val1, val2;
	double DACoffset[NCH];
	int coinc_level;
	int delay;

	/*Acquisition Mode */
	bd->run_mode = 0;
	widget = lookup_widget(window, "radiobutton3");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("RegisterControlMode \n");
		bd->run_mode = 0;
	}

	widget = lookup_widget(window, "radiobutton4");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("S-IN ControlMode \n");
		bd->run_mode = 1;
	}

	widget = lookup_widget(window, "radiobutton5");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("S-IN  GATE Mode \n");
		bd->run_mode = 2;
	}
	widget = lookup_widget(window, "radiobutton6");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("MultyBoard sync \n");
		bd->run_mode = 3;
	}

	/*Samples per channel */
	widget = lookup_widget(window, "hscale18");
	bd->buff_org = 0x0A; /*default*/
	val = gtk_range_get_value(GTK_RANGE(widget));
	int samples_per_channel;
	samples_per_channel = (int) val;

	if ((samples_per_channel % 2) == 1) samples_per_channel++;
	if ((samples_per_channel % 4) == 2) samples_per_channel++;

	printf("Number of samples per events per channel %i \n", samples_per_channel);
	samples_per_channel = samples_per_channel / 4; /*UNTIL explained, one must divide per 4 number of samples per ch and not per 2 as reported in the manual*/
#ifdef DEBUG
	printf("Number of memory location per events per channel %i \n",samples_per_channel);
#endif
	bd->custom_size = samples_per_channel;

	/*BLT EVENT NUMBER*/
	widget = lookup_widget(window, "hscale29");
	val = gtk_range_get_value(GTK_RANGE(widget));
	int blt_evnt_number;
	blt_evnt_number = (int) val;
	bd->blt_event_number = blt_evnt_number;

	/*BERR e ALIGN64 */
	widget = lookup_widget(window, "checkbutton55");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("BERR enabled\n");
		bd->BERR_enable = 1;

	} else {
		printf("BERR disabled\n");
		bd->BERR_enable = 0;
	}
	widget = lookup_widget(window, "checkbutton56");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("ALIGN64 enabled\n");
		bd->ALIGN64_enable = 1;

	} else {
		printf("ALIGN64 disabled\n");
		bd->ALIGN64_enable = 0;
	}

	/*Interrupt Control*/
	widget = lookup_widget(window, "checkbutton54");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("INTERRUPT on Optical Link enabled\n");
		bd->optical_interrupt_enable = 1;

	} else {
		printf("INTERRUPT on Opctical Link disabled\n");
		bd->optical_interrupt_enable = 0;
	}

	widget = lookup_widget(window, "radiobutton7");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("INTERRUPT RORA\n");
		bd->RORA_ROAK = 0;

	} else {
		printf("INTERRUPT ROAK\n");
		bd->RORA_ROAK = 1;
	}

	widget = lookup_widget(window, "hscale30");
	val = gtk_range_get_value(GTK_RANGE(widget));
	int int_event_number;
	int_event_number = (int) val;
	bd->interrupt_event_number = int_event_number;

	widget = lookup_widget(window, "hscale32");
	val = gtk_range_get_value(GTK_RANGE(widget));
	int_event_number = (int) val;
	bd->interrupt_level = int_event_number;

	/*TRIGGER CONTROL */

	/*COINCIDENCE LEVEL*/
	widget = lookup_widget(window, "hscale33");
	val = gtk_range_get_value(GTK_RANGE(widget));
	coinc_level = (int) val;
	printf("Coincidence level is %i \n", coinc_level);
	bd->coinc_level = coinc_level;

	widget = lookup_widget(window, "radiobutton1");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Count all triggers \n");
		bd->trig_count = 1;
	} else {
		printf("Count accepted triggers \n");
		bd->trig_count = 0;
	}

	widget = lookup_widget(window, "radiobutton9");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Internal trigger negative polarity \n");
		bd->trig_polarity = 1;
	} else {
		printf("Internal trigger positive polarity \n");
		bd->trig_polarity = 0;
	}

	widget = lookup_widget(window, "checkbutton57");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Ext trigger enabled \n");
		bd->ext_trig_enable = 1;
	} else {
		printf("Ext trigger disabled \n");
		bd->ext_trig_enable = 0;
	}

	widget = lookup_widget(window, "checkbutton58");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Software trigger enabled \n");
		bd->softw_trig_enable = 1;
	} else {
		printf("Software trigger disabled \n");
		bd->softw_trig_enable = 0;
	}

	widget = lookup_widget(window, "checkbutton59");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Ext trigger out enabled \n");
		bd->ext_trig_enable_out = 1;
	} else {
		printf("Ext trigger out disabled \n");
		bd->ext_trig_enable_out = 0;
	}

	widget = lookup_widget(window, "checkbutton60");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("Software trigger out enabled \n");
		bd->softw_trig_enable_out = 1;
	} else {
		printf("Software trigger out disabled \n");
		bd->softw_trig_enable_out = 0;
	}

	widget = lookup_widget(window, "hscale31");
	val = gtk_range_get_value(GTK_RANGE(widget));
	val = val / 4;
	int post_trig;
	post_trig = (int) val;
	printf("Post trigger setting (sample/4): %i \n", post_trig);
	bd->post_trigger_setting = post_trig;

	/*Input polarity*/
	widget = lookup_widget(window, "radiobutton11");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		printf("INPUT POLARITY IS NIM \n");
		bd->front_panel_nim_ttl = 0;

	} else {
		printf("INPUT POLARITY IS TTL \n");
		bd->front_panel_nim_ttl = 1;
	}

	/*single ch configuration */
	for (ii = 0; ii < NCH; ii++) {
		sprintf(name, "checkbutton%i", ii);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			printf("Chanell %i enabled\n", ii);
			bd->ch[ii].enabled = 1;

		} else {
			printf("Channel %i disabled\n", ii);
			bd->ch[ii].enabled = 0;
		}
		sprintf(name, "checkbutton%i", ii + 10);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			printf("Chanell %i trigger enabled\n", ii);
			bd->ch[ii].trig_enabled = 1;

		} else {
			printf("Channel %i trigger disabled\n", ii);
			bd->ch[ii].trig_enabled = 0;
		}

		sprintf(name, "checkbutton%i", ii + 20);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			printf("Chanell %i propagate trigger enabled\n", ii);
			bd->ch[ii].trig_out_enabled = 1;

		} else {
			printf("Channel %i propagate trigger disabled\n", ii);
			bd->ch[ii].trig_out_enabled = 0;
		}

		sprintf(name, "hscale%i", ii + 10);
		widget = lookup_widget(window, name);
		DACoffset[ii] = gtk_range_get_value(GTK_RANGE(widget));

		val = -(DACoffset[ii]) * (pow(2., 16.) - 1) / 2000;
		int dacdac;
		dacdac = (int) val;

		printf("Channel %i, offset (mV): %f ----> %i , %x \n", ii, DACoffset[ii], dacdac, dacdac);

		bd->ch[ii].dac = dacdac;

		sprintf(name, "hscale%i", ii + 0);
		widget = lookup_widget(window, name);
		val1 = gtk_range_get_value(GTK_RANGE(widget));

		dacdac = (int) val1;

		printf("Channel %i, ZLE thr: %i (0x%x) \n", ii, val1, dacdac);

		bd->ch[ii].zle_thr = dacdac;

		//trig n over thr. Now is common to all channels
		widget = lookup_widget(window, "hscale35");
		val = gtk_range_get_value(GTK_RANGE(widget));
		int n_samples_thr = (int) val;
		printf("Number n of samples over thr is %i \n", n_samples_thr * 4);
		bd->ch[ii].trig_n_over_thr = n_samples_thr;

	}

	/*abilito start. Disabilito anche il quit */

	widget = lookup_widget(window1, "button3");
	gtk_widget_set_sensitive(widget, 1);

	/*punto importante */

	v1720Init(bd);
	/* Now Auto DC Offset */
	for (int ii = 0; ii < NCH; ii++) {
		int flag = 0;
		int ret;
		sprintf(name, "checkbutton%i", ii + 61);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			flag = 1;
		}
		printf("flag is %i\n", flag);
		if ((bd->ch[ii].enabled == 1) && (flag)) {
			printf("Auto set ch %i \n", ii);
			ret = v1720AutoSetDCOffset(bd->handle, ii, -DACoffset[ii], &bd->ch[ii].baseline);
			bd->ch[ii].dac = ret;

			sprintf(name, "hscale%i", ii + 10);
			widget = lookup_widget(window, name);
			DACoffset[ii] = (int) (-ret * 2000 / (pow(2., 16.) - 1));

			//val=-(DACoffset[ii])*(pow(2.,16.)-1)/2000;
			gtk_range_set_value(GTK_RANGE(widget), DACoffset[ii]);

		}
	}

	/*Now get MEAN*/
	for (int ii = 0; ii < NCH; ii++) {
		int ret;
		if (bd->ch[ii].enabled == 1) {
			ret = v1720GetMean(bd->handle, ii);
			bd->ch[ii].dac = ret;
		}
	}

	/*end punto importante*/
	widget = lookup_widget(window, "window2");
	gtk_widget_hide((GtkWidget*) widget);
	gtk_window_present(GTK_WINDOW(window1));

}

void configure_save_callback(GtkWidget *window, gpointer data) {

	window2_data *my_data;
	my_data = (window2_data *) data;
	GtkWidget *window1;
	window1 = my_data->window1T;

	GtkWidget *widget;
	int ii;
	char name[100];
	double val, val1, val2;
	double DACoffset[NCH];
	int coinc_level;
	int delay;

	ofstream
	file("settings.txt");

	/*Acquisition Mode */
	file << "Acquisition mode" << endl;
	widget = lookup_widget(window, "radiobutton3");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "0" << endl;
	}

	widget = lookup_widget(window, "radiobutton4");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	}

	widget = lookup_widget(window, "radiobutton5");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "2" << endl;
	}
	widget = lookup_widget(window, "radiobutton6");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "3" << endl;
	}

	/*Samples per channel */
	widget = lookup_widget(window, "hscale18");

	val = gtk_range_get_value(GTK_RANGE(widget));
	int samples_per_channel;
	samples_per_channel = (int) val;
	file << "Samples per channel " << endl;
	file << samples_per_channel << endl;

	/*BLT EVENT NUMBER*/

	widget = lookup_widget(window, "hscale29");
	val = gtk_range_get_value(GTK_RANGE(widget));
	int blt_evnt_number;

	blt_evnt_number = (int) val;
	file << "BLT Event number " << endl;
	file << blt_evnt_number << endl;

	/*BERR e ALIGN64
	 widget=lookup_widget(window,"checkbutton55");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("BERR enabled\n");
	 bd->BERR_enable=1;

	 }
	 else{
	 printf("BERR disabled\n");
	 bd->BERR_enable=0;
	 }
	 widget=lookup_widget(window,"checkbutton56");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("ALIGN64 enabled\n");
	 bd->ALIGN64_enable=1;

	 }
	 else{
	 printf("ALIGN64 disabled\n");
	 bd->ALIGN64_enable=0;
	 }*/

	/*Interrupt Control
	 widget=lookup_widget(window,"checkbutton54");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("INTERRUPT on Optical Link enabled\n");
	 bd->optical_interrupt_enable=1;

	 }
	 else{
	 printf("INTERRUPT on Opctical Link disabled\n");
	 bd->optical_interrupt_enable=0;
	 }

	 widget=lookup_widget(window,"radiobutton7");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("INTERRUPT RORA\n");
	 bd->RORA_ROAK=0;

	 }
	 else{
	 printf("INTERRUPT ROAK\n");
	 bd->RORA_ROAK=1;
	 }



	 widget=lookup_widget(window,"hscale30");
	 val=gtk_range_get_value(GTK_RANGE(widget));
	 int int_event_number;
	 int_event_number=(int)val;
	 bd->interrupt_event_number=int_event_number;


	 widget=lookup_widget(window,"hscale32");
	 val=gtk_range_get_value(GTK_RANGE(widget));
	 int_event_number=(int)val;
	 bd->interrupt_level=int_event_number;
	 */

	/*TRIGGER CONTROL */
	file << "TRIGGER" << endl;
	/*COINCIDENCE LEVEL*/
	widget = lookup_widget(window, "hscale33");
	val = gtk_range_get_value(GTK_RANGE(widget));
	coinc_level = (int) val;
	file << "Coincidence level " << endl;
	file << coinc_level << endl;

	/*PROMPT-DELAY COINCIDENCE SOFTWARE */
	widget = lookup_widget(window, "hscale34");
	val = gtk_range_get_value(GTK_RANGE(widget));
	delay = (int) val;

	file << "Software prompt-delay gate (0:off) " << endl;
	file << delay << endl;

	file << "Count all triggers" << endl;
	widget = lookup_widget(window, "radiobutton1");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Internal trig polarity is negative:" << endl;
	widget = lookup_widget(window, "radiobutton9");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Enable ext trigger: " << endl;
	widget = lookup_widget(window, "checkbutton57");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Enable software trigger: " << endl;
	widget = lookup_widget(window, "checkbutton58");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Enable ext trigger out propagation: " << endl;
	widget = lookup_widget(window, "checkbutton59");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Enable soft trigger out propagation: " << endl;
	widget = lookup_widget(window, "checkbutton60");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Ext signals polarity is NIM:" << endl;

	/*Input polarity*/
	widget = lookup_widget(window, "radiobutton11");
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		file << "1" << endl;
	} else {
		file << "0" << endl;
	}

	file << "Post trigger setting: " << endl;
	widget = lookup_widget(window, "hscale31");
	val = gtk_range_get_value(GTK_RANGE(widget));
	file << val << endl;

	//trig n over thr. Now is common to all channels
	file << "Samples over thr for trigger: " << endl;
	widget = lookup_widget(window, "hscale35");
	val = gtk_range_get_value(GTK_RANGE(widget));
	file << val << endl;

	file << "CHANNELS:" << endl;
	/*single ch configuration */
	for (ii = 0; ii < NCH; ii++) {
		file << "Ch " << ii << " (Enabled? Trigger Enabled? - Propagate Trigger? - DAC Offset - ZLE Threshold)" << endl;

		sprintf(name, "checkbutton%i", ii);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			file << "1 ";
		} else {
			file << "0 ";
		}
		sprintf(name, "checkbutton%i", ii + 10);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			file << "1 ";
		} else {
			file << "0 ";
		}

		sprintf(name, "checkbutton%i", ii + 20);
		widget = lookup_widget(window, name);
		if (GTK_TOGGLE_BUTTON(widget)->active) {
			file << "1 ";
		} else {
			file << "0 ";
		}

		sprintf(name, "hscale%i", ii + 10);
		widget = lookup_widget(window, name);
		DACoffset[ii] = gtk_range_get_value(GTK_RANGE(widget));
		file << DACoffset[ii] << " ";

		sprintf(name, "hscale%i", ii + 0);
		widget = lookup_widget(window, name);
		val1 = gtk_range_get_value(GTK_RANGE(widget));
		file << val1 << " ";

		file << endl;
	}
	file.close();
}

void configure_load_callback(GtkWidget *window, gpointer data) {

	window2_data *my_data;
	my_data = (window2_data *) data;
	GtkWidget *window1;
	window1 = my_data->window1T;

	GtkWidget *widget;
	int ii;
	char name[100];
	double val, val1, val2;
	double DACoffset[NCH];
	int coinc_level;
	int delay;
	int tmp;
	string stmp;

	ifstream
	file("settings.txt");

	if (!file) {
		printf("Error, file settings.txt not found\n");
	}

	/*Acquisition Mode */
	// file<<"Acquisition mode"<<endl;
	file.ignore(1000, '\n');
	file >> tmp;

	switch (tmp) {
	case 0:
		widget = lookup_widget(window, "radiobutton3");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);
		break;
	case 1:
		widget = lookup_widget(window, "radiobutton4");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		break;
	case 2:
		widget = lookup_widget(window, "radiobutton5");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 2);
		break;
	case 3:
		widget = lookup_widget(window, "radiobutton6");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 3);
		break;
	default:
		printf("Error mode\n");
		break;
	}

	/*Samples per channel */
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	widget = lookup_widget(window, "hscale18");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	/*BLT EVENT NUMBER*/
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "hscale29");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	/*BERR e ALIGN64
	 widget=lookup_widget(window,"checkbutton55");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("BERR enabled\n");
	 bd->BERR_enable=1;

	 }
	 else{
	 printf("BERR disabled\n");
	 bd->BERR_enable=0;
	 }
	 widget=lookup_widget(window,"checkbutton56");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("ALIGN64 enabled\n");
	 bd->ALIGN64_enable=1;

	 }
	 else{
	 printf("ALIGN64 disabled\n");
	 bd->ALIGN64_enable=0;
	 }*/

	/*Interrupt Control
	 widget=lookup_widget(window,"checkbutton54");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("INTERRUPT on Optical Link enabled\n");
	 bd->optical_interrupt_enable=1;

	 }
	 else{
	 printf("INTERRUPT on Opctical Link disabled\n");
	 bd->optical_interrupt_enable=0;
	 }

	 widget=lookup_widget(window,"radiobutton7");
	 if (GTK_TOGGLE_BUTTON(widget)->active)
	 {
	 printf("INTERRUPT RORA\n");
	 bd->RORA_ROAK=0;

	 }
	 else{
	 printf("INTERRUPT ROAK\n");
	 bd->RORA_ROAK=1;
	 }



	 widget=lookup_widget(window,"hscale30");
	 val=gtk_range_get_value(GTK_RANGE(widget));
	 int int_event_number;
	 int_event_number=(int)val;
	 bd->interrupt_event_number=int_event_number;


	 widget=lookup_widget(window,"hscale32");
	 val=gtk_range_get_value(GTK_RANGE(widget));
	 int_event_number=(int)val;
	 bd->interrupt_level=int_event_number;
	 */

	/*TRIGGER CONTROL */
	file.ignore();
	file.ignore(1000, '\n');

	/*COINCIDENCE LEVEL*/
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "hscale33");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	/*PROMPT-DELAY COINCIDENCE SOFTWARE */
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "hscale34");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	widget = lookup_widget(window, "radiobutton1");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "radiobutton9");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "checkbutton57");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	widget = lookup_widget(window, "checkbutton58");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	widget = lookup_widget(window, "checkbutton59");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "checkbutton60");
	if (tmp) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	} else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	/*Input polarity*/
	widget = lookup_widget(window, "radiobutton11");
	if (tmp) {
		widget = lookup_widget(window, "radiobutton11");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);
	} else {
		widget = lookup_widget(window, "radiobutton12");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
	}
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;

	widget = lookup_widget(window, "hscale31");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	//trig n over thr. Now is common to all channels
	file.ignore();
	file.ignore(1000, '\n');
	file >> tmp;
	widget = lookup_widget(window, "hscale35");
	gtk_range_set_value(GTK_RANGE(widget), tmp);

	file.ignore();
	file.ignore(1000, '\n');

	/*single ch configuration */
	for (ii = 0; ii < NCH; ii++) {

		file.ignore();
		file.ignore(1000, '\n');
		//file<<"Ch "<<ii<<" (Enabled? Trigger Enabled? - Propagate Trigger? - DAC Offset - Threshold)" <<endl;
		file >> tmp;
		sprintf(name, "checkbutton%i", ii);
		widget = lookup_widget(window, name);
		if (tmp)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

		sprintf(name, "checkbutton%i", ii + 10);
		widget = lookup_widget(window, name);
		file >> tmp;
		if (tmp)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

		sprintf(name, "checkbutton%i", ii + 20);
		widget = lookup_widget(window, name);
		file >> tmp;
		if (tmp)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 1);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0);

		sprintf(name, "hscale%i", ii + 10);
		widget = lookup_widget(window, name);
		file >> tmp;
		gtk_range_set_value(GTK_RANGE(widget), tmp);

		sprintf(name, "hscale%i", ii + 0);
		widget = lookup_widget(window, name);
		file >> tmp;
		gtk_range_set_value(GTK_RANGE(widget), tmp);
		printf("%i %i aa \n", ii, tmp);
		file.ignore();
	}
	file.close();
	gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(window)));
}


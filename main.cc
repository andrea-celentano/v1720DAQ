/*ROOT STUFF */
#include "TTree.h"
#include "TFile.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TApplication.h"
#include "TMultiGraph.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TH1D.h"

extern "C" {
#include "CAENComm.h"

}
#include <pthread.h>
#include "v1720.h"
#include <gtk/gtk.h>
#include "interface.h"
#include "callback.h"
#include "support.h"
#include "struct_event.h" //struct to take data

#include "fadc.h"
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define DEBUG 2

#define BUF_LENGTH 41000000
#define MAX_LENGTH 4100
#define FADC_HEADER 0xA //1010->MSB of the Header of the Board.

#define FADC_CHANNELS_PER_BOARD NCH

/*TRIGGER TIME TAG IS A 31 bit counter.*/
#define TIME_OVFL 0x7FFFFFFF

using namespace std;

TApplication gui("gui", 0, NULL);
//int pthread_create(pthread_t * pth, pthread_attr_t *att, void * (*function), void * arg);
//int pthread_cancel(pthread_t thread);
//pthread_join(pth, NULL /* void ** return value could go here */);
//pthread_t pth;	// this is our thread identifier

//pthread_mutex_init, which takes 2 parameters. The first is a pointer to a mutex_t that we're creating. The second parameter is usually NULL
//pthread_mutex_lock(*mutex_t)
//pthread_mutex_unlock(*mutex_t)

V1720_board bd; //global var to handle v1720

fadc_analizer *MyFadc;

uint32_t buf[BUF_LENGTH];
uint32_t *read_buf;
uint32_t *write_buf;
int nw; /*number of words read while BLT */
//int nevents; //the number of events we've written to the disk

/********** Global vars for the output and for simple analisis ***********/
//FILE *out_file; /*output file */
uint32_t time_difference;
TTree *out_tree;
TFile *out_file;
ofstream file_txt;

event this_event;

pthread_mutex_t write_mutex;
pthread_mutex_t tree_mutex;

int is_first = 1;
int prev_samples = 0;
int this_samples = 0;

/* The function to decode events and write them to disk/root file/whatever it is.. */
/*At the beginning of the function,*write_buf point to the first word to be written or analized */
void decode_event(int event_size) {
#ifdef DEBUG
  printf("decode_event called, size: %i [zs: %i]\n",event_size,bd.zs_mode);
#endif 
	int fadc_data1, fadc_data2;
	int offset;
	int ichannel, isample, thechannel, prev_ichannel;
	int temp_channel_mask;
	int ii = 0;
	int jj = 0;
	int ctrl;
	int isize, isize2, ipulse, itime;

#ifdef V1725
	offset=0x3FFF;
#else
	offset=0xFFF;
#endif

	for (ii = 0; ii < FADC_CHANNELS_PER_BOARD; ii++) {
		this_event.nPEAKS[ii] = 0;
		for (jj = 0; jj < MAX_PEAKS; jj++) {
			this_event.peakStartTime[ii][jj] = 0;
			this_event.nFADC[ii][jj] = 0;
		}
	}

	*write_buf = 0x0;
	write_buf++; //ignore first word,

	this_event.channel_mask = (*write_buf) & 0xFF; //second word
	*write_buf = 0x0;
	write_buf++;

	this_event.num = *(write_buf)&0xFFFFFF;
#ifdef V1725
	this_event.channel_mask = this_event.channel_mask | (((*(write_buf)>>24)&0xFF)<<8);
#endif
	this_event.active_channels = 0;
	for (int qq=0;qq<FADC_CHANNELS_PER_BOARD;qq++){
	  this_event.active_channels += (this_event.channel_mask >> qq) &0x1;
	}

	*write_buf = 0x0;
	write_buf++;

	this_event.trig_time = (*write_buf) & 0x7FFFFFFF; //MSB is overflow
	*write_buf = 0x0;
	write_buf++;

	event_size = event_size - 4; //parole a 32-bit con i dati. In totale, tutti i sample sono 2*event_size (1 parola: 2 sample)

	this_event.samples_per_channel = bd.custom_size * 4; //A.C. FIX
	prev_samples = this_samples;
	this_samples = this_event.samples_per_channel;

	if ((is_first) || (prev_samples != this_samples)) {
		printf("Preparing vector for samples, something has changed OR this is first event\n");
		is_first = 0;

		printf("Setting up FADC ANALIZER\n");
		MyFadc->Setup(this_samples / 5);
		printf("Done setting up FADC ANALIZER\n");
	}

	thechannel = 0;
	ichannel = 0;
	temp_channel_mask = this_event.channel_mask;




	if (bd.zs_mode == 0) {
	  //A.C. work-around recompute samples
	  this_event.samples_per_channel=(2*event_size)/this_event.active_channels;
	  ii=0;
	  ichannel=0;
	  while (ii < event_size){ //counter on event

	    //loop on active channels
	    for (int ich=0;ich<this_event.active_channels;ich++){
	      //determine the ID of the current channel
#ifdef DEBUG
	      printf("decode_event start ch %i/%i %x\n",ich,this_event.active_channels,temp_channel_mask);
#endif
	      while ((temp_channel_mask&0x1) == 0){
		temp_channel_mask=(temp_channel_mask>>1)&0xFFFF;
		ichannel++;
	      }
	     

	      //loop over the samples
	      for (int qq=0;qq<this_event.samples_per_channel/2;qq++){ //samples_per_channel is even
		fadc_data1 = (*write_buf) & 0x3FFF;         //A.C. set to 0x3FFF on 27/10
		fadc_data2 = ((*write_buf) >> 16) & 0x3FFF; //A.C. set to 0x3FFF on 27/10
		*write_buf = 0x0;
		write_buf++;
		ii++;
		//fadc_data1 = -offset + fadc_data1; //THESE ARE bits, not mV
		//fadc_data2 = -offset + fadc_data2; //THESE ARE bits, not mV
		
		this_event.fadc[ichannel][0][2*qq] = fadc_data1;
		this_event.fadc[ichannel][0][2*qq + 1] = fadc_data2;
#ifdef DEBUG
		printf("%i %i %i: %x %x\n",ii,ichannel,qq,fadc_data1,fadc_data2);
#endif
	      }
	      //data of channel "ichannel" read
	      this_event.nPEAKS[ichannel] = 1;
	      this_event.nFADC[ichannel][0] = this_event.samples_per_channel;
	      
	      MyFadc->LoadEvent(this_event.fadc[ichannel][0], this_event.nFADC[ichannel][0]);
	      MyFadc->ProcessEvent();
	      
	      this_event.ped_mean[ichannel] = MyFadc->GetPedMean();
	      this_event.ped_sigma[ichannel] = MyFadc->GetPedSigma();
	      
	      this_event.peaks[ichannel][0].peak_start = MyFadc->GetPeakStart();
	      this_event.peaks[ichannel][0].peak_end = MyFadc->GetPeakEnd();
	      this_event.peaks[ichannel][0].peak_val = MyFadc->GetPeakValue();
	      this_event.peaks[ichannel][0].peak_position = MyFadc->GetPeakPosition();
	      this_event.peaks[ichannel][0].time = MyFadc->GetTime(0);
	      this_event.peaks[ichannel][0].energy = MyFadc->GetEnergy(0);

	      temp_channel_mask=temp_channel_mask>>1;
	      ichannel++;
	     
	    }
	  }
	} else {
		ii = 0;
		isample = 0;
		while (ii < event_size) {
			isize = (*write_buf++); //Total number of words for this channel
			*write_buf = 0x0;
			isize = isize - 1; 	        //First word doesn't count
			ii++;
			while ((temp_channel_mask & 0x01) != 1) {
				temp_channel_mask = temp_channel_mask >> 1;
				thechannel++;
			}
			isample = 0;
			itime = 0;
			ipulse = 0;
			jj = 0;
			while (jj < isize) {
				ctrl = (*write_buf++);
				*write_buf = 0x0;
				ii++;
				jj++;
				isize2 = ctrl & 0xFFFFF;
				if (ctrl >> 31 & 0x1 == 0) { //These are SKIPPED samples
					itime += isize2;
				} else {
					isample = 0;
					this_event.nPEAKS[thechannel]++;
					this_event.peakStartTime[thechannel][ipulse] = itime * 4; //each sample is 4ns
					this_event.nFADC[thechannel][ipulse] = isize2 * 2;

					for (int kk = 0; kk < isize2; kk++) {
 					        fadc_data1 = (*write_buf) & 0x3FFF;             //A.C. set to 3FFF on 27/10/2025: it is ok also for v1720 (bits are 0)
						fadc_data2 = ((*write_buf) >> 16) & 0x3FFF;     //A.C. set to 3FFF on 27/10/2025: it is ok also for v1720 (bits are 0)
						*write_buf = 0x0;
						write_buf++;
						ii++;
						jj++;
						fadc_data1 = -offset + fadc_data1; //THESE ARE bits, not mV
						fadc_data2 = -offset + fadc_data2; //THESE ARE bits, not mV
						this_event.fadc[thechannel][ipulse][isample++] = fadc_data1;
						this_event.fadc[thechannel][ipulse][isample++] = fadc_data2;
					}

					MyFadc->LoadEvent(this_event.fadc[thechannel][ipulse], this_event.nFADC[thechannel][ipulse]);
					MyFadc->ProcessEvent();
					if (ipulse == 0) {
						this_event.ped_mean[thechannel] = MyFadc->GetPedMean();
						this_event.ped_sigma[thechannel] = MyFadc->GetPedSigma();
					}
					this_event.peaks[thechannel][ipulse].peak_start = MyFadc->GetPeakStart();
					this_event.peaks[thechannel][ipulse].peak_end = MyFadc->GetPeakEnd();
					this_event.peaks[thechannel][ipulse].peak_position = MyFadc->GetPeakPosition();
					this_event.peaks[thechannel][ipulse].time = MyFadc->GetTime(0) + this_event.peakStartTime[thechannel][ipulse];
					this_event.peaks[thechannel][ipulse].energy = MyFadc->GetEnergy(0);
					ipulse++;
					itime += isize2;

				}
			}
		}
	}

	out_tree->Fill();
	if (this_event.num % 10000 == 0) out_tree->AutoSave("SaveSelf");
#ifdef DEBUG
	printf("event decoded");
#endif
}

/*Function for gui thread: this handles gtk! */
void * gui_fun(void *arg) {

	printf("Start gui thread\n");
	gtk_set_locale();
	gtk_init(0, 0);

	GtkWidget *window2;
	window2 = create_window2();

	GtkWidget *window1;
	window1 = create_window1();

	g_signal_connect(window1, "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect_swapped(window1, "destroy", G_CALLBACK(gtk_widget_destroy), window1);
	g_signal_connect(window1, "destroy", gtk_main_quit, NULL);

	/*quit*/
	button5_data data_temp_button5;
	data_temp_button5.bd = &bd;
	data_temp_button5.file = out_file;
	data_temp_button5.tree = out_tree;
	data_temp_button5.file_txt = &(file_txt);
	data_temp_button5.read_buf = &read_buf;
	data_temp_button5.write_buf = &write_buf;

	GtkWidget *button5;
	button5 = lookup_widget(window1, "button5");
	g_signal_connect(button5, "clicked", G_CALLBACK(on_button5_clicked), (gpointer)(&data_temp_button5));

//g_signal_connect_swapped(button5,"clicked",G_CALLBACK(gtk_widget_destroy),window1);

	/*configure*/
	GtkWidget *button2;
	button2 = lookup_widget(window1, "button2");

	button2_data data_temp_button2;

	(data_temp_button2.window1T) = window1;
	data_temp_button2.window2T = window2;
	g_signal_connect(button2, "clicked", G_CALLBACK(on_button2_clicked), (gpointer)(&data_temp_button2));

	GtkWidget *button3;/*start*/
	button3 = lookup_widget(window1, "button3");
	g_signal_connect(button3, "clicked", G_CALLBACK(on_button3_clicked), (gpointer)(&bd));

	GtkWidget *button4;
	button4 = lookup_widget(window1, "button4");
	g_signal_connect(button4, "clicked", G_CALLBACK(on_button4_clicked), (gpointer)(&bd));

	/*Abilito solo configure all'avvio, ovvero button 2, e quit, ovvero button 5*/
	GtkWidget *button6;
	button6 = lookup_widget(window1, "button6");
	g_signal_connect(button6, "clicked", G_CALLBACK(on_button6_clicked), (gpointer)(&bd));

	gtk_widget_set_sensitive(button3, 0);
	gtk_widget_set_sensitive(button4, 0);
//gtk_widget_set_sensitive(button6,0);

	/**********************************************
	 *********************************************
	 *********************************************
	 ************CONFIGURE WINDOW CALLBACKS*******
	 *********************************************
	 *********************************************
	 ********************************************/

	GtkWidget *button;

	/*****We have only three callbacks for the whole configuration panel*****/
	button = lookup_widget(window2, "button1"); /*Apply*/
	window2_data window2_data_temp;
	window2_data_temp.window1T = window1;
	window2_data_temp.board = &bd;
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(configure_enable_callback), (gpointer)(&window2_data_temp));

	button = lookup_widget(window2, "button2"); /*Load from file*/
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(configure_load_callback), (gpointer)(&window2_data_temp));

	button = lookup_widget(window2, "button3"); /*Save to file*/
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(configure_save_callback), (gpointer)(&window2_data_temp));

	/*SHOW WINDOW 1*/
	gtk_widget_show(window1);
	gtk_main();
	return 0;
}

/*Probably it's not good to use interrupts on trigger, since T_delay - T_prompt ~ us: we can't go so fast.'
 Better to use polling */
void * read_fun(void *arg) {

	uint32_t n_events_ready = 0;

//nevents=0;
	read_buf = buf; //At the beginning, set the pointer at 0.

	while (1) {

		if (bd.start_stop == 1) {
			while (n_events_ready == 0) {
				CAENComm_Read32(bd.handle, V1720event_stored, &n_events_ready);
			}

			//  CAENComm_IRQWait(bd.handle,-1);
			/*Clear IRQ via RORA */
			//CAENComm_Read32(bd.handle,V1720vme_control,&reg);
			//reg=reg & 0xFFFFFFF8;
			//CAENComm_Write32(bd.handle,V1720vme_control,reg);
			// CAENComm_IRQDisable(bd.handle);
#ifdef DEBUG
			printf("events ready are %i \n",n_events_ready);
			printf("read_buf is: %p. read_buf+Max_length is : %p, buf+BUf_length is %p\n",read_buf,read_buf+MAX_LENGTH,buf+BUF_LENGTH);
			printf("write_buf is %p \n",write_buf);
#endif

			pthread_mutex_lock(&write_mutex);
			CAENComm_BLTRead(bd.handle, V1720data, read_buf, 32800, &nw); /*328400=16400*2. 16400 is max number of bytes with 8 ch: 4 byte header, 8*2048 byte data*//*This function wants the number of bytes to read, not the number of 32-bit words*/

			//nevents=nevents+n_events_ready;
			n_events_ready = 0;
			read_buf += nw; /*increment the pointer to the next free location */
			if ((read_buf + MAX_LENGTH) > (buf + BUF_LENGTH)) read_buf = buf; /*if there's no more place, let's start from the beginning.*/
			pthread_mutex_unlock(&write_mutex);

			/*Set IRQ again */
			//reg=reg+bd.interrupt_level;
			//CAENComm_Write32(bd.handle,V1720vme_control,reg);
			// CAENComm_IRQEnable(bd.handle);
#ifdef DEBUG_PLUS 
			int ii;
			for (ii=0;ii<2*(MAX_LENGTH);ii++) {
				printf("%x ",buf[ii]);
			}
			printf("\n Read 32-bit word: %i \n",nw);
#endif 
		}
	}
}

/*This is the write thread. We continue moving along the buffer serching for events. */
/*Trick: when we write an event, we put to 0000 the MSB of the header, so the event will not be read again */
void * write_fun(void *arg) {

	int event_size;
	event_size = 0;

	write_buf = buf;   //At the beginning, set the pointer at 0.
	while (bd.start_stop == 0) {
	}
	while (1) {

		/*Here I search for the DATA header*/
		if ((((*write_buf) >> 28) & 0xF) == FADC_HEADER) {
			event_size = (int) ((*write_buf) & 0xFFFFFFF);
#ifdef DEBUG
			printf("%i %x %x \n",event_size,*write_buf,*(write_buf+2));
#endif
			if (event_size < MAX_LENGTH) { /*Trigger time tag could be misleading!*/
#ifdef DEBUG
				printf("%p %x %x %x %x %x %x %x %x %x ",write_buf,*write_buf,*(write_buf+1),*(write_buf+2),*(write_buf+3),*(write_buf+4),*(write_buf+5),*(write_buf+6),*(write_buf+7),*(write_buf+8));
				printf(" decode \n");
#endif 

				(*write_buf) = 0x0; /*we put the header at 0 so it wont't be recognised again.*/
				pthread_mutex_lock(&tree_mutex);
				decode_event(event_size);
				pthread_mutex_unlock(&tree_mutex);
				write_buf--; //to fix where write_buf points at the end of decode_event
#ifdef DEBUG
				printf(" %p %x \n",write_buf,*write_buf);
#endif 

			}
		}
		if (write_buf == (buf + BUF_LENGTH))
			write_buf = buf;
		else
			write_buf++;

		//  printf("data at %p is %x \n",write_buf,*write_buf);
#ifdef DEBUG_PLUS  
		printf("data at %p is %x \n",write_buf,*write_buf);
#endif 
	}
}

/*just a simple function to print out the rate */
/*And to do some monitoring!*/
void * rate_fun(void *arg) {
	printf("Start rate thr \n");
	uint64_t CurrentTime, PrevRateTime, ElapsedTime, TotalElapsedTime, OriginalTime;
	uint64_t nevents_diff = 0;
	uint64_t nevents_prev = 0;
	uint64_t nevents = 0;
	PrevRateTime = 0;

	float diff_rate, int_rate;

	TCanvas *c_waveforms1 = new TCanvas("Waveforms1", "Waveforms ch0-7", 1200, 800);
	c_waveforms1->Divide(3, 3);
	TCanvas *c_waveforms2 = 0;
#ifdef V1725
	c_waveforms2=new TCanvas("Waveforms2", "Waveforms ch8-15", 1200, 800);
	c_waveforms2->Divide(3, 3);
#endif
	TGraph **waveforms = new TGraph*[FADC_CHANNELS_PER_BOARD];
	for (int ii = 0; ii < FADC_CHANNELS_PER_BOARD; ii++) {
		waveforms[ii] = new TGraph();
		waveforms[ii]->SetTitle(Form("CH %i", ii));
		waveforms[ii]->GetXaxis()->SetTitle("Time (ns)");
		waveforms[ii]->GetYaxis()->SetTitle("Voltage (mV)");
		waveforms[ii]->SetMarkerStyle(7);
	}

	/*RATES*/
	TGraph *gdiff_rate = new TGraph();
	gdiff_rate->GetXaxis()->SetTitle("Time (5s)");
	gdiff_rate->GetYaxis()->SetTitle("Rate (Hz)");
	gdiff_rate->SetMarkerStyle(7);

	TGraph *gint_rate = new TGraph();
	gint_rate->GetXaxis()->SetTitle("Time (5s)");
	gint_rate->GetYaxis()->SetTitle("Rate (Hz)");
	gint_rate->SetMarkerStyle(7);
	gint_rate->SetMarkerColor(2);

	//	TMultiGraph *grate = NULL;

	int rate_counter = 0;
	const int n_rate_points = 500;
	double LSB=0.4884;
#ifdef V1725
	LSB=0.1221;
#endif
	while (1) {
		if (bd.start_stop == 1) {
			if (rate_counter == 0) OriginalTime = get_time();
			CurrentTime = get_time();
			ElapsedTime = CurrentTime - PrevRateTime;
			TotalElapsedTime = CurrentTime - OriginalTime;

			if ((ElapsedTime > 5000) && (bd.start_stop == 1)) {

				nevents_prev = nevents;
				nevents = this_event.num;
				nevents_diff = nevents - nevents_prev;

				pthread_mutex_lock(&tree_mutex);
				int_rate = (float) nevents / (float) (TotalElapsedTime);
				int_rate *= 1000.0f;
				if (nevents_diff == 0) {
					diff_rate = 0;
					printf("No data...\n");
				} else {
					diff_rate = (float) nevents_diff * 1000.0f / (float) ElapsedTime;
					for (int ii = 0; ii < FADC_CHANNELS_PER_BOARD; ii++) {

					
						//Draw current waveform
						waveforms[ii]->Clear();				   
						if ((this_event.channel_mask >> ii) & 0x1 == 1) {
							for (int jj = 0; jj < this_event.nFADC[ii][0]; jj++) {
								waveforms[ii]->SetPoint(jj, jj * 4, this_event.fadc[ii][0][jj] * LSB);	//THESE ARE mV
							}
							if (ii<8){
							  c_waveforms1->cd(ii + 1);
							}
#ifdef V1725
							else{
							  c_waveforms2->cd(ii-8+1);
							}
#endif
							waveforms[ii]->Draw("ALP");
						}

					}

					// printf("%i %i %i %i",nevents_diff,nevents,ElapsedTime,TotalElapsedTime);
					printf("Trg Rate: %.2f Hz (differential), %.2f Hz (integrated) \n", diff_rate, int_rate);
					// for (int jj=0;jj<FADC_CHANNELS_PER_BOARD;jj++) printf(" %i: E: %.2f V: %.2f T: %.2f Ped: %.2f start: %.2f stop: %.2f	\n",jj,this_event.energy[jj],this_event.peak_val[jj],this_event.time[jj],this_event.ped_mean[jj],this_event.peak_start[jj],this_event.peak_end[jj]);
					printf("\n");
					nevents_diff = 0;
				}
				if (rate_counter == 0) {
					gdiff_rate->Set(0);
					gint_rate->Set(0);
				}
				gdiff_rate->SetPoint(rate_counter, rate_counter * 5, diff_rate);
				gint_rate->SetPoint(rate_counter, rate_counter * 5, int_rate);
				rate_counter++;
				c_waveforms1->cd(9);
				gint_rate->Draw("AP");
				gdiff_rate->Draw("PSAME");

				if (rate_counter > n_rate_points) {
					gint_rate->GetXaxis()->SetRangeUser((rate_counter - n_rate_points) * 5, rate_counter * 5);
				}
				gint_rate->GetYaxis()->SetRangeUser(0, int_rate * 2);
				//else grate->GetXaxis()->SetRangeUser(0,n_rate_points*5);
				c_waveforms1->Modified();
				c_waveforms1->Update();
#ifdef V1725
				c_waveforms2->Modified();
				c_waveforms2->Update();
#endif
				PrevRateTime = CurrentTime;
				pthread_mutex_unlock(&tree_mutex);
			} //end if elapsed time
		} //end if (start_stop==1, i.e. board is acquiring)
		else {
			rate_counter = 0;
		}
	} //end while(1)
}

int main(int argc, char **argv) {

	gROOT->SetStyle("Plain");
	gStyle->SetPalette(1);
	gStyle->SetNumberContours(100);

	file_txt.open("out.txt");

	char OutputTime[20];
	char fname[50];
	time_t aclock;
	struct tm *newtime;
	aclock = time(NULL);
	newtime = localtime(&aclock);
	strftime(OutputTime, sizeof(OutputTime), "%Y%m%d%H%M%S", newtime);

	strcpy(fname, "root/data");
	strcat(fname, OutputTime);
	strcat(fname, ".root");

	time_t now;
	time(&now);
	printf("Starting at %s", ctime(&now));

	out_file = new TFile(fname, "recreate");
	out_tree = new TTree("out", "out");

//out_tree->Branch("FADC","std::vector < std::vector < int > >",&this_event.fadc);


	out_tree->Branch("TRIG_TIME", &this_event.trig_time);
	out_tree->Branch("NUM", &this_event.num);

	out_tree->Branch("SAMPLE_PER_CHANNEL", &this_event.samples_per_channel);
	out_tree->Branch("CHANNEL_MASK", &this_event.channel_mask);
	out_tree->Branch("ACTIVE_CHANNELS", &this_event.active_channels);


	out_tree->Branch("EVENT", &this_event);

	this_event.trig_time = 0;
	this_event.num = 0;

	int ii;
	for (ii = 0; ii < BUF_LENGTH; ii++)
		buf[ii] = 0;
	bd.handle = -1;
	bd.start_stop = 0;


	MyFadc = new fadc_analizer(); //constructor only wants the number of channels
	MyFadc->Setup(10); //Setup is then done in the function for decoding
	MyFadc->PrintEnergyMethods();
	MyFadc->PrintTimeMethods();

	/*Create thread for gui */
	pthread_t GUI_tr;
	pthread_create(&GUI_tr, NULL, gui_fun, NULL);

	/*Create thread for readout */
	pthread_t READ_tr;
	pthread_create(&READ_tr, NULL, read_fun, NULL);

	/*Create thread to write on disk/root Tree /whatever it is */
	pthread_t WRITE_tr;
	pthread_create(&WRITE_tr, NULL, write_fun, NULL);

	/*Create simple thread just to print out the rate */
	pthread_t RATE_tr;
	pthread_create(&RATE_tr, NULL, rate_fun, NULL);

	pthread_join(RATE_tr, NULL);
	pthread_join(GUI_tr, NULL);
	pthread_join(READ_tr, NULL);
	pthread_join(WRITE_tr, NULL);


	v1720Close(bd.handle);

	return 1;
}

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
#include <vector>
#include <sstream>
#include <cmath>

//#define DEBUG 2

#define TIME_ELAPSED_GUI_UPDATE_MS 2000

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
int nw; /*number of words read while BLT */
//int nevents; //the number of events we've written to the disk

/********** Global vars for the output and for simple analisis ***********/
//FILE *out_file; /*output file */
uint32_t time_difference;
TTree *out_tree;
TFile *out_file;


event this_event;

pthread_mutex_t write_mutex;
pthread_mutex_t tree_mutex;

int is_first = 1;
int prev_samples = 0;
int this_samples = 0;

// --- Energy monitor globals (visualization only) ---
std::vector<TH1D*> g_h_energy;
TH1D *g_h_etot = nullptr;
std::vector<int> g_ch_nbins;
std::vector<double> g_ch_xmin;
std::vector<double> g_ch_xmax;
std::vector<double> g_ch_calib;
int g_total_nbins = 100;
double g_total_xmin = 0.0;
double g_total_xmax = 1000.0;
double g_total_calib = 1.0;

/* Create and configure energy-monitor histograms and attach them to `file` (if provided).
   This function is exposed to C callbacks via prototype in callback.h */
void setup_energy_monitor(TFile *file) {
	// defaults
	int default_nbins = 100;
	double default_xmin = 0.0;
	double default_xmax = 1000.0;
	double default_calib = 1.0;

	int nch = FADC_CHANNELS_PER_BOARD;
	g_ch_nbins.assign(nch, default_nbins);
	g_ch_xmin.assign(nch, default_xmin);
	g_ch_xmax.assign(nch, default_xmax);
	g_ch_calib.assign(nch, default_calib);

	// try to read configuration file (optional)
	std::string cfg_path = "energy_monitor.cfg";
	std::ifstream cfg(cfg_path.c_str());
	if (cfg) {
		std::string line;
		while (std::getline(cfg, line)) {
			std::string s = line;
			// skip comments
			if (s.find('#') == 0) continue;
			size_t hashpos = s.find('#');
			if (hashpos != std::string::npos) s = s.substr(0, hashpos);
			// trim
			auto ltrim = [](std::string &st){ size_t p = st.find_first_not_of(" \t\r\n"); if (p!=std::string::npos) st=st.substr(p); else st.clear(); };
			auto rtrim = [](std::string &st){ size_t p = st.find_last_not_of(" \t\r\n"); if (p!=std::string::npos) st=st.substr(0,p+1); else st.clear(); };
			ltrim(s); rtrim(s);
			if (s.empty()) continue;
			size_t eq = s.find('=');
			if (eq==std::string::npos) continue;
			std::string key = s.substr(0,eq);
			std::string val = s.substr(eq+1);
			ltrim(key); rtrim(key); ltrim(val); rtrim(val);

			if (key=="nbins") default_nbins = std::stoi(val);
			else if (key=="xmin") default_xmin = std::stod(val);
			else if (key=="xmax") default_xmax = std::stod(val);
			else if (key=="calib") default_calib = std::stod(val);
			else if (key.rfind("ch",0)==0) {
				size_t dot = key.find('.');
				if (dot==std::string::npos) continue;
				std::string chs = key.substr(2,dot-2);
				int idx = std::stoi(chs);
				if (idx<0 || idx>=nch) continue;
				std::string param = key.substr(dot+1);
				if (param=="nbins") g_ch_nbins[idx] = std::stoi(val);
				else if (param=="xmin") g_ch_xmin[idx] = std::stod(val);
				else if (param=="xmax") g_ch_xmax[idx] = std::stod(val);
				else if (param=="calib") g_ch_calib[idx] = std::stod(val);
			}
			else if (key.rfind("total.",0)==0) {
				std::string param = key.substr(6);
				if (param=="nbins") g_total_nbins = std::stoi(val);
				else if (param=="xmin") g_total_xmin = std::stod(val);
				else if (param=="xmax") g_total_xmax = std::stod(val);
				else if (param=="calib") g_total_calib = std::stod(val);
			}
		}
		// apply global defaults where not overridden
		for (int i=0;i<nch;i++){
			if (g_ch_nbins[i]==default_nbins) g_ch_nbins[i]=default_nbins;
			if (g_ch_xmin[i]==default_xmin) g_ch_xmin[i]=default_xmin;
			if (g_ch_xmax[i]==default_xmax) g_ch_xmax[i]=default_xmax;
			if (g_ch_calib[i]==default_calib) g_ch_calib[i]=default_calib;
		}
	}

	// create histograms (attach to file if provided)
	if (file) file->cd();

	g_h_energy.clear();
	for (int ch=0; ch<nch; ++ch) {
		std::ostringstream name, title;
		name << "h_energy_ch" << ch;
		title << "Energy ch" << ch << ";Energy (arb);Counts";
		int nb = g_ch_nbins[ch];
		double xmin = g_ch_xmin[ch];
		double xmax = g_ch_xmax[ch];
		TH1D *h = new TH1D(name.str().c_str(), title.str().c_str(), nb, xmin, xmax);
		g_h_energy.push_back(h);
	}
	g_h_etot = new TH1D("h_energy_total","Total energy;Energy (arb);Counts", g_total_nbins, g_total_xmin, g_total_xmax);
}

/* The function to decode events and write them to disk/root file/whatever it is.. */
/*At the beginning of the function,*write_buf point to the first word to be written or analized */
void decode_event(int event_size,uint32_t *write_buf) {
#ifdef DEBUG
  printf("decode_event called, size: %i 0x%x\n",event_size,*write_buf);
#endif 
	int fadc_data1, fadc_data2;
	int offset;
	int ichannel, isample, thechannel, prev_ichannel;
	int temp_channel_mask;
	int ii = 0;
	int jj = 0;
	int ctrl;
	int isize, isize2, ipulse, itime;
	double _etot_event = 0.0; // accumulate total energy for this event (raw units)

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
	      {
		double e = this_event.peaks[ichannel][0].energy;
		if ((int)g_h_energy.size() > ichannel) {
		  double evis = e * ( (ichannel < (int)g_ch_calib.size()) ? g_ch_calib[ichannel] : 1.0 );
		  if (g_h_energy[ichannel]) g_h_energy[ichannel]->Fill(evis);
		}
		_etot_event += (std::isfinite(evis) ? evis : 0.0);
	      }
	      
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
					{
						double e = this_event.peaks[thechannel][ipulse].energy;
						if ((int)g_h_energy.size() > thechannel) {
							double evis = e * ( (thechannel < (int)g_ch_calib.size()) ? g_ch_calib[thechannel] : 1.0 );
							if (g_h_energy[thechannel]) g_h_energy[thechannel]->Fill(evis);
						}
						_etot_event += (std::isfinite(evis) ? evis : 0.0);
					}
					ipulse++;
					itime += isize2;

				}
			}
		}
	}
		// Fill total-energy histogram (visualization)
		if (g_h_etot) {
		  double et_vis = _etot_event * g_total_calib;
		  if (std::isfinite(et_vis)) g_h_etot->Fill(et_vis);
		}

		if (out_tree){
			out_tree->Fill();
			if (this_event.num % 10000 == 0) out_tree->AutoSave("SaveSelf");
		}

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
	button_data data_temp_button;
	data_temp_button.bd = &bd;
	data_temp_button.file = &out_file;
	data_temp_button.tree = &out_tree;
	data_temp_button.this_event = &this_event;
	GtkWidget *button5;
	button5 = lookup_widget(window1, "button5");
	g_signal_connect(button5, "clicked", G_CALLBACK(on_button5_clicked), (gpointer)(&data_temp_button));


	/*configure*/
	GtkWidget *button2;
	button2 = lookup_widget(window1, "button2");
	button2_data data_temp_button2;
	(data_temp_button2.window1T) = window1;
	data_temp_button2.window2T = window2;
	g_signal_connect(button2, "clicked", G_CALLBACK(on_button2_clicked), (gpointer)(&data_temp_button2));



	GtkWidget *button3;
	button3 = lookup_widget(window1, "button3");
	g_signal_connect(button3, "clicked", G_CALLBACK(on_button3_clicked), (gpointer)(&data_temp_button));

	//Stop
	GtkWidget *button4;
	button4 = lookup_widget(window1, "button4");
	g_signal_connect(button4, "clicked", G_CALLBACK(on_button4_clicked), (gpointer)(&data_temp_button));

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
	uint32_t event_size = 0;

//nevents=0;
	read_buf = buf; //At the beginning, set the pointer at 0.

	while (1) {

		if (bd.start_stop == 1) {
			while (n_events_ready == 0) {
				CAENComm_Read32(bd.handle, V1720event_stored, &n_events_ready);
			}

	   
#ifdef DEBUG
			printf("events ready are %i \n",n_events_ready);
			printf("read_buf is: %p. read_buf+Max_length is : %p, buf+BUf_length is %p\n",read_buf,read_buf+MAX_LENGTH,buf+BUF_LENGTH);
#endif
			
			pthread_mutex_lock(&write_mutex);
			CAENComm_BLTRead(bd.handle, V1720data, read_buf, 32800, &nw);
                        /*328400=16400*2. 16400 is max number of bytes with 8 ch: 4 byte header, 8*2048 byte data*/
                        /*This function wants the number of bytes to read, not the number of 32-bit words*/

		
		
			event_size = (int) ((*read_buf) & 0xFFFFFFF);
			pthread_mutex_lock(&tree_mutex);
			decode_event(event_size,read_buf);
			pthread_mutex_unlock(&tree_mutex);
			//nevents=nevents+n_events_ready;
			n_events_ready = 0;
			pthread_mutex_unlock(&write_mutex);

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

	TCanvas *c_ene1 = new TCanvas("Ene1", "Ene ch0-7", 1200, 800);
	c_ene1->Divide(3, 3);
	TCanvas *c_ene2 = 0;

#ifdef V1725
	c_waveforms2=new TCanvas("Waveforms2", "Waveforms ch8-15", 1200, 800);
	c_waveforms2->Divide(3, 3);
	c_ene2=new TCanvas("Ene2", "Ene ch8-15", 1200, 800);
	c_ene2->Divide(3, 3);
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
	gdiff_rate->GetXaxis()->SetTitle("Time (s)");
	gdiff_rate->GetYaxis()->SetTitle("Rate (Hz)");
	gdiff_rate->SetMarkerStyle(7);

	TGraph *gint_rate = new TGraph();
	gint_rate->GetXaxis()->SetTitle("Time (s)");
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

			if ((ElapsedTime > TIME_ELAPSED_GUI_UPDATE_MS) && (bd.start_stop == 1)) {

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
					    
					    // Draw energy spectrum
					    if (ii<8){
					      c_ene1->cd(ii + 1);
					    }
#ifdef V1725
					    else{
					      c_ene2->cd(ii-8+1);
					    }
#endif
					    g_h_energy[ii]->Draw();
					  } 
					}

					printf("Trg Rate: %.2f Hz (differential), %.2f Hz (integrated) \n", diff_rate, int_rate);
				
					printf("\n");
					nevents_diff = 0;
				}
				if (rate_counter == 0) {
				  gdiff_rate->Set(0);
				  gint_rate->Set(0);
				}
				gdiff_rate->SetPoint(rate_counter, rate_counter * TIME_ELAPSED_GUI_UPDATE_MS/1000., diff_rate);
				gint_rate->SetPoint(rate_counter, rate_counter * TIME_ELAPSED_GUI_UPDATE_MS/1000., int_rate);
				rate_counter++;
				c_waveforms1->cd(9);
				gint_rate->Draw("AP");
				gdiff_rate->Draw("PSAME");
				
				if (rate_counter > n_rate_points) {
				  gint_rate->GetXaxis()->SetRangeUser((rate_counter - n_rate_points) * 5, rate_counter * 5);
				}
				gint_rate->GetYaxis()->SetRangeUser(0, int_rate * 2);
				
				c_waveforms1->Modified();
				c_waveforms1->Update();
				c_ene1->Modified();
				c_ene1->Update();
#ifdef V1725
			        c_waveforms2->Modified();
				c_waveforms2->Update();	
				c_ene2->Modified();
				c_ene2->Update();
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





	int ii;
	for (ii = 0; ii < BUF_LENGTH; ii++)
		buf[ii] = 0;

	bd.dry_run = 0;
	bd.handle = -1;
	bd.start_stop = 0;
	out_tree=0;
	out_file=0;

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

	/*Create simple thread just to print out the rate */
	pthread_t RATE_tr;
	pthread_create(&RATE_tr, NULL, rate_fun, NULL);

	pthread_join(RATE_tr, NULL);
	pthread_join(GUI_tr, NULL);
	pthread_join(READ_tr, NULL);


	v1720Close(bd.handle);

	return 1;
}

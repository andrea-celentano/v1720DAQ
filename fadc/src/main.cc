///
/// This  example Fills a ROOT Tree from a EVIO file.
/// It's used for the Forward Tagger Prototype, Genova
///
///
/// usage:
///
/// ./bin/example5 eviofile
/// //////////////////
//**History**////////
/////////////////////
/*
////////////////////
09/02/2011
Starting to work with the first version, filling Root Tree with EVIO.
We have: 1 Multi-Event TDC with 32 channels (CAEN vx1290A) and 2 FADCs with 8 channels each (Caen v1720)

In the EVIO file, the first 2 32-bit words must be neglected!! Then, we have the TDC data, the first FADC, the second FADC.
The order is fixed by the DAQ.
To see when the TDC ends, we check the MSB 4 bits: if they are 1010, we have the FADC Header; TDC can't have 1010 as 4 MSB.
*/

//my headers 
#include "fadc.h"
#include "main.h"
#include "EnergyFunctions.h"


// %%%%%%%%%%%%%
// EVIO headers
// %%%%%%%%%%%%%
#include "evio.h"

// %%%%%%%%%%%%%
// bank headers
// %%%%%%%%%%%%%
#include "bank.h"

// %%%%%%%%%%%%
// ROOT Headers
// %%%%%%%%%%%%
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TApplication.h"


// %%%%%%%%%%%
// C++ headers
// %%%%%%%%%%%
#include <string>
#include <iostream>
#include <cmath>
#include <vector>
using namespace std;

 TApplication gui("gui",0,NULL);





#define FADC_SAMPLES_ENABLE 0


int main(int argc, char **argv)
{
 
	// must have filename as argument
        if((argc != 2)&&(argc !=3))
	{
		cout << endl << endl << " usage: " << endl
		<< "Use: ./main <eviofile> <optional output root file>" << endl << endl;
		cout<< "if output is not provided will be automatically detected"<<endl<<endl;
		return 1; 
	}


	string file     = argv[1];
      	string outname;
	string first,middle,end;
	if (argc==2){
	  int n1=file.find(".",-1);
	  int n2=file.rfind(".",-1);
	  first=file.substr(0,n1);
	  if (n2!=n1){
	     middle=file.substr(n2,file.length()-n2);
	  }
	  end=".root";
	  outname=first+middle+end;
	}
	if (argc==3){
	  outname=argv[2];
	}
	int evn, nhits;
 
  
	//vars to fill the tree
	int tdc[TDC_MULTIPLICITY]; 
	int tdc_id[TDC_MULTIPLICITY];	
	int isfirst=1;
	int hit;
 
	double peak_val[FADC_CH];
	double peak_position[FADC_CH];
	double peak_start[FADC_CH];
	double peak_end[FADC_CH];
	double ped_mean[FADC_CH];
	double ped_sigma[FADC_CH];
	//int fadc[FADC_CH][FADC_SAMPLES];
	
	vector< vector <int> > fadc(FADC_CH,vector<int>(FADC_SAMPLES)); //initialize the vector 
	/*  
	int **fadc=new int*[FADC_CH];
	for (int ii=0;ii<FADC_CH;ii++){
	  fadc[ii]=new int[FADC_SAMPLES];
	}
		*/
	int fadc_samples; //Number of samples per channel.
 
	double energy[FADC_CH];
	double time[FADC_CH];

	double etot;
	double etot_simple;
	//end vars to fill the tree
	
 
	//here goes all the variables that I use to take words from the DAQ and get the relevant infos
	int iword,tdc_chip;
	int tdc_data,tdc_channel;
	int tdc_words;
	int tdc_chip_words[TDC_CHIPS]; //just to control.
 

	int fadc_words;
	int fadc_samples_per_channel;
	int fadc_channel_mask;
	int fadc_active_channels;
	int fadc_data1,fadc_data2;
	int fadc_current_channel,fadc_samples_per_channel_previous;
	iword=0;
	//end of variables for the DAQ
    	
	TFile rf(outname.c_str(),"RECREATE");
		
	TTree *out = new TTree("out", "generated tree");

	/* Here are all the branches for the TTree ouput: */
	/*
	  - Event Number
	  - TDC: id, hit value, number of hits
	  - Pedestal mean (mV) and sigma (mV)
	  - Peak position (ns) and value (mV)
	  - Peak start (ns) and end (ns)
	  - Energy (pC)
	  - Time from fadc
	  
	  -if FADC_SAMPLES_ENABLE, also the FADC samples (and their number), in LSB (not in mV)
	  
	*/

	
	out->Branch("evn",&evn,"evn/I");   //event ID
	out->Branch("TDC_HIT",&hit,"hit/I");
	out->Branch("TDC", tdc,Form("tdc[hit]/I",TDC_MULTIPLICITY));
	out->Branch("TDC_ID", tdc_id,Form("tdc_multiplicity[hit]/I",TDC_MULTIPLICITY));

	out->Branch("PED_MEAN",ped_mean,Form("ped_mean[%i]/D",FADC_CH));
	out->Branch("PED_SIGMA",ped_sigma,Form("ped_sigma[%i]/D",FADC_CH));

	out->Branch("PEAK_POSITION",peak_position,Form("peak_position[%i]/D",FADC_CH));
	out->Branch("PEAK_VAL",peak_val,Form("peak_val[%i]/D",FADC_CH));
	
	out->Branch("PEAK_START",peak_start,Form("peak_start[%i]/D",FADC_CH));
	out->Branch("PEAK_END",peak_end,Form("peak_end[%i]/D",FADC_CH));
	


#ifdef FADC_SAMPLES_ENABLE
	//	out->Branch("FADC",fadc,Form("&(fadc[0][0])/I",FADC_CH,FADC_SAMPLES)); //THIS IS IN LSB!! NOT IN mV!!!
	out->Branch("FADC",&fadc);
	out->Branch("FADC_SAMPLES",&fadc_samples,"fadc_samples/I");
#endif

		out->Branch("ENERGY", energy,Form("energy[%i]/D",FADC_CH));		
		out->Branch("TIME",time,Form("time[%i]/D",FADC_CH));
	
	/*
	  Here I create the FADC Object that is used to do ALL the calculations.
	*/
		fadc_analizer* MyFadc=new fadc_analizer(FADC_BOARDS*FADC_CH_PER_BOARD); //constructor only wants the number of channels
	//MyFadc->Setup(int SamplesPerChannel=200,int PedWidth=60);
	MyFadc->Setup(FADC_SAMPLES,PED_WIDTH);
	MyFadc->PrintEnergyMethods();
	MyFadc->PrintTimeMethods();

	evioFileChannel *chan = new evioFileChannel(file, "r", 3000000);
	chan->open();
 read_event:
	while(chan->read())
	  {
	    
			evioDOMTree EDT(chan);
			Mdgt head(49152, 0, EDT);
			if(head.vars.size()) 
			  {
				evn = head.vars[0];	
				if(evn%1000 == 1) cout << " Event number: " << evn << " " << evn%256 << endl;
			
				Mdgt content(0,evn%256, EDT);
				nhits = content.vars.size();
			        if (nhits) //here starts the important part
				 
				  {
				   
				  //first we read the TDC
				  if (((content.vars[2]>>27)&0x1F)!=0x8){
				    printf("Error in First WORD  event %i\n. Word is %x %x %x \n",evn,content.vars[2],content.vars[3],content.vars[4]);
				    printf("Skipping this iteration going to the next\n");
				    continue; //skip this iteration and goto next
				  }
				  iword=2;
				  tdc_chip=0;
				  tdc_words=0;
				  for (int jj=0;jj<TDC_CHIPS;jj++){
				    tdc_chip_words[jj]=0;
				  }
				 			 
				    for (int kk=0;kk<TDC_MULTIPLICITY;kk++) tdc[kk]=0;
				    hit=0;
				     
				  while(((content.vars[iword]>>28)&0xF)!=0xA){ //1010 is only MSB of FADC
				    if (((content.vars[iword]>>27)&0x1F)==0xF) { //tdc global header
				      tdc_words++;
				    }
				    
				    if (((content.vars[iword]>>27)&0x1F)==0x1){ //tdc chip header
				      tdc_chip=(content.vars[iword]>>24)&0x3;
				      tdc_chip_words[tdc_chip]++;
				      tdc_words++;
				    }
				        
				    if (((content.vars[iword]>>27)&0x1F)==0x0){ //tdc data
				      tdc_data=content.vars[iword]&0x1fffff;
				      tdc_channel=(content.vars[iword]>>21)&0x1F;    
				      tdc[hit]=tdc_data;
				      tdc_id[hit]=tdc_channel;
				      if (tdc_data==0){
					//	printf("%x ch %i %i \n",content.vars[iword],tdc_channel,tdc_multiplicity[tdc_channel]);
					
				      }

				      tdc_chip_words[tdc_chip]++;
				      tdc_words++;				      
				      hit++;
				      
				    }
				  
				    if (((content.vars[iword]>>27)&0x1F)==0x3){ //tdc chip trailer
				      if (tdc_chip!=((content.vars[iword]>>24)&0x3)){
					printf("Error 1 reading TDC event %i CHIP  %i\n",evn,tdc_chip);
					 
					  goto read_event;				  
				      }
				      tdc_chip_words[tdc_chip]++;
				      tdc_words++;
				      if (tdc_chip_words[tdc_chip]!=((content.vars[iword])&0xFFF)){
					printf("Error 2 reading TDC event %i CHIP  %i\n",evn,tdc_chip);
					//printf("a2= %i aa  %x",iword,content.vars[iword]);
				        goto read_event;
				      }
				    }
				    if (((content.vars[iword]>>27)&0x1F)==0x4){ //tdc error
				      tdc_words++;
				    }
				    
				    if (((content.vars[iword]>>27)&0x1F)==0x11){ //tdc Extended Trigger Time TAG
				      tdc_words++;
				    }

				    
				    if (((content.vars[iword]>>27)&0x1F)==0x11){ //tdc Extended Trigger Time TAG
				      tdc_words++;
				    }
				    if (((content.vars[iword]>>27)&0x1F)==0x10){ //tdc Global trailer
				      tdc_words++;
				      if (tdc_words!=((content.vars[iword]>>5)&0xFFFF)){
					printf("Error reading TDC event %i Global Trailer\n",evn);
					 goto read_event;
				      }
				      
				    }
				       
				    iword++;
				 
				  } //end of TDC
				  //Now, content.vars[iword] is the Header of the FIRST FADC/
				
				  for (int kk=0;kk<FADC_BOARDS;kk++){

				    for (int ii=0;ii<FADC_CH_PER_BOARD;ii++){
				      for (int jj=0;jj<FADC_SAMPLES;jj++) fadc[kk*FADC_CH_PER_BOARD+ii][jj]=0;
				    }
				   
				    
				    // FADC START
				  
				    fadc_words=content.vars[iword]&0xFFFFFFF;
				    fadc_words=fadc_words-4; //Number of words containing data.
				    iword++;
				    fadc_channel_mask=content.vars[iword]&0xFF;
				    
				    fadc_active_channels=((fadc_channel_mask>>7)&0x1)+((fadc_channel_mask>>6)&0x1)+((fadc_channel_mask>>5)&0x1)+((fadc_channel_mask>>4)&0x1)+((fadc_channel_mask>>3)&0x1)+((fadc_channel_mask>>2)&0x1)+((fadc_channel_mask>>1)&0x1)+((fadc_channel_mask)&0x1);
				   
				    /*  
				    printf("board %i: %i\n",kk,fadc_words+4);
				    for (int qq=iword-1;qq<(fadc_words+4+iword-1);qq++){
				      printf("%x ",content.vars[qq]);
				    }
				    printf("\n");
				    */
				    /*
				    printf("evn %i board %i\n",evn, kk);
				      printf("tdc words: %i \n",tdc_words);
				      printf("%x \n",content.vars[iword-9]); //tdc end/tdc end	      
				      printf("%x \n",content.vars[iword-8]); 
				      printf("%x \n",content.vars[iword-7]); //tdc end
				      printf("%x \n",content.vars[iword-6]); //tdc end
				      printf("%x \n",content.vars[iword-5]); //tdc end
				      printf("%x \n",content.vars[iword-4]); //tdc end
				      printf("%x \n",content.vars[iword-3]); //tdc end
				      printf("%x \n",content.vars[iword-3]); //tdc end
				      printf("%x \n",content.vars[iword-2]); //tdc end
				      printf("%x \n",content.vars[iword-1]); //fadc header 1
				      printf("%x \n",content.vars[iword]); //fadc chan mask
				      printf("%x \n",content.vars[iword+1]);
				      printf("%x \n",content.vars[iword+2]);
				     
				    */

				    //printf("channels: %i. Mask: %x \n",fadc_active_channels,fadc_channel_mask);
				    fadc_samples_per_channel=2*fadc_words/fadc_active_channels;
				   
				    if ((fadc_samples_per_channel!=fadc_samples_per_channel_previous)&&(isfirst!=1)) {
				      cout<<"error event "<<evn<<" skip it"<<endl;
				      goto read_event;
				    }
				    
				    fadc_samples_per_channel_previous=fadc_samples_per_channel;
				    isfirst=0;

				    //printf("words %i %i\n",fadc_words,fadc_samples_per_channel);
				    
				    iword=iword+3; //first data
				    

				    fadc_current_channel=0;
				    fadc_samples=fadc_samples_per_channel;
				    while((fadc_channel_mask&0x1)!=0x1){
				      fadc_channel_mask=fadc_channel_mask>>1;
				      fadc_current_channel++;
				    }
				   
				    for (int ii=0;ii<fadc_words;ii++){
				     
				    

				      fadc_data1=content.vars[iword]&0xFFF;
				      fadc_data2=(content.vars[iword]>>16)&0xFFF;
				      fadc_data1=-0xFFF+fadc_data1;
				      fadc_data2=-0xFFF+fadc_data2;
				      
				      
				      fadc[kk*FADC_CH_PER_BOARD+fadc_current_channel][2*(ii%(fadc_samples_per_channel/2))]=fadc_data1;
				      fadc[kk*FADC_CH_PER_BOARD+fadc_current_channel][2*(ii%(fadc_samples_per_channel/2))+1]=fadc_data2;
				     
				      //fadc_current_channel=2*ii/fadc_samples_per_channel;
				      
				      
				      //printf("%i %i %i \n",fadc_current_channel,(2*(ii%(fadc_samples_per_channel/2))),(2*(ii%(fadc_samples_per_channel/2))+1));
				      
				      if (((2*(ii%(fadc_samples_per_channel/2))+2)==fadc_samples_per_channel)&&((ii+1)!=fadc_words)){
					fadc_channel_mask=fadc_channel_mask>>1;
					fadc_current_channel++;
					while((fadc_channel_mask&0x1)!=0x1){
					  fadc_channel_mask=fadc_channel_mask>>1;
					  fadc_current_channel++;
					 
					}
				      }
				      
				      
				      iword++; 
				    } 


				  }//FADC END

				  
				  MyFadc->LoadEvent(&fadc);
				  MyFadc->ProcessEvent();
				
				  for (int jj=0;jj<FADC_CH;jj++){
				  
				    ped_mean[jj]=MyFadc->GetPedMean(jj);
				 
				    ped_sigma[jj]=MyFadc->GetPedSigma(jj);
				    peak_position[jj]=MyFadc->GetPeakPosition(jj);
				    peak_val[jj]=MyFadc->GetPeakValue(jj);
				    peak_start[jj]=MyFadc->GetPeakStart(jj);
				    peak_end[jj]=MyFadc->GetPeakEnd(jj);
				    energy[jj]=MyFadc->GetEnergy(jj,0); //0 is the method of integration.
				    time[jj]=MyFadc->GetTime(jj,0);
				    //  cout<<time[jj]<<endl;
				  }

	
				  





				  //now calculate total energy, peak position and so on
				  //USING THE FADC CLASS!!!
				  /*
				    
				  for (int jj=0;jj<FADC_CH;jj++){
				    
		      				    
				    peakPosition[jj]=GetPeakPosition(fadc[jj],peakVal[jj]);				  
				    samplesOverThr[jj]=GetSamplesOverThr(fadc[jj]);		                          
				    energy_simple[jj]=GetEnergySum(fadc[jj],jj);
				   
				    energy[jj]=GetEnergy(fadc[jj],jj,energy_simple[jj],offset[jj],(int)(peakPosition[jj]/4),(pedestal[jj]),(pedestalAfter[jj]));
				  
				    // time[jj]=GetTimeRect(fadc[jj],jj,(int)(peakPosition[jj]/4),energy[jj],&(functions[jj]),FAllocated);
				    if (jj<16) etot_simple+=energy_simple[jj];
				    if (jj<16) etot+=energy[jj];

				  }
				  */

				  // fill tree
				  out->Fill();
				}//end of EVENT

			}
			
	
		}
 end:
		chan->close();
		

	
	rf.Write();
	rf.Close();



}






/****************************************************************/
/*Routine used to calculate the timing of the FT-proto from FADC*/
/* Signal from FADC is fitted using a simple linear function   */
/****************************************************************/
/*
double GetTimeRect(int *fadc,int channel,int peakpos,double energy,TF1 **f,int *Fallocated){
  if ((peakpos<170) || (peakpos>200)) return -2; //se il picco non e' dove deve, ritorna un errore.
 double T[FADC_SAMPLES];
 double efadc[FADC_SAMPLES];
 double dfadc[FADC_SAMPLES];
 double PeakVal=fadc[peakpos];
 for (int ii=0;ii<FADC_SAMPLES;ii++) {T[ii]=ii*4;efadc[ii]=ErrorFADC;dfadc[ii]=(double)fadc[ii];}
 

 if ((channel<16)) { //crystals
 
   TGraphErrors *gr=new TGraphErrors(FADC_SAMPLES,T,dfadc,0,efadc);   
   *f=new TF1("f","pol1",4*(peakpos-NReject-NTime+1)-0.1,4*(peakpos-NReject)+0.1);
   Fallocated[channel]=1;
   TF1 *f1=new TF1("f1","pol0",4*(START_BASE_BEFORE)-0.1,4*(START_BASE_BEFORE+WIDTH_ENERGY)+0.1);
   
   //now FIT. r1 and r2 are fit results. If r==0, OK. Else error!
   int r2=gr->Fit("f1","RQ","GOFF");
   int r1=gr->Fit("f","RQ","GOFF",4*(peakpos-NReject-NTime+1)-0.1,4*(peakpos-NReject)+0.1);   

   
   double a=(*f)->GetParameter(1);
   double b=(*f)->GetParameter(0);
   double pedestal=f1->GetParameter(0);
  
 
   delete f1;
   delete gr;


   double val=pedestal+(PeakVal-pedestal)*CFraction;
   double t=val/a-(b/a);
   
   if (fabs(PeakVal-pedestal)<40) return -3;
  
  // if (channel==8){
  //     TCanvas *c=new TCanvas("c","c",700,700);
  //     f->SetLineColor(2);
  //     f1->SetLineColor(3);
  //     gr->Draw("AP");
  //     f->Draw("SAME");
  //     f1->Draw("SAME");   
  //     gr->GetXaxis()->SetRangeUser(620,800);
  //     c->SetGridx();
  //     c->SetGridy();
  //     c->Modified();
  //     c->Update();   
      
  //     cout<<t<<endl;
   //    cin.get();
  // }
   
 
   if ((t>600) && (t<800) && (r1==0)) return t;
   else if ((r1!=0)) return -1;
   else return -2;
   

 }

 else return 0;



}
*/

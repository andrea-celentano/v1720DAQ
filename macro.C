/*Samples has to be splitted also (32bits->2 data)*/

#define MAX 4096 
#define MAX_S 1024
#define Nch 8

#include <iostream>
#include <fstream>
#include "TFile.h"
#include "TH1D.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TGraph.h"
#include "TF1.h"
#include "TApplication.h"
#include <string>

using namespace std;

//This is the function used to fit the signal shapes 
double wave(double *x,double *par){ 
  double Volt;
  double t=x[0]; 
  double t0=par[0];
  double C=par[1];
  double sigma=par[2];
  double tau=par[3];
  double tau1=par[4];
  double n=par[5];
  double D=par[6];
  
  if (t<t0){
    Volt=C*exp(-(t-t0)*(t-t0)/(2*sigma*sigma))+D;
  }
  else if (t<t0+n*tau) {
    Volt=C*exp(-t/tau)*exp(t0/tau)+D;
    }
    else {
    Volt=C*exp(-n);
    Volt=Volt*exp(-t/(tau1))*exp((t0+n*tau)/(tau1))+D;
  }
  return Volt;
}



//TApplication gui("gui",0,NULL);



double sumSamples(int n,double *x){
  double a=0;
  for (int ii=0;ii<n;ii++) {
    a+=x[ii];
  }
  return a;

}


main(int argc,char **argv){
  if (argc!=2){
    cout<<"manca nome file"<<endl;
    exit(1);
  }

  string name=argv[1];
  string oname;
  oname=strcat(argv[1],"_data.root");
  TFile f(name.c_str(),"update");
  TTree *t=(TTree *)f.Get("out");
  int n=t->GetEntries();
  UInt_t ptime;
  UInt_t dtime;
  UInt_t plength,dlength;
  UInt_t pchmask,dchmask;
  UInt_t Tpchmask,Tdchmask;
  UInt_t psamples[MAX];
  UInt_t dsamples[MAX];

  int n_words_per_channel;
  double x[MAX_S];
  for (int ii=0;ii<MAX_S;ii++){
    x[ii]=ii*4;
  }
  int n_active;





  double pdata[Nch][MAX_S];
  double ddata[Nch][MAX_S];


  t->SetBranchAddress("delay_time",&dtime);
  t->SetBranchAddress("prompt_time",&ptime);
  t->SetBranchAddress("prompt_length",&plength);  //TOTAL NUMBER OF 32-bit words with data
  t->SetBranchAddress("delay_length",&dlength);
  t->SetBranchAddress("prompt_channel_mask",&pchmask);
  t->SetBranchAddress("delay_channel_mask",&dchmask);
  t->SetBranchAddress("prompt_samples",psamples);
  t->SetBranchAddress("delay_samples",dsamples);

  double penergy[Nch];
  double denergy[Nch];

  TFile *f1=new TFile(oname.c_str(),"recreate");
  TTree *t1=new TTree("out_add","out_add");
  t1->Branch("prompt_energy",&penergy,Form("penergy[%i]/D",Nch));
  t1->Branch("delay_energy",&denergy,Form("denergy[%i]/D",Nch));
 


  
   TF1 *fun=new TF1("fun",wave,0,4000,7);
   TGraph *grP,*grD;

   int nmin;
   double Xmax,Ymax;
  //min and max time.
   double min,max;




  /*Let's analize */
  t1->Fill(); //first event is bad
  for (int ii=1;ii<n;ii++){ //first event is bad
    t->GetEntry(ii);
    if ((ii%1000)==0) cout<<"ANA EV "<<ii<<endl;
    if (plength!=dlength) {
      cout<<"error in length data "<<ii<<endl;
      exit(1);
    }
    if (pchmask!=dchmask){
      cout<<"error in chmask data "<<ii<<endl;
    }
    Tdchmask=dchmask;
    Tpchmask=pchmask;
    n_active=0;
    for (int jj=0;jj<Nch;jj++){
      n_active+=(Tdchmask>>jj)&0x1;
    }

    
    n_words_per_channel=plength/n_active; //number of 32-bits words per channel
    int counter=0;				
   
    for (int jj=0;jj<Nch;jj++){
    penergy[jj]=0.;
    denergy[jj]=0.;
    }

    for (int jj=0;jj<Nch;jj++){
      Tdchmask=dchmask;
      Tpchmask=pchmask;
      if ((Tdchmask>>jj)&0x1==0x1){ //this channel is present in data
	for (int kk=0;kk<n_words_per_channel;kk++){
	  pdata[jj][2*kk]=(double)(psamples[kk+counter*n_words_per_channel]&0xfff);
	  pdata[jj][2*kk+1]=(double)((psamples[kk+counter*n_words_per_channel]>>16)&0xfff);
	ddata[jj][2*kk]=(double)(dsamples[kk+counter*n_words_per_channel]&0xfff);
      ddata[jj][2*kk+1]=(double)((dsamples[kk+counter*n_words_per_channel]>>16)&0xfff);

	  ddata[jj][2*kk]=2000*(1-ddata[jj][2*kk]/4096);
	  ddata[jj][2*kk+1]=2000*(1-ddata[jj][2*kk+1]/4096);
	  pdata[jj][2*kk]=2000*(1-pdata[jj][2*kk]/4096);
	  pdata[jj][2*kk+1]=2000*(1-pdata[jj][2*kk+1]/4096);
	}
	counter++;
      
     
     
   



    /*START CALCULATING ENERGY ch jj*/

      //prompt
      grP=new TGraph(2*n_words_per_channel,x,pdata[jj]);
      nmin=TMath::LocMax(n_words_per_channel*2,grP->GetY());
      Xmax=grP->GetX()[nmin];
      Ymax=grP->GetY()[nmin];
      min=grP->GetX()[0];
       max=grP->GetX()[n_words_per_channel*2-1];
       grP->SetMarkerStyle(7);

       TCanvas c1;
     
    


      fun->SetParameter(0,Xmax);
      fun->FixParameter(1,Ymax);
      fun->SetParameter(2,1); //sigma gaussiana
      fun->SetParameter(3,6); //tau first exp
      fun->SetParameter(4,6); //tau second exp
      fun->SetParameter(5,4); //to switch between first and second exp
      fun->SetParLimits(5,1,4); 
      fun->SetParameter(6,1); //costant
      fun->SetNpx(1000);
      int resP;
      //grP->Fit("fun","QRB0","",min,max);
      //   resP=grP->Fit("fun","QRB0","",min,max);
      resP=0;
     
      if (resP!=0){
	//	grP->Draw("AP");
	//fun->Draw("SAME");
	//c1.Modified();
	//c1.Update();
	//c1.Draw();
	//cin.get();
	penergy[jj]=-1;
      }
      else{
	penergy[jj]=sumSamples(grP->GetN(),grP->GetY());
	fun->SetParameter(6,0);
	//penergy[jj]=fun->Integral(min,max);
      }

      //delay

      grD=new TGraph(2*n_words_per_channel,x,ddata[jj]);
      nmin=TMath::LocMax(n_words_per_channel*2,grD->GetY());
      Xmax=grD->GetX()[nmin];
      Ymax=grD->GetY()[nmin];
      min=grD->GetX()[0];
      max=grD->GetX()[n_words_per_channel*2-1];
      
      grD->SetMarkerStyle(7);
     
      


      fun->SetParameter(0,Xmax);
      fun->FixParameter(1,Ymax);
      fun->SetParameter(2,1); //sigma gaussiana
      fun->SetParameter(3,6); //tau first exp
      fun->SetParameter(4,6); //tau second exp
      fun->SetParameter(5,4); //to switch between first and second exp
      fun->SetParLimits(5,1,4); 
      fun->SetParameter(6,1); //costant
      fun->SetNpx(1000);

      int resD;
      //grD->Fit("fun","QR0B","",min,max);
      //resD=grD->Fit("fun","QBR0","",min,max);
      resD=0;
      if (resD!=0){
	//grD->Draw("AP");
	//fun->Draw("SAME");
	//c1.Update();
	//c1.Modified();


	//cin.get();
      denergy[jj]=-1;
      }

      else{
	fun->SetParameter(6,0);
	denergy[jj]=sumSamples(grD->GetN(),grD->GetY());
	//denergy[jj]=fun->Integral(min,max);
      }      
      //cout<<"Pen: "<<penergy[jj]<<"Den: "<<denergy[jj]<<endl;
      



     
    /*END CALCULATING ENERGY ch jj*/
      }
    }
    // cout<<"Fill"<<endl;
    t1->Fill();
  }

  cout<<"Start to save"<<endl;
  
  f1->Write();
  f1->Close();
 




  f.cd();
  t->AddFriend("out_add",oname.c_str());

  t->Write();
  f.Write();
  f.Close();
 



 
}

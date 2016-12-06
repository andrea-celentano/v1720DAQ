 
/*	TTree *out = new TTree("out", "generated tree");
	out->Branch("evn",   &evn,   "evn/I");   //event ID
	out->Branch("nhits", &nhits, "nhits/I"); //number of 32-bit words from EVIO data in this event
	out->Branch("TDC", tdc,Form("tdc[%i][%i]/I",TDC_CH,TDC_MULTIPLICITY));
	out->Branch("TDCMULTIPLICITY", tdc_multiplicity,Form("tdc_multiplicity[%i]/I",TDC_CH));
	out->Branch("FADC", fadc,Form("fadc[%i][%i]/I",FADC_CH,FADC_SAMPLES));
	out->Branch("FADC_SAMPLES",&fadc_samples,"fadc_samples/I");
	out->Branch("FADC_SIMPLE_ENERGY",fadc_simple_energy,Form("fadc_simple_energy[%i]/I",FADC_CH));
*/

#include <iostream>
#include <string>
#include "TFile.h"
#include "TApplication.h"
#include "TH1D.h"
#include "TGraph.h"
#include "TF1.h"
#include <stdio.h>
#include <stdlib.h>
#include "TTree.h"
#include <cmath>
#include "TMath.h"
#include "TCanvas.h"

using namespace std;
TApplication gui("gui",0,NULL);


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

main(int argc, char **argv){
  if (argc!=2){
    printf("Use ./ana <nome_file> \n");
    exit(1);
  }
  
  TFile f(argv[1],"update");
  TTree *t=(TTree *)f.Get("out");
  int N=t->GetEntries();

  
  //Here i define the vars that i fill using the Tree informations (FADC)
  int fadc[16][1024];
  int fadc_samples;
  int fadc_simple_energy[16];
  //The graph that i fill 
  TGraph *gr;
  //The function to fit 
  TF1 *fun=new TF1("fun",wave,0,4000,7);
  //Location of Maximum of the waveform and its value
  int nmin;
  double Xmax,Ymax;
  //min and max time.
  double min,max;


  //Connect the TREE
  t->SetBranchAddress("FADC",fadc);
  t->SetBranchAddress("FADC_SAMPLES",&fadc_samples);
  t->SetBranchAddress("FADC_SIMPLE_ENERGY",fadc_simple_energy);

  //Crete the new tree and the new brah for energy from FADC
  //Let's save new info in a new TFILE also
  double fadc_energy[16];
  string new_name=argv[1];
  new_name=new_name.substr(0,strlen(argv[1])-5);
  new_name=new_name+"_ana.root";
  cout<<"Writing energy data in "<<new_name<<endl;

  TFile *f1=new TFile(new_name.c_str(),"recreate");
 
  TTree *new_tree=new TTree("out_add","out_add");
  TBranch *branch_energy=new_tree->Branch("FADC_ENERGY",fadc_energy,"fadc_energy[16]/D");
  //  branch_energy->SetFile(f1);

  TCanvas c1;

  for (int ii=0;ii<N;ii++){
    //if ((ii%1000)==0) cout<<"Processing event "<<ii<<endl;
    if ((ii%100)==0) cout<<"Processing event "<<ii<<endl;
    for (int jj=0;jj<16;jj++) fadc_energy[jj]=0;
    t->GetEntry(ii);
    for (int jj=0;jj<6;jj++){      //SONO I PMT
      gr=new TGraph(0);
      for (int kk=0;kk<fadc_samples;kk++){
	gr->SetPoint(kk,kk*4,fadc[jj+10][kk]);	
      }
      nmin=TMath::LocMax(fadc_samples,gr->GetY());
      Xmax=gr->GetX()[nmin];
      Ymax=gr->GetY()[nmin];
      min=gr->GetX()[0];
      max=gr->GetX()[fadc_samples-1];


  
      gr->Draw("AP");
      //now i set the parameters
      fun->SetParameter(0,Xmax);
      fun->FixParameter(1,Ymax);
      fun->SetParameter(2,1); //sigma gaussiana
      fun->SetParameter(3,6); //tau first exp
      fun->SetParameter(4,6); //tau second exp
      fun->SetParameter(5,4); //to switch between first and second exp
      fun->SetParLimits(5,1,4); 
      fun->SetParameter(6,1); //costant
      fun->SetNpx(1000);

      //gr->Fit("fun","RQB","",min,max);
      //gr->Fit("fun","RBQ","",min,max);
      // c1.Update();
      // c1.Draw();
     
      //cin.get();
      
     

     

      //now i calculate the integral
      fun->SetParameter(6,0); 
      //fadc_energy[jj+10]=fun->Integral(min,max);
      //cout<<fadc_energy[jj+10]<<" "<<fadc_simple_energy[jj+10]*4<<endl;

      /*if (fabs(fadc_energy[jj+10]-fadc_simple_energy[jj+10]*4)>10000){
	cout<<fadc_energy[jj+10]<<" "<<fadc_simple_energy[jj+10]*4<<endl;
	c1.Update();
	c1.Draw();
	cin.get();
      }
      */


 
      //cin.get();
      
      //gui.Run(1);
    }
    new_tree->Fill();
  }
  f1->Write();
  f1->Close();

  f.cd();
  t->AddFriend("out_add",new_name.c_str());
  t->Write(); 
  f.Write();
  f.Close();
  //f.Write("",TObject::kOverwrite);
  
  cout<<"ANALYSIS DONE"<<endl;
}
    


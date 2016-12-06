#include "fadc.h"
#include "EnergyFunctions.h"
#include "TimeFunctions.h"
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
using namespace std;

//energy calculator main class
fadc_analizer::energy_calculator::energy_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description):
m_name(name),
m_description(description),
m_fadc(o_fadc),
m_calibration(vector < double > (m_fadc.m_Nchannels,1.)) //init all the calibration constants to 1 (one for each channel!)
{};

/*This is very important: its the initialization of the function pointer for the energy calculator.
  Parameters are the following (BY DEFINITION, YOU CAN'T CHANGE THEM):
  0 -> ped mean in mV
  1 -> ped sigma in mV
  2 -> peak val in mV
  3 -> peak position in SAMPLES (NOT ns)
  4 -> integration start in SAMPLES (NOT ns)
  5 -> integration end in SAMPLES (NOT ns)
  6 -> samples per channel
*/
int fadc_analizer::energy_calculator::SetFunction(double (*fun)(int *points,double *par)){
  m_function=fun;
}


//this is just a "wrapper"
double fadc_analizer::energy_calculator::CalculateEnergy(int ch){
  double *par=new double[7];
  par[0]=m_fadc.m_ped_mean[ch];
  par[1]=m_fadc.m_ped_sigma[ch];
  par[2]=m_fadc.m_peak_val[ch];
  par[3]=m_fadc.m_peak_pos[ch]/m_fadc.dT;
  par[4]=m_fadc.m_peak_start[ch]/m_fadc.dT;
  par[5]=m_fadc.m_peak_end[ch]/m_fadc.dT;
  par[6]=m_fadc.m_SamplesPerChannel;


  energy=m_function(&(m_fadc.points->at(ch)[0]),par);
  /*This is the calibration part */
  if (m_calibration[ch]!=0) energy=energy/m_calibration[ch];
  delete par;
  return energy;
}
int fadc_analizer::energy_calculator::SetCalConstant(int ch,double o_constant){
  if ((ch<0)||(ch>=m_fadc.m_Nchannels)){
    cerr<<"Error in fadc_analizer::energy_calculator::SetCalConstants"<<endl;
    return -1;
  }
  if (o_constant==0){
    cerr<<"Error in fadc_analizer::energy_calculator::SetCalConstants"<<endl;
    return -2;
    
  }
  m_calibration[ch]=o_constant;

}



int fadc_analizer::energy_calculator::SetCalConstants(vector <double> o_calibration){
  if (o_calibration.size()!=m_fadc.m_Nchannels){
    cerr<<"Error in fadc_analizer::energy_calculator::SetCalConstants"<<endl;
    return -1;
  }
  m_calibration=o_calibration;
  return 0;
}

int fadc_analizer::energy_calculator::SetCalConstants(double *o_calibration){
  if(!o_calibration){
    cerr<<"Error in fadc_analizer::energy_calculator::SetCalConstants"<<endl;
    return -1;
  }
  for (int ii=0;ii<m_fadc.m_Nchannels;ii++){
    m_calibration[ii]=o_calibration[ii];
  } 
}



fadc_analizer::time_calculator::time_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description):
m_name(name),
m_description(description),
m_fadc(o_fadc)
{};


/*This is very important: its the initialization of the function pointer for the time calculator.
  Parameters are the following (BY DEFINITION, YOU CAN'T CHANGE THEM):
  0 -> ped mean in mV
  1 -> ped sigma in mV
  2 -> peak val in mV
  3 -> peak position in SAMPLES (NOT ns)
  4 -> integration start in SAMPLES (NOT ns)
  5 -> integration end in SAMPLES (NOT ns)
  6 -> Number of samples
*/
int fadc_analizer::time_calculator::SetFunction(double (*fun)(int *points,double *par)){
  m_function=fun;
}


//this is just a "wrapper"
double fadc_analizer::time_calculator::CalculateTime(int ch){
  double *par=new double[7];
  par[0]=m_fadc.m_ped_mean[ch];
  par[1]=m_fadc.m_ped_sigma[ch];
  par[2]=m_fadc.m_peak_val[ch];
  par[3]=m_fadc.m_peak_pos[ch]/m_fadc.dT;
  par[4]=m_fadc.m_peak_start[ch]/m_fadc.dT;
  par[5]=m_fadc.m_peak_end[ch]/m_fadc.dT;
  par[6]=m_fadc.m_SamplesPerChannel;
  
  time=m_function(&(m_fadc.points->at(ch)[0]),par);
  delete par;
  return time;
}


















//here is the fadc stuff.

/*The constructor with its variable initializer list*/

fadc_analizer::fadc_analizer(int Nchannels):
  m_Nchannels(Nchannels),
  m_setup_done(1), //not yet done
  m_ped_width(0),
  m_SamplesPerChannel(0),
  /*Put here are the initialization for the calculators you want. Change only the name!*/
  ECalculator1(*this,"GetEnergySumV2","Summing the value of bins but in a event-by-event determined interval"), //initializer for creator of calculator1.
  ECalculator2(*this,"GetEnergySum","Summing the value of bins in a fixed interval centered on peak position"), //initializer for creator of calculator1.
  TCalculator1(*this,"GetTimeFADC250Firmware","The same function in the FADC250 Firmware") //initializer for creator of calculator1.
{
  m_ped_mean=NULL;
  m_ped_sigma=NULL;
  m_peak_pos=NULL;
  m_peak_val=NULL;
  m_peak_start=NULL;
  m_peak_end=NULL;
};



/*The destructor*/
fadc_analizer::~fadc_analizer(){
  if (m_ped_mean) delete m_ped_mean;
  if (m_ped_sigma) delete m_ped_sigma;
  if (m_peak_pos) delete m_peak_pos;
  if (m_peak_val) delete m_peak_val;

  if (m_peak_start) delete m_peak_start;
  if (m_peak_end) delete m_peak_end;
  
}

void fadc_analizer::Setup(int SamplesPerChannel,int PedWidth){
  
  
  
  if (!m_ped_mean) m_ped_mean=new double[m_Nchannels];
  if (!m_ped_sigma) m_ped_sigma=new double[m_Nchannels];
  if (!m_peak_pos) m_peak_pos=new double[m_Nchannels];
  if (!m_peak_val) m_peak_val=new double[m_Nchannels];
  if (!m_peak_start) m_peak_start=new double[m_Nchannels];
  if (!m_peak_end) m_peak_end=new double[m_Nchannels];


  m_SamplesPerChannel=SamplesPerChannel; 
  m_ped_width=PedWidth; 


  //here goes the setup of the energy calculators!
  //functions are defined in EnergyFunctions.cc
  ECalculator1.SetFunction(GetEnergySumv2);
  //put all in the vector
  m_energy_calculators.push_back(&ECalculator1);

  ECalculator2.SetFunction(GetEnergySum);
  m_energy_calculators.push_back(&ECalculator2);

  //here goes the setup of the time calculators!
  //functions are defined in TimeFunctions.cc
  TCalculator1.SetFunction(GetTimeFADC250Firmware);
  //put all in the vector
  m_time_calculators.push_back(&TCalculator1);

  m_setup_done=0;
};


/*Function used to load the event data
IN: int **samples: the fadc samples as a "matrix". First int is ch, second is samples
OUT: the status 
*/

int fadc_analizer::LoadEvent(vector < vector <int> > *samples){
  if (!samples) return -1;
  points=samples;
  return 0;
}
/*Function used to load the event data
IN: nothing
OUT: the status 
*/

int fadc_analizer::ProcessEvent(){
  int ret=0;
  for (int ii=0;ii<m_Nchannels;ii++){
    ret+=CalculatePedestal(ii);
    ret+=CalculatePeak(ii);
    ret+=CalculatePeakLimits(ii);
  }
  return ret; //should be 0 if fine.
};


/*Function used to calculate the pedestal mean and sigma in mV 
IN: the channel to calculate
OUT: the status of the calculation (0: ok, -1 error)
*/

int fadc_analizer::CalculatePedestal(int ch){   
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  }
  if (m_ped_width<=0) {
    return -3;
  }
  m_ped_mean[ch]=0;
  m_ped_sigma[ch]=0;
  for(int ii=0;ii<m_ped_width;ii++){
    m_ped_mean[ch]+=points->at(ch)[ii];
    m_ped_sigma[ch]+=((points->at(ch)[ii])*(points->at(ch)[ii]));
  }
  m_ped_mean[ch]=LSB*m_ped_mean[ch]/m_ped_width; //OK
  m_ped_sigma[ch]=LSB*LSB*m_ped_sigma[ch]/m_ped_width; //this is the mean square value!

  m_ped_sigma[ch]=sqrt(m_ped_sigma[ch]-m_ped_mean[ch]*m_ped_mean[ch]);


  return 0;
};

/*Function used to calculate the peak position in ns and the peak value in mV
IN: the channel to calculate
OUT: the status of the calculation (0: ok, -1 error)
*/
int fadc_analizer::CalculatePeak(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  
  
  m_peak_val[ch]=(int)((-3*m_ped_sigma[ch]+m_ped_mean[ch])/fadc_analizer::LSB);
  m_peak_pos[ch]=0;
  
  for (int ii=0;ii<m_SamplesPerChannel;ii++){
    if(points->at(ch)[ii]<=m_peak_val[ch]){
      m_peak_val[ch]=points->at(ch)[ii];
      /*if(min<(-sigma_pedest)+mean_pedest)*/ m_peak_pos[ch]=ii*dT;
    }
  }
  m_peak_val[ch]=m_peak_val[ch]*LSB-m_ped_mean[ch]; 
  m_peak_val[ch]=m_peak_val[ch]*(-1);//since signals are negative!
  return 0;
};


/*Function used to calculate the peak start and end in ns
IN: the channel to calculate
OUT: the status of the calculation (0: ok, -1 error)
*/
int fadc_analizer::CalculatePeakLimits(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  

  if(m_peak_pos[ch]<=0){
    //to be consistent with F.Cipro Macro!
    m_peak_start[ch]=0;
    m_peak_end[ch]=4; 
    return -3;
  }
  //1: init them at the peak position IN SAMPLES (so I divide by dT)
  m_peak_start[ch]=(int)(m_peak_pos[ch]/dT);
  m_peak_end[ch]=(int)(m_peak_pos[ch]/dT);
 
    //2: the calculation: move start back until we reach the pedestal mean
    do {m_peak_start[ch]--;}
    while (points->at(ch)[(int)m_peak_start[ch]]<m_ped_mean[ch]/fadc_analizer::LSB);
    //3: the calculatino: move end forward until we reach the pedestal mean
    do {m_peak_end[ch]++;}
    while (points->at(ch)[(int)m_peak_end[ch]]<m_ped_mean[ch]/fadc_analizer::LSB);
    
  

  //4: a correction
  m_peak_start[ch]++;
  m_peak_end[ch]--;
  
  //5: lets check we are within limits!
  if (m_peak_start[ch]<0) m_peak_start[ch]=0;
  if (m_peak_end[ch]>=m_SamplesPerChannel) m_peak_end[ch]=m_SamplesPerChannel-1;
    
  m_peak_start[ch]*=dT;
  m_peak_end[ch]*=dT;
  
  
  return 0;
  
};


/*Here goes the functions that get back variables to the user*/

double fadc_analizer::GetPedMean(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_ped_mean) {
    return -3;
  }
  return m_ped_mean[ch];  
}

double fadc_analizer::GetPedSigma(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_ped_sigma) {
    return -3;
  }
  return m_ped_sigma[ch];  
}


double fadc_analizer::GetPeakPosition(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_peak_pos) {
    return -3;
  }
  return m_peak_pos[ch];  
}

double fadc_analizer::GetPeakValue(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_peak_val) {
    return -3;
  }
  return m_peak_val[ch];  
}

double fadc_analizer::GetPeakStart(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_peak_start) {
    return -3;
  }
  return m_peak_start[ch];  
}
double fadc_analizer::GetPeakEnd(int ch){
  if (m_setup_done!=0) return -1;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2;
  } 
  if (!m_peak_end) {
    return -3;
  }
  return m_peak_end[ch];
}


double* fadc_analizer::GetPedMean(){
  if (m_setup_done!=0) return NULL;
 
  if (!m_ped_mean) {
    return NULL;
  }
  return m_ped_mean;  
}

double* fadc_analizer::GetPedSigma(){
  if (m_setup_done!=0) return NULL;
 
  if (!m_ped_sigma) {
    return NULL;
  }
  return m_ped_sigma;  
}


double* fadc_analizer::GetPeakPosition(){
  if (m_setup_done!=0) return NULL;
  
  if (!m_peak_pos) {
    return NULL;
  }
  return m_peak_pos;  
}

double* fadc_analizer::GetPeakValue(){
  if (m_setup_done!=0) return NULL;

  if (!m_peak_val) {
    return NULL;
  }
  return m_peak_val;  
}

double* fadc_analizer::GetPeakStart(){
  if (m_setup_done!=0) return NULL;

  if (!m_peak_start) {
    return NULL;
  }
  return m_peak_start;  
}
double* fadc_analizer::GetPeakEnd(){
  if (m_setup_done!=0) return NULL;
  if (!m_peak_end) {
    return NULL;
  }
  return m_peak_end;
}























/*Here are the more complicated get-back functions for the energy.
1) Check if the chosen EnergyCalculator exists and is correctly initialized
2) Use its methods to calculate the energy
3) Get it back
*/

double fadc_analizer::GetEnergy(int ch,int method){
  if (m_setup_done!=0) return -1000;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2000;
  } 
  if ((method<0)||(method>=m_energy_calculators.size())){
    return -3000;
  }
  
  //CELE checks here the correctness of the calculator we want to use
  return m_energy_calculators[method]->CalculateEnergy(ch);
}






void fadc_analizer::PrintEnergyMethods(){
  cout<<"Here are the available energy calculators: "<<endl;
  for (int ii=0;ii<m_energy_calculators.size();ii++){
    cout<<ii<<" : "<<m_energy_calculators[ii]->GetName()<<" --> "<<m_energy_calculators[ii]->GetDescription()<<endl;
  }

}


int fadc_analizer::LoadCalConstants(int method,string o_fname){
  if ((method<0)||(method>=m_energy_calculators.size())){
	cerr<<"Error in fadc_analizer::LoadCalConstants wrong calculator index"<<endl;
	return -1;
  }
  ifstream file(o_fname.c_str());
  if (!file){
	cerr<<"Error in fadc_analizer::LoadCalConstants wrong file"<<endl;
	return -2;
  }
  double dCalcConstant=0;
  for (int ii=0;ii<m_Nchannels;ii++) {
    file>>dCalcConstant;
    m_energy_calculators[method]->SetCalConstant(ii,dCalcConstant);					 
  }
}





/*Here are the more complicated get-back functions for the time
1) Check if the chosen TimeCalculator exists and is correctly initialized
2) Use its methods to calculate the time
3) Get it back
*/

double fadc_analizer::GetTime(int ch,int method){
  if (m_setup_done!=0) return -1000;
  if ((ch<0) || (ch>=m_Nchannels)){
    return -2000;
  } 
  if ((method<0)||(method>=m_time_calculators.size())){
    return -3000;
  }
  
  //CELE checks here the correctness of the calculator we want to use
  return m_time_calculators[method]->CalculateTime(ch);
}



void fadc_analizer::PrintTimeMethods(){
  cout<<"Here are the available time calculators: ( "<<m_time_calculators.size()<<" )"<<endl;
  for (int ii=0;ii<m_time_calculators.size();ii++){
    cout<<ii<<" : "<<m_time_calculators[ii]->GetName()<<" --> "<<m_time_calculators[ii]->GetDescription()<<endl;
  }

}

#include "TimeFunctions.h"
#include "fadc.h"
#include <iostream>
/* In this file I put All the functions used to calculate the time for a channel

Don't care about the structure: they are always
double function(unsigned short *fadc,double *par).

fadc --> array of fadc data (for 1 channel!) in "bits", not in mV
par --> some parameters already calculated. 
       0 ped mean in mV
       1 ped sigma in mV
       2 peak val in mV (positive, pedestal subtracted!)
       3 peak position in SAMPLES
       4 integration start in SAMPLES
       5 integration end in SAMPLES
       6 number of total samples (integer)
*/


/*If YOU need some "costants" (i.e. #define xxx) please put it in TimeFunctions.h*/

 
//This is the same functions that is implemented now in the FADC250

double GetTimeFADC250Firmware(unsigned short *fadc, double *par){
  
  double energy_tot=0;
  
    double& ped_mean=par[0];
      
    double& ped_sigma=par[1];
    
  double& peak_val=par[2];
  double& peak_pos=par[3];
  /* 
     double& peak_start=par[4];
     double& peak_end=par[5];
  */
  
  int index=0;
  double vmin;
  double vmean;
  double tcoarse;
  double tfine;
  double vbp,vap;

 
  //a simple check to avoid stupid errors
  if (peak_val<2*ped_sigma){
    return -1;
  }

  //0 calculate the pedestal: use ped mean
  //1 find the maximum value: use peak val and peak pos
  
  //2 find the mean value of the pulse (in mV)
  vmean=(-peak_val)/2+ped_mean;
 
  //3 search for the samples before and after vmean.
  //4 search for coarse time
  for (int ii=peak_pos;ii>0;ii--){

    

    if (((fadc[ii]*(fadc_analizer::LSB))>vmean)&&((fadc[ii+1]*(fadc_analizer::LSB))<vmean)){
      tcoarse=(double)ii;
      vbp=(double)fadc[ii]*fadc_analizer::LSB;
      vap=(double)fadc[ii+1]*fadc_analizer::LSB;
      break;      
    }
  }
  if (vbp==vap) return -2;
  //5 calculate fine time
  tfine=4*(vmean-vbp)/(vap-vbp); //from 0 to 4 ns
  return (tcoarse*4)+tfine;
}

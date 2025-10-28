#include "EnergyFunctions.h"
#include "fadc.h"
#include <iostream>
/* In this file I put All the functions used to calculate the energy for a channel

Don't care about the structure: they are always
double function(unsigned short *fadc,double *par).

fadc --> array of fadc data (for 1 channel!) in "bits", NOT in mV
par --> some parameters already calculated. 
       0 ped mean in samples
       1 ped sigma in samples
       2 peak val in samples
       3 peak position in samples
       4 integration start in samples
       5 integration end in samples
       6 number of samples
*/


/*If YOU need some "costants" (i.e. #define xxx) please put it in EnergyFunctions.h*/

double GetEnergySumv2(unsigned short *fadc, double *par){
  
  double energy_tot=0;
  double& ped_mean=par[0];
  double& ped_sigma=par[1];
  double& peak_val=par[2];
  double& peak_pos=par[3];
  double& peak_start=par[4];
  double& peak_end=par[5];

 
  int nintegrated=0;
  if(peak_pos>START_ENERGY_V2 && peak_pos<(START_ENERGY_V2+WIDTH_ENERGY_V2) && (peak_end-peak_start)>MIN_WIDTH){
    for(int ii=(int)peak_start;ii<=(int)peak_end;ii++){
      energy_tot+=(double)fadc[ii];
      nintegrated++;
     }  
    energy_tot=-energy_tot+(ped_mean)*(nintegrated);  
  }
  else {
    energy_tot=-5000;
  }
  return energy_tot;
}

/*Fixed integration window centered on the peak position*/
double GetEnergySum(unsigned short *fadc, double *par){
  
  double energy_tot=0;
  double& ped_mean=par[0];
  double& ped_sigma=par[1];
  double& peak_val=par[2];
  double& peak_pos=par[3];
  double& peak_start=par[4];
  double& peak_end=par[5];
  double& nsamples=par[6];
 
  int nintegrated=0;
 
  for(int ii=0;ii<PRE_SAMPLES;ii++){
    if ((peak_pos-ii)<0) break;
    energy_tot+=(double)fadc[(int)peak_pos-ii];
    nintegrated++;
	}  

  for(int ii=0;ii<POST_SAMPLES;ii++){
    if ((peak_pos+ii+1) >= nsamples) break;
    energy_tot+=(double)fadc[(int)peak_pos+ii+1];
    nintegrated++;  
}
 
  energy_tot=-energy_tot+(ped_mean)*(nintegrated);
  return energy_tot;
}

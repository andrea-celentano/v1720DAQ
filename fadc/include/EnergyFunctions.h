#ifndef ENERGY_FUNCTION_HGUARD
#define ENERGY_FUNCTION_HGUARD



//for GetEnergySumv2. THESE are in samples
#define START_ENERGY_V2 100 /*A. Celentano 12/12/2013. Lets took out here the peak position cut.*/
#define WIDTH_ENERGY_V2 175
#define MIN_WIDTH 5 /*In samples */

//GetEnergySum
#define PRE_SAMPLES 5
#define POST_SAMPLES 10

double GetEnergySumv2(int *fadc,double *par);
double GetEnergySum(int *fadc,double *par);


#endif

#ifndef ENERGY_FUNCTION_HGUARD
#define ENERGY_FUNCTION_HGUARD



//for GetEnergySumv2. THESE are in samples
#define START_ENERGY_V2 40 
#define WIDTH_ENERGY_V2 40
#define MIN_WIDTH 5 /*In samples */

//GetEnergySum
#define PRE_SAMPLES 5
#define POST_SAMPLES 10

double GetEnergySumv2(unsigned short *fadc,double *par);
double GetEnergySum(unsigned short *fadc,double *par);


#endif

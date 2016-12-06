#ifndef EXAMPLE5_H1
#define EXAMPLE5_H1


#include <TSpline.h>
#include <TF1.h>


//for FADC
#define TDC_CH 32 //number of channels in the TDC
#define TDC_CHIPS 4 // number of chips in each TDC. Each chip has 8 channels.
#define TDC_CH_PER_CHIP 8
#define TDC_MULTIPLICITY 300 //max number of hits per event
 

#define FADC_BOARDS 3 //number of FADC boards.
#define FADC_CH_PER_BOARD 8 //number of FADC channels per board. Second FADC starts from 8 to 16 and so on
#define FADC_CH 32 //total number of channels
#define FADC_SAMPLES 200 //maximum number of samples per channel in each event

#define PED_WIDTH 60

/* FOR FADC TIMING */
#define NTime 5 //numero di samples da usare per il best fit con retta (metodo 1) 
#define NReject 2 //numero di samples da scartare prima del picco. 0=prendi ANCHE il picco 
#define ErrorFADC 1 //errore sui punti campionati (in LSB)
#define CFraction 0.5 //frazione del segnale a cui prendere il picco
 
#define TRUE 1
#define FALSE 0

/*----------------------//
// Global vars go here //
/----------------------*/




//ALGORITH TO CALCULATE TIMING

double GetTimeRect(int *fadc,int channel,int peakpos,double energy,TF1 **function,int *FAllocated);
double GetTimeEvolved(int *fadc,int channel,int peakpos,double energy);
double GetTimeEvolvedOld(int *fadc,int channel,int peakpos,double energy,TF1 **function,int *FAllocated);

 
#endif


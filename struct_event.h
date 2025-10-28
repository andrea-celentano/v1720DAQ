#ifndef STRUCTEVENT_HG
#define STRUCTEVENT_HG

#define FADC_CH 16
#define MAX_PEAKS 30
#define MAX_SAMPLES_PER_PEAK 200
#include <vector>

struct peak{
	double peak_position;
	double peak_val;
	double peak_start;
	double peak_end;
	double energy;
	double time;

	ClassDefNV(peak,1);
};

struct event{
  uint32_t trig_time;
  int num;
  int samples_per_channel;
  int channel_mask;
  int active_channels;

  unsigned int nPEAKS[FADC_CH];
  double ped_mean[FADC_CH];
  double ped_sigma[FADC_CH];

  unsigned int peakStartTime[FADC_CH][MAX_PEAKS];
  unsigned int nFADC[FADC_CH][MAX_PEAKS];
  unsigned short fadc[FADC_CH][MAX_PEAKS][MAX_SAMPLES_PER_PEAK];


  peak  peaks[FADC_CH][MAX_PEAKS];

  ClassDefNV(event,1);


};

#endif

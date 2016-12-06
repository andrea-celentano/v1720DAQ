#define FADC_CH 8
#include <vector>

typedef struct{
  uint32_t trig_time;
  int num;
  int samples_per_channel;
  int channel_mask;
  int active_channels;
  std::vector < std::vector < int > > fadc;

	  
  std::vector < double > ped_mean ;
  std::vector < double > ped_sigma ;
  std::vector < double > peak_position ;
  std::vector < double > peak_val ;
  std::vector < double > peak_start ;
  std::vector < double > peak_end ;
  std::vector < double > energy ;
  std::vector < double > time ;
}event;

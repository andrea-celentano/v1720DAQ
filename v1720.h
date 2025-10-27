#include <gtk/gtk.h>

/* single channel registers map */
/* You should sum to this values 0x0n00, with n:0...7 */
#define V1725

#ifdef V1725
  #define NCH    16     // V1725 has 16 channels
  #define Nbit   14     // V1725 has 14-bit ADC
  #define mVmax  2000   // Example: different voltage range
#else
  #define NCH    8      // Default (e.g., V1720)
  #define Nbit   12
  #define mVmax  2000
#endif

#define V1720zs_thres  0x1024       
#define V1720zs_nsamp 0x1028     
#define V1720threshold 0x1080
#define V1720ndata_over_under_threshold 0x1084        
#define V1720status 0x1088     
#define V1720amc_fpga_firmware_rev 0x108C      
#define V1720buffer_occupancy 0x1094       
#define V1720dac_offset 0x1098 
#define V1720adc_config 0x109C     

/* registers address map */

#define V1720data 0x0000 //first data

 

 
#define  V1720chan_config 0x8000       
#define  V1720chan_config_bit_set 0x8004       
#define  V1720chan_config_bit_clear 0x8008        
#define  V1720buffer_organization 0x800C       
#define  V1720buffer_free 0x8010      
#define  V1720custom_size 0x8020       
#define  V1720acquisition_control 0x8100      
#define  V1720acquisition_status 0x8104        
#define  V1720sw_trigger  0x8108       
#define  V1720trigger_source_enable_mask 0x810C       
#define  V1720front_panel_trigger_out_enable_mask 0x8110     
#define  V1720post_trigger_setting 0x8114       
#define  V1720front_panel_io_data 0x8118       
#define  V1720front_panel_io_control 0x811C      
#define  V1720channel_enable_mask  0x8120      
#define  V1720roc_fpga_firmware_rev 0x8124       
#define  V1720event_stored 0x812C        
#define  V1720monitor_dac 0x8138        
#define  V1720board_info 0x8140       
#define  V1720monitor_mode 0x8144        
#define  V1720event_size 0x814C        
#define  V1720vme_control 0xEF00      
#define  V1720vme_status 0xEF04        
#define  V1720geo_address 0xEF08        
#define  V1720multicast_base_address_and_control 0xEF0C        
#define  V1720relocation_address 0xEF10       
#define  V1720interrupt_status_id 0xEF14     
#define  V1720interrupt_event_number 0xEF18      
#define  V1720blt_event_number 0xEF1C       
#define  V1720scratch 0xEF20       
#define  V1720sw_reset 0xEF24       
#define  V1720sw_clear 0xEF28       
#define  V1720flash_enable 0xEF2C       
#define  V1720flash_data 0xEF30        
#define  V1720config_reload 0xEF34       




#define V1720_BOARD_ID   0x000006B8
#define V1720_FIRMWARE_REV   0x202

/*status register*/
#define V1720_STATUS_DATA_READY      0x1
#define V1720_STATUS_BUFFER_FULL     0x2
#define V1720_STATUS_BERR_FLAG       0x4

/*control register*/
#define V1720_OPTICAL_INTERRUPT      0x008
#define V1720_BERR_ENABLE            0x010
#define V1720_ALIGN64                0x020
#define V1720_RELOC                  0x040
#define V1720_RORA_ROAK              0x080 /*RORA=0,ROAK=1*/
#define V1720_BLT_RANGE              0x100 /*0x100-> bit8 counting from 0.Not defined in the manual for VME control register, 0xEF00. Ok, is a undocumented feature!*/

/*Acquisition Control */
#define V1720_START_STOP 0x04 /*third bit */

/*channel status register */
#define V1720_CH_DAC_BUSY 0x04 /*third bit*/


/*Error and OK*/
#define ERROR 0
#define OK 1


/*DC Offset auto-set*/
  
#define AUTO_DC_OFFSET_START 20000
#define DC_OFFSET_VAR_BIG 100 /*Variation while going down from AUTO_DC_START*/
#define DC_OFFSET_VAR_SMALL 1 /*Variation while returning up*/


/* Structure to hold important informations from single channel */
typedef struct{
  int enabled; /* is the channel enabled(1) or not (0) */ 
  int trig_enabled; /*is the channel going to trigger the system(1) or not (0) */
  int trig_out_enabled; /*If this channel triggers, the out trig is enabled (1) or not (0) */
  int trig_thr; /*in ADC counts (12 bits) */
  int zle_thr; /*in ADC counts (12 bits) */
  int zle_pre; /*how many ZLE pre-samples*/
  int zle_post; /*how many ZLE post-samples*/
  int trig_n_over_thr; /*How many (4/5) samples should stay at least under/over thr to trigger */
  int dac; /* DAC valuprintf("I'm going to read %i words \n",n_words_per_channel);e for the channel, in DAC)(16 bits) counts */

  int baseline; /*The baseline value in DAC counts*/
}V1720_channel;

/* structure to hold informations for the board */
typedef struct {
  int start_stop ; //the status of acquisition. Start(1) or Stop(0).
  int handle; /* the int used to handle comm with VME */
  V1720_channel ch[8]; /*info for the channels */
  
  /* Channel configuration*/
  int trig_overlap_enable;
  int test_pattern_enable;
  int memory_access_type;
  int trig_polarity; /*Over thr: 0, under thr: 1. Positive pulse: 0, negative pulse: 1 */
  int pack25_enable;
  int zs_mode; /*0--> no zero suppression,2-->zero lenght encoding,3->full suppression based on amplitude */
  
  /* Buffer stuff */
  int buff_org; /*See 4.15 manual */
  int custom_size; /*See 4.17 manual */
  
  /*Acquisition */
  int trig_count; /*0  count accepted triggers,1 count all*/
  int run_mode; /*0-register-controlled run mode, 1-s-in controlled run mode, 2-s-in gate mode, 3-multi-board sync mode*/
  int chan_mask; /*1->chan i enabled*/

  /*Trigger */
  int softw_trig_enable;
  int ext_trig_enable;
  int coinc_level; /*number of channels that should be und/over thr, beyond the triggering ch,to generate the local trig*/
  int trig_chan_mask; /*1->chan i can trigger*/
  int softw_trig_enable_out;
  int ext_trig_enable_out;
  int trig_out_chan_mask;
  int post_trigger_setting;
  int n_samples_over_thr;
  int front_panel_nim_ttl; /*0: NIM, 1: TTL*/


  /*VME Control */
  int BERR_enable;  
  int interrupt_level; /*From 0 to 7 */
  int ALIGN64_enable;
  int RELOC_enable;
  int RORA_ROAK; /*RORA=0,ROAK=1*/

  /*interrupt */
  int optical_interrupt_enable;
  int interrupt_event_number; /*from 0 to 1023*/
  

  /*BLT*/
  int blt_event_number;
  int increased_blt_range; /*Undocumented feature */

}V1720_board;



typedef struct{
  GtkWidget *window1T;
  V1720_board *board;
}window2_data;








/* Low level functions prototypes */
int v1720SetOpticalInterrupt(int handle,int flag);
int v1720SetInterruptLevel(int handle,int int_level);
int v1720SetInterruptEventNumber(int handle,int evnt_number);

int v1720SetBLTRange(int handle,int flag);
int v1720SetAlign64(int handle,int flag);
int v1720SetRORAROAK(int handle,int flag);
int v1720SetBusError(int handle,int flag);
int v1720GetNextEventSize(int handle);
int v1720Dready(int handle);
int v1720GetBusError(int handle);
int v1720StatusFull(int handle);
int v1720GetBLTEventNumber(int handle);
int v1720SetBLTEventNumber(int handle,int nevents);
int v1720SetMonitorMode(int handle,int mask);
int v1720SetMonitorDAC(int handle,int mask);
int v1720SetChannelEnableMask(int handle,int mask);
int v1720SetFrontPanelIOData(int handle,int data);
int v1720SetFrontPanelIOControl(int handle, int data);
int v1720SetPostTrigger(int handle,int mask);
int v1720SetFontPanelTriggerOutEnableMask(int handle,int sw_trig,int ext_trig,int chan_mask);
int v1720SetTriggerEnableMask(int handle,int sw_trig,int ext_trig,int coinc_level,int chan_mask);
int v1720GenerateSoftwareTrigger(int handle);
int v1720SetAcquisition(int handle,int trig_count,int mode);
int v1720SetBufferSizePerEvent(int handle,int nloc);
int v1720FreeBuffer(int handle,int nblocks);
int v1720SetBuffer(int handle,int code); /*Buffer Organization register */
int v1720SetAllChannels(int handle,int zero_supr,int pack25,int trig_out,int mem_access,int test_pattern,int trig_overlap);
int v1720SetChannelDACOffset(int handle,int ch,int DACoffset);
int v1720SetChannelNdataOverUnderThreshold(int handle,int ch,int ndata);
int v1720SetChannelTrigThreshold(int handle,int ch,int threshold);
int v1720SetChannelZSNsamples(int handle,int ch,int nsamples);
int v1720SetChannelZSSamplesPREPOST(int handle, int ch, int npre,int npost);
int v1720GetChannelZSThreshold(int handle,int ch);
int v1720SetChannelZSThreshold(int handle,int ch,int logic,int threshold);
int v1720Reset(int handle);
int v1720Clear(int handle);
int v1720Init(V1720_board *bd);

int v1720StartStopAcquisition(int handle,int flag); /*0: stop. 1:start */

/*Function used to auto-set the DC offset of the board, for negative polarity*/
int v1720AutoSetDCOffset(int handle,int ch,double baseline_goal,int *ret_baseline);

/*Function used to get mean*/
int v1720GetMean(int handle,int ch);


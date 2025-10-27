#include <stdio.h>
extern "C" {
#include "CAENComm.h"
}
#include "v1720.h"
#include <string.h>
#include <pthread.h>
#include <cmath>

/* Mutex to hold of reads and writes from competing threads */
pthread_mutex_t c1720_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_1720 {	if(pthread_mutex_lock(&c1720_mutex)<0) perror("pthread_mutex_lock");}
#define UNLOCK_1720 { if(pthread_mutex_unlock(&c1720_mutex)<0) perror("pthread_mutex_unlock"); }

int v1720Init(V1720_board *bd) { /*we need just a pointer to the V1720_board struct*/
	printf("Init v1720\n");
	int res;
	res = 0;
	if (bd->handle == -1) {
		res = CAENComm_OpenDevice(CAENComm_OpticalLink, 0, 0, 0, &(bd->handle));
	}
	if (res != 0) {
		char err_code[50];
		CAENComm_DecodeError(res, err_code);
		printf("Error opening device: error %s \n", err_code);
		return 0;
	} else { /* ok, let's configure the board */
		// v1720Clear(bd->handle);
		v1720Reset(bd->handle);

		int ii;
		bd->zs_mode = 0; /*default, no Zero Suppression */
		bd->test_pattern_enable = 0;
		bd->trig_overlap_enable = 0;
		bd->memory_access_type = 1; /*sequential*/
		bd->pack25_enable = 0;

		bd->trig_polarity = 0;

		/*Single channel stuffs*/
		bd->chan_mask = 0; /*info stored in single ch */
		bd->trig_chan_mask = 0;
		bd->trig_out_chan_mask = 0;
		for (ii = 0; ii < NCH; ii++) {
			v1720SetChannelTrigThreshold(bd->handle, ii, bd->ch[ii].trig_thr);
			v1720SetChannelNdataOverUnderThreshold(bd->handle, ii, bd->ch[ii].trig_n_over_thr);
			v1720SetChannelDACOffset(bd->handle, ii, bd->ch[ii].dac);
			v1720SetChannelZSThreshold(bd->handle, ii, 0, bd->ch[ii].zle_thr); /*Logic 0 is for negative threshold*/
			//	v1720SetChannelZSSamplesPREPOST(bd->handle, ii, bd->ch[ii].zle_pre, bd->ch[ii].zle_post);
			bd->chan_mask = bd->chan_mask | ((bd->ch[ii].enabled) << ii);
			bd->trig_chan_mask = bd->trig_chan_mask | ((bd->ch[ii].trig_enabled) << ii);
			bd->trig_out_chan_mask = bd->trig_out_chan_mask | ((bd->ch[ii].trig_out_enabled) << ii);
		}
		/*Whole board stuffs*/

		v1720SetAllChannels(bd->handle, bd->zs_mode, bd->pack25_enable, bd->trig_polarity, bd->memory_access_type, bd->test_pattern_enable, bd->trig_overlap_enable);

		v1720SetBuffer(bd->handle, bd->buff_org);
		v1720SetBufferSizePerEvent(bd->handle, bd->custom_size);

		v1720SetTriggerEnableMask(bd->handle, bd->softw_trig_enable, bd->ext_trig_enable, bd->coinc_level, bd->trig_chan_mask);
		v1720SetFontPanelTriggerOutEnableMask(bd->handle, bd->softw_trig_enable_out, bd->ext_trig_enable_out, bd->trig_out_chan_mask);
		v1720SetPostTrigger(bd->handle, bd->post_trigger_setting);
		v1720SetChannelEnableMask(bd->handle, bd->chan_mask);
		v1720SetBLTEventNumber(bd->handle, bd->blt_event_number);
		v1720SetBusError(bd->handle, bd->BERR_enable);
		v1720SetAlign64(bd->handle, bd->ALIGN64_enable);
		v1720SetBLTRange(bd->handle, bd->increased_blt_range);
		v1720SetInterruptLevel(bd->handle, bd->interrupt_level);
		v1720SetOpticalInterrupt(bd->handle, bd->optical_interrupt_enable);
		v1720SetInterruptEventNumber(bd->handle, bd->interrupt_event_number);
		v1720SetRORAROAK(bd->handle, bd->RORA_ROAK);

		v1720SetFrontPanelIOControl(bd->handle, bd->front_panel_nim_ttl);

		/*Interrupt Stuffs*/

		//    CAENComm_IRQEnable(bd->handle);
		v1720SetAcquisition(bd->handle, bd->trig_count, bd->run_mode);
		return 1;
	}
}

/*******************/
/* low-level calls */

int v1720Clear(int handle) {

	LOCK_1720;
	CAENComm_Write32(handle, V1720sw_clear, 1);
	UNLOCK_1720;
	return 1;
}
int v1720Reset(int handle) {
	LOCK_1720;
	CAENComm_Write32(handle, V1720sw_reset, 1);
	UNLOCK_1720;
	return 1;
}
int v1720SetChannelZSThreshold(int handle, int ch, int logic, int threshold) {
	unsigned int mask;
	int offset;
	offset = ch * (0x0100);
	if (logic < 0 || logic > 1 || threshold < 0 || threshold > 0xFFF) {
		printf("v1720SetChannelZSThreshold ERROR: bad parameter(s)\n");
		return 0;
	}
	mask = (logic << 31) + threshold;
	LOCK_1720;
	CAENComm_Write32(handle, V1720zs_thres + offset, mask);
	UNLOCK_1720;
	return 1;
}
int v1720GetChannelZSThreshold(int handle, int ch) {
	int offset;
	unsigned int thr;
	offset = ch * (0x0100);
	LOCK_1720;
	CAENComm_Read32(handle, V1720zs_thres + offset, &thr);
	UNLOCK_1720;
	thr = (thr) & (0xFFF); //first 12 LSB
	return thr;
}

int v1720SetChannelZSNsamples(int handle, int ch, int nsamples) {
	int offset;
	offset = ch * (0x0100);
	unsigned int mask;
	mask = nsamples; /*must distinguish 2 cases - see manual ??*/
	LOCK_1720;
	CAENComm_Write32(handle, V1720zs_nsamp + offset, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetChannelZSNsamplesPrePost(int handle, int ch, int nPRE, int nPOST) {
	int offset;
	offset = ch * (0x0100);
	unsigned int mask;
	if ((nPRE < 0) || (nPRE > 0xFFFF) || (nPOST < 0) || (nPOST > 0xFFFF)) {
		printf("v1720SetChannelZSNsamplesPrePost bad parameter %i - %i \n", nPRE, nPOST);
		return 0;
	}
	mask = (nPRE << 16) + nPOST; /*must distinguish 2 cases - see manual ??*/
	LOCK_1720;
	CAENComm_Write32(handle, V1720zs_nsamp + offset, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetChannelTrigThreshold(int handle, int ch, int threshold) {
	unsigned int mask;
	int offset;
	offset = ch * (0x0100);
	if (threshold < 0 || threshold > 0xFFF) {
		printf("v1720SetChannelThreshold ERROR: bad parameter(s)\n");
		return 0;
	}

	mask = threshold;
	LOCK_1720;
	CAENComm_Write32(handle, V1720threshold + offset, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetChannelNdataOverUnderThreshold(int handle, int ch, int ndata) {
	unsigned int mask;
	int offset;
	offset = ch * (0x0100);
	if (ndata < 0 || ndata > 0xFFF) {
		printf("v1720SetNdataOverUnderThreshold ERROR: bad parameter(s)\n");
		return 0;
	}
	mask = ndata;
	LOCK_1720;
	CAENComm_Write32(handle, V1720ndata_over_under_threshold + offset, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetChannelDACOffset(int handle, int ch, int DACoffset) {
	unsigned int mask;
	unsigned int ret;
	ret = 0;
	int offset;
	offset = ch * (0x0100);
	if (DACoffset < 0 || DACoffset > 0xFFFF) {
		printf("v1720SetChannelDACOffset ERROR: bad parameter(s). Offset can't be %i\n", DACoffset);
		return 0;
	}

	mask = DACoffset;
	LOCK_1720;
	CAENComm_Write32(handle, V1720dac_offset + offset, mask);
	CAENComm_Read32(handle, V1720status + offset, &ret);
	UNLOCK_1720;
	ret = ret & V1720_CH_DAC_BUSY;
	/*need to check status bit 2 ???*//*Seems yes, otherwise only 1 channel every 2 is configured */
	while (ret == V1720_CH_DAC_BUSY) {
		LOCK_1720;
		CAENComm_Read32(handle, V1720status + offset, &ret);
		UNLOCK_1720;
		ret = ret & V1720_CH_DAC_BUSY;
	}

	LOCK_1720;
	CAENComm_Read32(handle, V1720dac_offset + offset, &ret);
	UNLOCK_1720;
	if (ret == mask) {
		printf("I wrote in %#x the DAQ offset %i for ch %i \n", V1720dac_offset + offset, mask, ch);
		return 1;
	} else {
		printf("Error while writing in %#x the DAQ offset for ch %i. Board is %i, trying to wrote %i \n", V1720dac_offset + offset, ch, ret, mask);
		return 0;
	}
}

/*
 inputs:
 zero_supr: 0 - no zero suppression, 2 - ZLE(zero length encoded), 3 - ZS AMP(full suppression besed on amplitude)
 pack25: 0 - pack2.5 disabled, 1 - pack2.5 enabled
 trig_out: 0 - trigger out on input over threshold, 1 - under threshold
 mem_access: 0 - memory random access, 1 - sequential access
 test_pattern: 0 - test pattern generation disabled, 1 - enabled
 trig_overlap: 0 - trigger overlapping disabled, 1 - enabled
 */
int v1720SetAllChannels(int handle, int zero_supr, int pack25, int trig_out, int mem_access, int test_pattern, int trig_overlap) {
	unsigned int mask;

	if (zero_supr < 0 || zero_supr == 2 || zero_supr > 3 || pack25 < 0 || pack25 > 1 || trig_out < 0 || trig_out > 1 || mem_access < 0 || mem_access > 1 || test_pattern < 0 || test_pattern > 1 || trig_overlap < 0 || trig_overlap > 1) {
		printf("v1720SetAllChannels ERROR: bad parameter(s)\n");
		return 0;
	}

	mask = (zero_supr << 16) + (pack25 << 11) + (trig_out << 6) + (mem_access << 4) + (test_pattern << 3) + (trig_overlap << 1);
	LOCK_1720;
	CAENComm_Write32(handle, V1720chan_config, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetBuffer(int handle, int code) {
	unsigned int mask;

	if (code < 0 || code > 0x0A) {
		printf("v1720SetBuffer ERROR: bad parameter(s)\n");
		return 0;
	}

	mask = code;
	LOCK_1720;
	CAENComm_Write32(handle, V1720buffer_organization, mask);
	UNLOCK_1720;
	return 1;
}

/* frees first 'nblocks' outbut buffer memory blocks */
int v1720FreeBuffer(int handle, int nblocks) {
	unsigned int mask;
	if (nblocks < 0 || nblocks > 0xFFF) {
		printf("v1720FreeBuffer ERROR: bad parameter(s)\n");
		return 0;
	}
	mask = nblocks;
	LOCK_1720;
	CAENComm_Write32(handle, V1720buffer_free, mask);
	UNLOCK_1720;
	return 1;
}

int v1720SetBufferSizePerEvent(int handle, int nloc) {
	unsigned int mask;
	mask = nloc;
	LOCK_1720;
	CAENComm_Write32(handle, V1720custom_size, mask);
	UNLOCK_1720;
	return 1;
}

/*
 inputs:
 trig_count: 0-count accepted triggers, 1-count all triggers
 mode: 0-register-controlled run mode, 1-s-in controlled run mode, 2-s-in gate mode, 3-multi-board sync mode
 */
int v1720SetAcquisition(int handle, int trig_count, int mode) {
	unsigned int mask;

	if (trig_count < 0 || trig_count > 1 || mode < 0 || mode > 3) {
		printf("v1720SetAcquisition ERROR: bad parameter(s)\n");
		return 0;
	}
	mask = (trig_count << 3) + mode;
	LOCK_1720;
	CAENComm_Write32(handle, V1720acquisition_control, mask);
	UNLOCK_1720;
	return 1;
}

int v1720StartStopAcquisition(int handle, int flag) {
	if (flag < 0 || flag > 1) {
		printf("v1720StartStopAcquisition: bad parameter(s)\n");
		return 0;
	}
	unsigned int mask;
	mask = 0;
	LOCK_1720;
	CAENComm_Read32(handle, V1720acquisition_control, &mask);
	UNLOCK_1720;
	if (flag == 1) {
		mask = mask | V1720_START_STOP;

	}
	if (flag == 0) {
		mask = mask & (~V1720_START_STOP);

	}
	LOCK_1720;
	CAENComm_Write32(handle, V1720acquisition_control, mask);
	UNLOCK_1720;
	return OK;
}

int v1720GenerateSoftwareTrigger(int handle) {
	LOCK_1720;
	CAENComm_Write32(handle, V1720sw_trigger, 1);
	UNLOCK_1720;
	return 1;
}

/*
 inputs:
 sw_trig: 0-softare trigger disabled, 1-software trigger enabled
 ext_trig 0-external trigger disabled, 1-external trigger enabled
 coinc_level: 1-at least 2 channels above threshold required to generate trigger, 2-at least 3, and so on
 chan_mask: 8-bit mask enabling/disabling channel triggers, bit 0 is channel 0, bit 7 is channel 7
 */
int v1720SetTriggerEnableMask(int handle, int sw_trig, int ext_trig, int coinc_level, int chan_mask) {
	unsigned int mask;

	if (sw_trig < 0 || sw_trig > 1 || ext_trig < 0 || ext_trig > 1 || coinc_level < 0 || coinc_level > 7 || chan_mask < 0 || chan_mask > 0xFF) {
		printf("v1720SetTriggerEnableMask ERROR: bad parameter(s)\n");
		return ERROR;
	}

	mask = (sw_trig << 31) + (ext_trig << 30) + (coinc_level << 24) + chan_mask;

	printf("Trigger mask is %x\n", mask);

	LOCK_1720;
	CAENComm_Write32(handle, V1720trigger_source_enable_mask, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetFontPanelTriggerOutEnableMask(int handle, int sw_trig, int ext_trig, int chan_mask) {
	unsigned int mask;

	if (sw_trig < 0 || sw_trig > 1 || ext_trig < 0 || ext_trig > 1 || chan_mask < 0 || chan_mask > 0xFF) {
		printf("v1720SetFontPanelTriggerOutEnableMask ERROR: bad parameter(s)\n");
		return ERROR;
	}

	mask = (sw_trig << 31) + (ext_trig << 30) + chan_mask;
	LOCK_1720;
	CAENComm_Write32(handle, V1720front_panel_trigger_out_enable_mask, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetPostTrigger(int handle, int mask) {

	if (mask < 0 || mask > 0xFFFF) {
		printf("v1720SetPostTrigger ERROR: bad parameter(s)\n");
		return ERROR;
	}
	LOCK_1720;
	CAENComm_Write32(handle, V1720post_trigger_setting, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetFrontPanelIOData(int handle, int data) {
	unsigned int mask;

	if (data < 0 || data > 0xFFFF) {
		printf("v1720SetFontPanelIOData ERROR: bad parameter(s)\n");
		return ERROR;
	}

	mask = data;
	LOCK_1720;
	CAENComm_Write32(handle, V1720front_panel_io_data, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetFrontPanelIOControl(int handle, int data) {
	unsigned int mask;

	if (data < 0 || data > 0xFFFF) {
		printf("v1720SetFontPanelIOData ERROR: bad parameter(s)\n");
		return ERROR;
	}

	mask = data;
	LOCK_1720;
	CAENComm_Write32(handle, V1720front_panel_io_control, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetChannelEnableMask(int handle, int mask) {
	if (mask < 0 || mask > 0xFF) {
		printf("v1720SetChannelEnableMask ERROR: bad parameter(s)\n");
		return ERROR;
	}
	printf("v1720 Enable mask is %x \n", mask);
	LOCK_1720;
	CAENComm_Write32(handle, V1720channel_enable_mask, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetMonitorDAC(int handle, int mask) {
	if (mask < 0 || mask > 0xFFF) {
		printf("v1720SetMonitorDAC ERROR: bad parameter(s)\n");
		return ERROR;
	}
	LOCK_1720;
	CAENComm_Write32(handle, V1720monitor_dac, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetMonitorMode(int handle, int mask) {
	if (mask < 0 || mask > 4) {
		printf("v1720SetMonitorMode ERROR: bad parameter(s)\n");
		return ERROR;
	}
	LOCK_1720;
	CAENComm_Write32(handle, V1720monitor_mode, mask);
	UNLOCK_1720;
	return OK;
}

int v1720SetBLTEventNumber(int handle, int nevents) {
	if (nevents < 0 || nevents > 0xFF) /*only 8 bits, from 0 to 255 */
	{
		printf("v1720SetBLTEventNumber ERROR: bad parameter(s)\n");
		return ERROR;
	}
	LOCK_1720;
	CAENComm_Write32(handle, V1720blt_event_number, nevents);
	UNLOCK_1720;
	return OK;
}

int v1720GetBLTEventNumber(int handle) {
	unsigned int rval;
	LOCK_1720;
	rval = CAENComm_Read32(handle, V1720blt_event_number, &rval);
	UNLOCK_1720;
	rval = rval & 0xFF;
	return rval;
}

/*******************************************************************************
 * RETURNS: 1 if Full, 0 if Not Full
 */

int v1720StatusFull(int handle) {
	unsigned int res;
	LOCK_1720;
	res = CAENComm_Read32(handle, V1720vme_status, &res);
	UNLOCK_1720;
	res = res & V1720_STATUS_BUFFER_FULL;
	return (res);
}

/* BERR is re-set after a status register read-out */

int v1720GetBusError(int handle) {
	unsigned int res;
	LOCK_1720;
	CAENComm_Read32(handle, V1720vme_status, &res);
	res = res & V1720_STATUS_BERR_FLAG;
	UNLOCK_1720;
	return (res);
}

/******************************************************************************
 * RETURNS: 0(No Data) or the number of events
 */

int v1720Dready(int handle) {
	unsigned int stat = 0, nevents;
	LOCK_1720;
	CAENComm_Read32(handle, V1720vme_status, &stat);
	stat = stat & V1720_STATUS_DATA_READY;
	if (stat) {
		CAENComm_Read32(handle, V1720event_stored, &nevents);
		UNLOCK_1720;
		return (nevents);
	} else {
		UNLOCK_1720;
		return (0);
	}
}

int v1720GetNextEventSize(int handle) {
	unsigned int ret;
	LOCK_1720;
	CAENComm_Read32(handle, V1720event_size, &ret);
	/*ret++; *//*until explained Andrea: seems not needed with new firmware!*/
	UNLOCK_1720;

	return (ret);
}

/******************************************************************************
 *
 * v1720SetBusError - Enable/Disable Bus Errors (to finish a block transfer,
 *                   or on an empty buffer read)
 *                    flag = 0 : Disable
 *                           1 : Enable
 *
 * REv1720SetChannelDACOffset(%s,%s,%s)",fadc,ch,value)TURNS: OK if successful, ERROR otherwise.
 */

int v1720SetBusError(int handle, int flag) {
	unsigned int reg;
	if (flag < 0 || flag > 1) {
		printf("v1720SetBusError ERROR: Invalid flag = %d", flag);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	if (flag == 1) {
		printf("set BUSerror\n");
		reg = reg | V1720_BERR_ENABLE;
	} else if (flag == 0) {
		printf("reset BUSerror\n");
		reg = reg & ~V1720_BERR_ENABLE;
	}

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (OK);
}

/******************************************************************************
 *
 * v1720SetAlign64 - Enable/Disable 64 bit alignment for block transfers
 *                   flag = 0 : Disable
 *                          1 : Enable
 *
 * RETURNS: OK if successful, ERROR otherwise.
 */

int v1720SetAlign64(int handle, int flag) {
	unsigned int reg;

	if (flag < 0 || flag > 1) {
		printf("v1720SetAlign64 ERROR: Invalid flag = %d", flag);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	if (flag == 1) {
		printf("set Align64\n");
		reg = reg | V1720_ALIGN64;
	} else if (flag == 0) {
		printf("reset Aligh64\n");
		//reg = reg & ~V1720_ALIGN64;
	}

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (OK);
}

int v1720SetBLTRange(int handle, int flag) {
	unsigned int reg;
	if (flag < 0 || flag > 1) {
		printf("v1720SetBLTRange ERROR: Invalid fg = %d", flag);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	if (flag == 1) {
		printf("set BLT_RANGE\n");
		reg = reg | V1720_BLT_RANGE;
	} else if (flag == 0) {
		printf("reset BLT_RANGE\n");
		reg = reg & ~V1720_BLT_RANGE;
	}

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (0);
}

int v1720SetInterruptLevel(int handle, int int_level) {
	unsigned int reg;
	if (int_level < 0 || int_level > 7) {
		printf("v1720SetInterruptLevel ERROR: Invalid fg = %d \n", int_level);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	reg = reg & 0xFFFFFFF8; /* put at 0 3 LSB */
	reg = reg + int_level;

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (0);
}

int v1720SetInterruptEventNumber(int handle, int evnt_number) {

	if (evnt_number < 0 || evnt_number > 1023) {
		printf("v1720SetInterruptLevel ERROR: Invalid fg = %d", evnt_number);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Write32(handle, V1720interrupt_event_number, evnt_number);

	UNLOCK_1720;

	return (0);
}

int v1720SetOpticalInterrupt(int handle, int flag) {
	unsigned int reg;
	if (flag < 0 || flag > 1) {
		printf("v1720SetOpticalInterrupt ERROR: Invalid fg = %d", flag);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	if (flag == 1) {
		printf("set OPTICAL INTERRUPT\n");
		reg = reg | V1720_OPTICAL_INTERRUPT;
	} else if (flag == 0) {
		printf("reset OPTICAL INTERRUPT\n");
		reg = reg & ~V1720_OPTICAL_INTERRUPT;
	}

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (0);
}

int v1720SetRORAROAK(int handle, int flag) {
	unsigned int reg;
	if (flag < 0 || flag > 1) {
		printf("v1720Set RORAROAK ERROR: Invalid fg = %d \n", flag);
		return ERROR;
	}

	LOCK_1720;

	CAENComm_Read32(handle, V1720vme_control, &reg);

	if (flag == 1) {
		printf("set ROAK \n");
		reg = reg | V1720_RORA_ROAK;
	} else if (flag == 0) {
		printf("set RORA\n");
		reg = reg & ~V1720_RORA_ROAK;
	}

	CAENComm_Write32(handle, V1720vme_control, reg);

	UNLOCK_1720;

	return (0);

}



int v1720GetBaseline(int handle, int ch) {
	printf("START v1720GetBaseline ch %i\n", ch);
	int jj;
	uint32_t trigger_mask, channel_mask;
	uint32_t old_trigger_mask, old_channel_mask;
	uint32_t buffp[20000];
	uint32_t *buff_point;
	int n_words;
	int mean_sum;
	double mean;

	uint32_t bd_config;

	LOCK_1720;
	CAENComm_Read32(handle, V1720trigger_source_enable_mask, &old_trigger_mask);
	UNLOCK_1720;
	trigger_mask = (1 << 31); //enable ONLY sw_trigger: we need it!!
	LOCK_1720;
	CAENComm_Write32(handle, V1720trigger_source_enable_mask, trigger_mask);
	CAENComm_Read32(handle, V1720channel_enable_mask, &old_channel_mask);
	//DISABLE ZLE
	CAENComm_Read32(handle, V1720chan_config, &bd_config);
	CAENComm_Write32(handle, V1720chan_config, bd_config & (~(0xF << 16))); //disable the ZLE by setting bits 16.19 at 0
	UNLOCK_1720;

	if (((old_channel_mask >> ch) & 0x1) == 1) {

		channel_mask = (1 << ch);
		LOCK_1720;
		CAENComm_Write32(handle, V1720channel_enable_mask, channel_mask);
		UNLOCK_1720;

		/*First trigger: ignore it!*/
		v1720StartStopAcquisition(handle, 1);
		LOCK_1720;
		CAENComm_Write32(handle, V1720sw_trigger, 0x1);
		UNLOCK_1720;
		v1720StartStopAcquisition(handle, 0);

		/*To do it it the easy way (and port it to VXWORKS in a few seconds), i read word by word */

		buff_point = buffp;
		LOCK_1720;
		CAENComm_BLTRead(handle, V1720data, buff_point, 16400, &n_words); //number of max bytes to read
		UNLOCK_1720;
		usleep(100000);
		/*End first trigger */

		v1720StartStopAcquisition(handle, 1);
		LOCK_1720;
		CAENComm_Write32(handle, V1720sw_trigger, 0x1);
		UNLOCK_1720;
		v1720StartStopAcquisition(handle, 0);

		/*To do it it the easy way (and port it to VXWORKS in a few seconds), i read word by word */

		buff_point = buffp;
		LOCK_1720;
		CAENComm_BLTRead(handle, V1720data, buff_point, 16400, &n_words); //number of max bytes to read
		UNLOCK_1720;
		usleep(100000);

		buff_point = buffp;
		buff_point += 4; // this is the first word containing data
		n_words = n_words - 4; //This is the number of words containing data
		mean_sum = 0;
		for (jj = 0; jj < n_words; jj++) {
			mean_sum = mean_sum + ((*buff_point) & 0xFFF) + (((*buff_point) >> 16) & 0xFFF);
			buff_point++;
		}
		mean = mean_sum / (n_words * 2);
		return mean;
	}
}

int v1720AutoSetDCOffset(int handle, int ch, double baseline_goal, int *ret_baseline) {

	printf("START AutoSetDCoffset ch %i\n", ch);
	int jj;
	uint32_t trigger_mask, channel_mask;
	uint32_t old_trigger_mask, old_channel_mask;
	uint32_t buffp[20000];
	uint32_t *buff_point;
	int n_words;
	int mean_sum;
	double mean;
	int DC_OFFSET;
	int flag; /*flag=1: first step, going down. flag=0: second step, going up.*/

	uint32_t bd_config;

	DC_OFFSET = AUTO_DC_OFFSET_START;

	LOCK_1720;
	CAENComm_Read32(handle, V1720trigger_source_enable_mask, &old_trigger_mask);
	UNLOCK_1720;
	trigger_mask = (1 << 31); //enable ONLY sw_trigger: we need it!!
	LOCK_1720;
	CAENComm_Write32(handle, V1720trigger_source_enable_mask, trigger_mask);
	CAENComm_Read32(handle, V1720channel_enable_mask, &old_channel_mask);
	//DISABLE ZLE
	CAENComm_Read32(handle, V1720chan_config, &bd_config);
	CAENComm_Write32(handle, V1720chan_config, bd_config & (~(0xF << 16))); //disable the ZLE by setting bits 16.19 at 0
	UNLOCK_1720;

	DC_OFFSET = AUTO_DC_OFFSET_START;
	v1720SetChannelDACOffset(handle, ch, DC_OFFSET);

	if (((old_channel_mask >> ch) & 0x1) == 1) {
		flag = 1;
		mean = -2000;
		channel_mask = (1 << ch);
		LOCK_1720;
		CAENComm_Write32(handle, V1720channel_enable_mask, channel_mask);
		UNLOCK_1720;

		/*First trigger: ignore it!*/
		v1720StartStopAcquisition(handle, 1);
		LOCK_1720;
		CAENComm_Write32(handle, V1720sw_trigger, 0x1);
		UNLOCK_1720;
		v1720StartStopAcquisition(handle, 0);

		/*To do it it the easy way (and port it to VXWORKS in a few seconds), i read word by word */

		buff_point = buffp;
		LOCK_1720;
		CAENComm_BLTRead(handle, V1720data, buff_point, 16400, &n_words); //number of max bytes to read
		UNLOCK_1720;
		usleep(100000);
		/*End first trigger */

		while ((flag) || ((-mean) <= baseline_goal)) {
			usleep(10);

			v1720StartStopAcquisition(handle, 1);
			LOCK_1720;
			CAENComm_Write32(handle, V1720sw_trigger, 0x1);
			UNLOCK_1720;
			v1720StartStopAcquisition(handle, 0);

			/*To do it it the easy way (and port it to VXWORKS in a few seconds), i read word by word */

			buff_point = buffp;
			LOCK_1720;
			CAENComm_BLTRead(handle, V1720data, buff_point, 16400, &n_words); //number of max bytes to read
			UNLOCK_1720;
			usleep(100000);

			buff_point = buffp;
			buff_point += 4; // this is the first word containing data
			n_words = n_words - 4; //This is the number of words containing data
			mean_sum = 0;
			for (jj = 0; jj < n_words; jj++) {
				mean_sum = mean_sum + ((*buff_point) & 0xFFF) + (((*buff_point) >> 16) & 0xFFF);
				buff_point++;
			}
			mean = (double) mean_sum / (n_words * 2);
			mean = -mVmax + mean / 4095 * mVmax;

			if (flag == 1) {
				if ((-mean) <= baseline_goal) {
					flag = 0;
					printf("0 Mean for ch %i is %f mV. Baseline_goal: %f. DC Offset is %i\n", ch, mean, -baseline_goal, DC_OFFSET);
				} else {
					DC_OFFSET = DC_OFFSET - DC_OFFSET_VAR_BIG;
					v1720SetChannelDACOffset(handle, ch, DC_OFFSET);
					printf("1 Mean for ch %i is %f mV. DC Offset is %i\n", ch, mean, DC_OFFSET);
				}
			} else if (flag == 0) {
				DC_OFFSET = DC_OFFSET + DC_OFFSET_VAR_SMALL;
				v1720SetChannelDACOffset(handle, ch, DC_OFFSET);
				printf("2 Mean for ch %i is %f mV. DC Offset is %i\n", ch, mean, DC_OFFSET);
			}
		}
	}
#ifdef VXWORKS
	taskDelay(sysClkRateGet()); /*wait 1 s*/
#else
	sleep(1);
#endif

	LOCK_1720;
	CAENComm_Write32(handle, V1720trigger_source_enable_mask, old_trigger_mask);
	CAENComm_Write32(handle, V1720channel_enable_mask, old_channel_mask);

	//ZLE
	CAENComm_Write32(handle, V1720chan_config, bd_config);
	UNLOCK_1720;
	UNLOCK_1720;

	/*Report also the baseline in ADC units*/
	*ret_baseline = (mean_sum) / (n_words * 2);

	return DC_OFFSET;
}


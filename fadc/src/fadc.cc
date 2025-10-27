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
fadc_analizer::energy_calculator::energy_calculator(fadc_analizer& o_fadc, const std::string& name, const std::string& description) :
		m_name(name), m_description(description), m_fadc(o_fadc),m_function(0),energy(0) {
}
;

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
int fadc_analizer::energy_calculator::SetFunction(double (*fun)(int *points, double *par)) {
	m_function = fun;
	return 0;
}

//this is just a "wrapper"
double fadc_analizer::energy_calculator::CalculateEnergy() {
	double *par = new double[7];
	par[0] = m_fadc.m_ped_mean;
	par[1] = m_fadc.m_ped_sigma;
	par[2] = m_fadc.m_peak_val;
	par[3] = m_fadc.m_peak_pos / m_fadc.dT;
	par[4] = m_fadc.m_peak_start / m_fadc.dT;
	par[5] = m_fadc.m_peak_end / m_fadc.dT;
	par[6] = m_fadc.m_SamplesPerChannel;

	energy = m_function(m_fadc.points, par);
	delete par;
	return energy;
}

fadc_analizer::time_calculator::time_calculator(fadc_analizer& o_fadc, const std::string& name, const std::string& description) :
		m_name(name), m_description(description), m_fadc(o_fadc),m_function(0),time(0) {
}
;

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
int fadc_analizer::time_calculator::SetFunction(double (*fun)(int *points, double *par)) {
	m_function = fun;
	return 0;
}

//this is just a "wrapper"
double fadc_analizer::time_calculator::CalculateTime() {
	double *par = new double[7];
	par[0] = m_fadc.m_ped_mean;
	par[1] = m_fadc.m_ped_sigma;
	par[2] = m_fadc.m_peak_val;
	par[3] = m_fadc.m_peak_pos / m_fadc.dT;
	par[4] = m_fadc.m_peak_start / m_fadc.dT;
	par[5] = m_fadc.m_peak_end / m_fadc.dT;
	par[6] = m_fadc.m_SamplesPerChannel;

	time = m_function(m_fadc.points[0], par);
	delete par;
	return time;
}

//here is the fadc stuff.

/*The constructor with its variable initializer list*/

fadc_analizer::fadc_analizer() :
		m_setup_done(1), //not yet done
		m_ped_width(0), m_SamplesPerChannel(0),
		/*Put here are the initialization for the calculators you want. Change only the name!*/
		ECalculator1(*this, "GetEnergySumV2", "Summing the value of bins but in a event-by-event determined interval"), //initializer for creator of calculator1.
		ECalculator2(*this, "GetEnergySum", "Summing the value of bins in a fixed interval centered on peak position"), //initializer for creator of calculator1.
		TCalculator1(*this, "GetTimeFADC250Firmware", "The same function in the FADC250 Firmware") //initializer for creator of calculator1.
{
	m_peak_end=0;
	m_ped_sigma=0;
	m_ped_mean=0;
	points=0;
	m_peak_start=0;
	m_peak_val=0;
	m_peak_pos=0;
}
;

/*The destructor*/
fadc_analizer::~fadc_analizer() {
}

void fadc_analizer::Setup(int PedWidth) {


	m_ped_width = PedWidth;

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

	m_setup_done = 0;
}
;

/*Function used to load the event data
 IN: int **samples: the fadc samples as a "matrix". First int is ch, second is samples
 OUT: the status
 */

int fadc_analizer::LoadEvent(unsigned short *samples, unsigned int N) {
	if (!samples) return -1;

	m_SamplesPerChannel = N;
	points = samples;
	return 0;
}
/*Function used to load the event data
 IN: nothing
 OUT: the status
 */

int fadc_analizer::ProcessEvent() {
	int ret = 0;

	ret += CalculatePedestal;
	ret += CalculatePeak();
	ret += CalculatePeakLimits();

	return ret; //should be 0 if fine.
}
;

/*Function used to calculate the pedestal mean and sigma in mV 
 IN: the channel to calculate
 OUT: the status of the calculation (0: ok, -1 error)
 */

int fadc_analizer::CalculatePedestal() {
	if (m_setup_done != 0) return -1;
	if (m_ped_width <= 0) {
		return -3;
	}
	m_ped_mean = 0;
	m_ped_sigma = 0;
	for (int ii = 0; ii < m_ped_width; ii++) {
		m_ped_mean += points[ii];
		m_ped_sigma += ((points[ii]) * (points[ii]));
	}
	m_ped_mean = LSB * m_ped_mean / m_ped_width; //OK
	m_ped_sigma = LSB * LSB * m_ped_sigma / m_ped_width; //this is the mean square value!

	m_ped_sigma = sqrt(m_ped_sigma - m_ped_mean * m_ped_mean);

	return 0;
}
;

/*Function used to calculate the peak position in ns and the peak value in mV
 IN: the channel to calculate
 OUT: the status of the calculation (0: ok, -1 error)
 */
int fadc_analizer::CalculatePeak() {
	if (m_setup_done != 0) return -1;

	m_peak_val = (int) ((-3 * m_ped_sigma + m_ped_mean) / fadc_analizer::LSB);
	m_peak_pos = 0;

	for (int ii = 0; ii < m_SamplesPerChannel; ii++) {
		if (points[ii] <= m_peak_val) {
			m_peak_val = points[ii];
			/*if(min<(-sigma_pedest)+mean_pedest)*/m_peak_pos = ii * dT;
		}
	}
	m_peak_val = m_peak_val * LSB - m_ped_mean;
	m_peak_val = m_peak_val * (-1); //since signals are negative!
	return 0;
}
;

/*Function used to calculate the peak start and end in ns
 IN: the channel to calculate
 OUT: the status of the calculation (0: ok, -1 error)
 */
int fadc_analizer::CalculatePeakLimits() {
	if (m_setup_done != 0) return -1;
	if (m_peak_pos <= 0) {
		//to be consistent with F.Cipro Macro!
		m_peak_start = 0;
		m_peak_end = 4;
		return -3;
	}
	//1: init them at the peak position IN SAMPLES (so I divide by dT)
	m_peak_start = (int) (m_peak_pos / dT);
	m_peak_end = (int) (m_peak_pos / dT);

	//2: the calculation: move start back until we reach the pedestal mean
	do {
		m_peak_start--;
	} while (points[(int) m_peak_start] < m_ped_mean / fadc_analizer::LSB);
	//3: the calculatino: move end forward until we reach the pedestal mean
	do {
		m_peak_end++;
	} while (points[(int) m_peak_end] < m_ped_mean / fadc_analizer::LSB);

	//4: a correction
	m_peak_start++;
	m_peak_end--;

	//5: lets check we are within limits!
	if (m_peak_start < 0) m_peak_start = 0;
	if (m_peak_end >= m_SamplesPerChannel) m_peak_end = m_SamplesPerChannel - 1;

	m_peak_start *= dT;
	m_peak_end *= dT;

	return 0;

}
;

/*Here goes the functions that get back variables to the user*/

double fadc_analizer::GetPedMean() {
	if (m_setup_done != 0) return -1;
	return m_ped_mean;
}

double fadc_analizer::GetPedSigma() {
	if (m_setup_done != 0) return -1;
	return m_ped_sigma;
}

double fadc_analizer::GetPeakPosition() {
	if (m_setup_done != 0) return -1;
	return m_peak_pos;
}

double fadc_analizer::GetPeakValue() {
	if (m_setup_done != 0) return -1;
	return m_peak_val;
}

double fadc_analizer::GetPeakStart() {
	if (m_setup_done != 0) return -1;
	return m_peak_start;
}
double fadc_analizer::GetPeakEnd() {
	if (m_setup_done != 0) return -1;
	return m_peak_end;
}

/*Here are the more complicated get-back functions for the energy.
 1) Check if the chosen EnergyCalculator exists and is correctly initialized
 2) Use its methods to calculate the energy
 3) Get it back
 */

double fadc_analizer::GetEnergy(int method) {
	if (m_setup_done != 0) return -1000;
	if ((method < 0) || (method >= m_energy_calculators.size())) {
		return -3000;
	}

	//CELE checks here the correctness of the calculator we want to use
	return m_energy_calculators[method]->CalculateEnergy();
}

void fadc_analizer::PrintEnergyMethods() {
	cout << "Here are the available energy calculators: " << endl;
	for (int ii = 0; ii < m_energy_calculators.size(); ii++) {
		cout << ii << " : " << m_energy_calculators[ii]->GetName() << " --> " << m_energy_calculators[ii]->GetDescription() << endl;
	}

}


/*Here are the more complicated get-back functions for the time
 1) Check if the chosen TimeCalculator exists and is correctly initialized
 2) Use its methods to calculate the time
 3) Get it back
 */

double fadc_analizer::GetTime(int method) {
	if (m_setup_done != 0) return -1000;
	if ((method < 0) || (method >= m_time_calculators.size())) {
		return -3000;
	}

	//CELE checks here the correctness of the calculator we want to use
	return m_time_calculators[method]->CalculateTime();
}

void fadc_analizer::PrintTimeMethods() {
	cout << "Here are the available time calculators: ( " << m_time_calculators.size() << " )" << endl;
	for (int ii = 0; ii < m_time_calculators.size(); ii++) {
		cout << ii << " : " << m_time_calculators[ii]->GetName() << " --> " << m_time_calculators[ii]->GetDescription() << endl;
	}

}

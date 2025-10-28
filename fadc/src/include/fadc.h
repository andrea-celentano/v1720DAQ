#ifndef FADC_GUARD
#define FADC_GUARD

#include <string>
#include <vector>
using namespace std;


//here is the main FADC class


class fadc_analizer
{
  
  class energy_calculator //energy calculator nested class
  {
  private:
    fadc_analizer& m_fadc;
    std::string m_name;
    std::string m_description;
   
    double energy;
    double (*m_function)(unsigned short *points,double *par);

  public: 
    //first creator, no cal. constants
    energy_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description);
    int SetFunction(double (*fun)(unsigned short *points,double *par)); //return is error code
    double CalculateEnergy();
    


    inline std::string GetName(){return m_name;}
    inline std::string GetDescription(){return m_description;}
    
  };


 
  class time_calculator //time calculator nested class
  {
  private:
    fadc_analizer& m_fadc;
    std::string m_name;
    std::string m_description;
   
    double time; //in ns
    double (*m_function)(unsigned short *points,double *par);
    

  public: 
    //first creator, no cal. constants
    time_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description);
    int SetFunction(double (*fun)(unsigned short *points,double *par)); //return is error code
    double CalculateTime();
 
    
    inline std::string GetName(){return m_name;}
    inline std::string GetDescription(){return m_description;}
    
  };







private:



 
  /*
    Has the Setup being done correctly?
    0: Correctly done
    1: Not yet done
    -1: Error code
  */
  int m_setup_done;
  /* 
     The pointer to the array containing the fadc points.
     You never touch it. Use LoadEvent method instead
  */
  unsigned short *points;
  int m_SamplesPerChannel; //number of samples FOR EACH CHANNEL!!!
  /*
    The variables that are used in the calculations (as inputs and outputs)
    Can't touch them directly, use Setup method instead.
    To get back a variable, first use Calculate() to process the event,
    then use GetX() to get variable X.
    For the energies, we don't store them here, rately we use directly the GetEnergy() method.    
  */

  int m_ped_width;

  
  double m_ped_mean,m_ped_sigma;
  double m_peak_pos, m_peak_val;
  double m_peak_start,m_peak_end;

  energy_calculator ECalculator1; //the first energy calculator
  energy_calculator ECalculator2; //the first energy calculator
  time_calculator TCalculator1;

  vector<energy_calculator*> m_energy_calculators; //the vector we need to use for the energy calculators (just the pointers!)
  vector<time_calculator*> m_time_calculators; //the vector we need to use for the time calculators
public:
 
  fadc_analizer();
  ~fadc_analizer();
  void Setup(int PedWidth=60);
  
  int LoadEvent(unsigned short *samples,unsigned int N); //the function to load data for the event
  int ProcessEvent(); //the function to tell the class to do the common calculations


  /*Here are the function that calculates things*/
  /*They save everything in the relevant private variables*/
  int CalculatePedestal(); //calculate pedestal mean and sigma in mV
  int CalculatePeak(); //the peak position in ns and the peak value in mV
  int CalculatePeakLimits(); //the pulse start and end in ns (integration limits)
  int CalculateEnergy(int method);



  //here are the functions to get back what we (commonly used variables);
  double GetPedMean(); //in samples
  double GetPedSigma(); //in samples
  double GetPeakPosition(); //in samples
  double GetPeakValue(); //in samples
  double GetPeakStart(); //in samples
  double GetPeakEnd(); //in samples

  //The same as above, but as c vector (for all channels)
  

  //here is the function to get back the energy
  double GetEnergy(int method);
  void PrintEnergyMethods();
  
 //here is the function to get back the timw
  double GetTime(int method);
  void PrintTimeMethods();




};







#endif

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
    double (*m_function)(int *points,double *par);
    vector <double> m_calibration;

  public: 
    //first creator, no cal. constants
    energy_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description);
    int SetFunction(double (*fun)(int *points,double *par)); //return is error code
    double CalculateEnergy(int ch);
    
    int SetCalConstant(int ch,double o_costant);
    int SetCalConstants(vector <double> o_calibration); //set the cal.constants
    int SetCalConstants(double *o_calibration); //set the cal.constants
    
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
    double (*m_function)(int *points,double *par);
    

  public: 
    //first creator, no cal. constants
    time_calculator(fadc_analizer& o_fadc,const std::string& name,const std::string& description);
    int SetFunction(double (*fun)(int *points,double *par)); //return is error code
    double CalculateTime(int ch);
 
    
    inline std::string GetName(){return m_name;}
    inline std::string GetDescription(){return m_description;}
    
  };







private:

  int m_Nchannels;


 
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
  vector < vector <int > > *points;
  int m_SamplesPerChannel; //number of samples FOR EACH CHANNEL!!!
  /*
    The variables that are used in the calculations (as inputs and outputs)
    Can't touch them directly, use Setup method instead.
    To get back a variable, first use Calculate() to process the event,
    then use GetX() to get variable X.
    For the energies, we don't store them here, rately we use directly the GetEnergy() method.    
  */

  int m_ped_width;

  
  double *m_ped_mean,*m_ped_sigma;
  double *m_peak_pos, *m_peak_val;
  double *m_peak_start,*m_peak_end;

  energy_calculator ECalculator1; //the first energy calculator
  energy_calculator ECalculator2; //the first energy calculator
  time_calculator TCalculator1;

  vector<energy_calculator*> m_energy_calculators; //the vector we need to use for the energy calculators (just the pointers!)
  vector<time_calculator*> m_time_calculators; //the vector we need to use for the time calculators
public:
  /* Some global "constants" */
  /* You never touch them (12 bit digitizer 250MHz clock) */

  static constexpr double LSB=0.4884;
  static constexpr double R=50;
  static constexpr double dT=4;





  fadc_analizer(int Nchannels);
  ~fadc_analizer();
  void Setup(int SamplesPerChannel=200,int PedWidth=60);
  
  int LoadEvent(vector <vector <int> > *samples); //the function to load data for the event
  int ProcessEvent(); //the function to tell the class to do the common calculations


  /*Here are the function that calculates things*/
  /*They save everything in the relevant private variables*/
  int CalculatePedestal(int ch); //calculate pedestal mean and sigma in mV
  int CalculatePeak(int ch); //the peak position in ns and the peak value in mV
  int CalculatePeakLimits(int ch); //the pulse start and end in ns (integration limits)   
  int CalculateEnergy(int ch,int method);



  //here are the functions to get back what we (commonly used variables);
  double GetPedMean(int ch); //in mV
  double GetPedSigma(int ch); //in mV
  double GetPeakPosition(int ch); //in ns
  double GetPeakValue(int ch); //in mV
  double GetPeakStart(int ch); //in ns
  double GetPeakEnd(int ch); //in ns

  //The same as above, but as c vector (for all channels)
  
  double* GetPedMean(); //in mV
  double* GetPedSigma(); //in mV
  double* GetPeakPosition(); //in ns
  double* GetPeakValue(); //in mV
  double* GetPeakStart(); //in ns
  double* GetPeakEnd(); //in ns

  //here is the function to get back the energy
  double GetEnergy(int ch,int method);
  void PrintEnergyMethods();
  

  //here goes the functions to load the cal.constants for the various energy calculators.
  //first prototype: read them from a file (1 number for each channels of fadc).
  int LoadCalConstants(int method,std::string o_fname);
  //for all calculators
  inline int LoadCalConstantsAll(std::string o_fname){int ret=0;for (int ii=0;ii<m_energy_calculators.size();ii++) ret+=LoadCalConstants(ii,o_fname);return ret;};


 //here is the function to get back the timw
  double GetTime(int ch,int method);
  void PrintTimeMethods();




};







#endif

#ifndef inl_helper_H
#define inl_helper_H

#include "datatypes_global.h"

#include <vector>
#include <string>
#include <iostream>       // Stream to consol (may be needed for files too???)
#include <cmath>        // floor, abs
#include <random>

#include <vector>
#include <string>

struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};


#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)

std::vector<std::string> tokenize(std::string s, std::string delim = ",");


//#############################################################################
//                          linear_regression
//#############################################################################

struct xy_point
{
    const double x;
    const double y;
    const double w;

    xy_point(const double& x, const double& y, const double& w) : x(x), y(y), w(w) {}
};

struct lin_reg_slope_yinter
{
    const double m;
    const double b;

    lin_reg_slope_yinter(const double& m, const double& b) : m(m), b(b) {}
};

class linear_regression
{
public:
    static lin_reg_slope_yinter  non_weighted(const std::vector<xy_point>& points);
    static lin_reg_slope_yinter  weighted(const std::vector<xy_point>& points);
};


//##########################################################
//                      line_segment
//##########################################################

struct line_segment
{
    // y = a*x + b
    double x_LB;
    double x_UB;
    double a;
    double b;
   
    const double y(const double x) const { return this->a*x + this->b; }
    const double y_LB() const { return this->y(this->x_LB); }
    const double y_UB() const { return this->y(this->x_UB); }
    
    bool operator<(const line_segment& rhs) const
    {
        return this->x_LB < rhs.x_LB;
    }

    bool operator<(line_segment& rhs) const
    {
        return this->x_LB < rhs.x_LB;
    }

    line_segment(const double x_LB, const double x_UB, const double a, const double b)
        : x_LB(x_LB), x_UB(x_UB), a(a), b(b) {}
    
    line_segment( const std::pair<double,double> x0y0, std::pair<double,double> x1y1 )
        : x_LB(0), x_UB(0), a(0), b(0)
    {
        const double x0 = x0y0.first;
        const double x1 = x1y1.first;
        const double y0 = x0y0.second;
        const double y1 = x1y1.second;
        
        this->a = (y1-y0)/(x1-x0);
        this->b = y0 - this->a*x0;
        this->x_LB = x0;
        this->x_UB = x1;
    }
};
std::ostream& operator<<(std::ostream& out, const line_segment& x);



//##########################################################
//                      SOC_vs_P2
//##########################################################

struct SOC_vs_P2
{
    const std::vector<line_segment> curve;
    const double zero_slope_threshold;

    SOC_vs_P2() : curve(std::vector<line_segment>()), zero_slope_threshold(0.0) {}
    SOC_vs_P2(const std::vector<line_segment>& curve,
              const double& zero_slope_threshold);
    
    double eval( const double x ) const
    {
        for( const line_segment& ls : curve )
        {
            if( x >= ls.x_LB && x <= ls.x_UB )
            {
                return ls.y(x);
            }
        }
        std::cout << "ERROR in 'SOC_vs_P2::eval'. x out of range." << std::endl;
        exit(1);
        return 0.0;
    }
    
    double xmin() const
    {
        return this->curve.at(0).x_LB;
    }
    
    double xmax() const
    {
        return this->curve.at(this->curve.size()-1).x_UB;
    }
};
std::ostream& operator<<(std::ostream& out, const SOC_vs_P2& x);



//#############################################################################
//                          poly_function_of_x
//#############################################################################

enum class poly_degree
{
    first,
    second,
    third,
    fourth
};


struct poly_segment
{
    double x_LB;
    double x_UB;
    poly_degree degree;
    double a;
    double b;
    double c;
    double d;
    double e;

    poly_segment(const double& x_LB, const double& x_UB, const poly_degree& degree, const double& a,
        const double& b, const double& c, const double& d, const double& e)
        : x_LB(x_LB), x_UB(x_UB), degree(degree), a(a), b(b), c(c), d(d), e(e) {}
    
    bool operator<(const poly_segment &rhs) const
    {
        return x_LB < rhs.x_LB;
    }

    bool operator<(poly_segment &rhs) const
    {
        return x_LB < rhs.x_LB;
    }
}; 


struct poly_function_of_x_args
{
    double x_tolerance;
    bool take_abs_of_x;
    bool if_x_is_out_of_bounds_print_warning_message;
    std::vector<poly_segment> segments;
    std::string warning_msg_poly_function_name;
};


class poly_function_of_x
{
private:
    std::vector<poly_segment> segments;
    double x_LB, x_UB, a, b, c, d, e;
    int index;
    poly_degree degree;
    double x_tolerance;
    bool take_abs_of_x;
    std::string warning_msg_poly_function_name;
    bool if_x_is_out_of_bounds_print_warning_message;
    
public:
    poly_function_of_x(){}
    poly_function_of_x(double x_tolerance_, bool take_abs_of_x_, bool if_x_is_out_of_bounds_print_warning_message_, const std::vector<poly_segment> &segments_, std::string warning_msg_poly_function_name_);
    double get_val(double x);
};

//#############################################################################
//                              Functions
//#############################################################################

// splits a 'char' delimited string into tokens
std::vector<std::string> split(const std::string& line, char delim);


//#############################################################################
//                                 Random
//#############################################################################

class rand_val
{
public:
    static double rand_range(double min, double max);
    static double rand_zero_to_one();
                               
private:
    static bool rand_is_initialized;        
    static void init_rand();    
};


//#############################################################################
//                               Low Pass Filter
//#############################################################################


class LPF_kernel
{
private:
    
    int max_window_size;

    // raw_data has a max length of max_window_size.
    // new value added to the raw_data is added in a round-robin fashin. 
    std::vector<double> raw_data;

    // cur_raw_data_index tracks the latest info added to the round-robin raw_data.
    int cur_raw_data_index;
    
    // Options are Hanning, Blackmann, and Rectangular.
    LPF_window_enum window_type;

    int window_size;
    
    std::vector<double> window;
    double window_area;
    
public:
    LPF_kernel();
    LPF_kernel(int max_window_size, double initial_raw_data_value);
    void update_LPF(LPF_parameters& LPF_params);
    void add_raw_data_value(double next_input_value);
    double get_filtered_value();
};


//#############################################################################
//                       Get Base Load Forecast
//#############################################################################

class get_base_load_forecast
{
private:
    double adjustment_interval_hrs;
    double default_value_akW;
    double data_start_unix_time ;
    int data_timestep_sec;
    std::vector<double> actual_load_akW;
    std::vector<double> forecast_load_akW; 

public:
    get_base_load_forecast() {};
    get_base_load_forecast( const double data_start_unix_time_, 
                            const int data_timestep_sec_, 
                            const std::vector<double>& actual_load_akW_, 
                            const std::vector<double>& forecast_load_akW_, 
                            const double adjustment_interval_hrs_ );
    std::vector<double> get_forecast_akW( const double unix_start_time, 
                                          const int forecast_timestep_mins, 
                                          const double forecast_duration_hrs ) const;
};


//#############################################################################
//                        Get Probability Values
//#############################################################################


class get_value_from_normal_distribution
{
private:
    double mean;
    double stdev;
    double stdev_bounds;
    
    std::mt19937 gen;
    std::normal_distribution<double> dis; 
    
public:
    get_value_from_normal_distribution() {};
    get_value_from_normal_distribution(int seed, double mean_, double stdev_, double stdev_bounds_);
    double get_value();
};


class get_real_value_from_uniform_distribution
{
private:
    std::mt19937 gen;
    std::uniform_real_distribution<double> dis; 
    
public:
    get_real_value_from_uniform_distribution() {};
    get_real_value_from_uniform_distribution(int seed, double LB, double UB);
    double get_value();
};


class get_int_value_from_uniform_distribution
{
private:
    std::mt19937 gen;
    std::uniform_int_distribution<int> dis;
    
public:
    get_int_value_from_uniform_distribution() {};
    get_int_value_from_uniform_distribution(int seed, int LB, int UB);
    int get_value();
};


class get_bernoulli_success
{
private:
    double p_val;
    std::mt19937 gen;
    std::uniform_real_distribution<double> dis; 
    
public:
    get_bernoulli_success() {};
    get_bernoulli_success(int seed, double p);
    bool is_success();
};


#endif







/*


#ifndef _inl_helper_H
#define _inl_helper_H

#include <cstddef>
#include <map>
#include <stdexcept>

#include <sstream>    // String manipulation

#include <sys/time.h> // used by function get_real_time()

#include <cstdlib>
#include <ctime>


enum dcfc_enum
{
    dcfc_50kW_125A_LV_ABB_Terra53CJ = 0,
    dcfc_320kW_500A_HV_ABB_TerraHP_SAE = 1,
    dcfc_100kW_200A_LV_ABB_TerraHP_Chatemo = 2,
    dcfc_200kW_300A_HV = 3,
    dcfc_500kW_600A_HV = 4
};


enum pev_enum
{
    pev_52pkW_22kWh_2c_132A_LMO_LV = 0,
    pev_68pkW_54kWh_1c_176A_NMC_LV = 1,
    pev_203pkW_54kWh_3c_390A_NMC_LV = 2,
    pev_368pkW_98kWh_3c_360A_NMC_HV = 3,
    pev_431pkW_115kWh_3c_399A_NMC_HV = 4,
    pev_214pkW_38kWh_5c_200A_LTO_HV = 5,
    pev_188pkW_150kWh_1c_385A_NMC_LV = 6,
    pev_363pkW_150kWh_2c_385A_NMC_HV = 7
};


enum site_bat_enum
{
    site_bat_100kWh_350kW_LMO
//    site_bat_200kWh_500kW,
//    site_bat_1000kWh_750kW        
};


class enum_conversion
{
public:       
    static pev_enum  get_pev_enum(const std::string pev_str, std::string invalid_enum_str);
    static dcfc_enum  get_dcfc_enum(const std::string dcfc_str, std::string invalid_enum_str);
    static site_bat_enum  get_site_bat_enum(const std::string site_bat_str, std::string invalid_enum_str);
    static bool pev_is_HV(pev_enum pev_type);
    static bool dcfc_is_HV(dcfc_enum dcfc_type);
};       



struct dcfc_charge_event
{
    int dcfc_id;
    pev_enum PEV_type;
    double arrival_unix_time;
    double departure_unix_time;
    double arrival_SOC;
    double departure_SOC;
    bool stop_charging_at_target_soc_even_if_not_depart_time;
    
    bool operator<(const dcfc_charge_event& x) const
    {
        return arrival_unix_time < x.arrival_unix_time;
    }

    bool operator<(dcfc_charge_event& x) const
    {
        return arrival_unix_time < x.arrival_unix_time;
    }
};


struct dcfc_charge_event_control_input
{
    int dcfc_id;
    dcfc_enum dcfc_type;
    pev_enum PEV_type;
    double arrival_unix_time;
    double departure_unix_time;
    double arrival_SOC;
    double departure_SOC;
    bool stop_charging_at_target_soc_even_if_not_depart_time;
    bool is_charging_now;
    double cur_dcP14_kW;
    double target_dcP14_kW;
};


class dcfc;
struct dcfc_info
{
    int dcfc_id;
    dcfc* dcfc_ptr;
    dcfc_enum dcfc_type;
    bool use_charge_rate_transitions;
};


class site_battery;
struct site_bat_info
{
    int site_bat_id;
    site_battery* site_bat_ptr;
    site_bat_enum site_bat_type;
    bool use_charge_rate_transitions;
};
        

struct bat_objfun_constraints
{
    double a;
    double b;
};


struct all_bat_objfun_constraints
{
    std::vector<bat_objfun_constraints> charging_constraints;
    std::vector<bat_objfun_constraints> discharging_constraints;  
};

        
struct cumulative_energy
{
    double acE_kWh_bat_loss;
    double acE_kWh_inv_loss;
    double acE_kWh_in;
    double acE_kWh_out;
};


enum battery_charging_status
{
    off, 
    charging,
    discharging
};
        
        
struct battery_state
{
    double soc;
    battery_charging_status charging_status;
    double cur_dcP14_kW;
    double target_dcP14_kW;
    double max_calculation_timestep_hrs;
};


struct power_struct
{
    double soc_t1;
    double acP_kW;
    double acQ_kVAR;
    double dcP_kW;
    double dcP_kW_bat_losses;
    double aprox_charge_time_hrs;
    double min_time_to_charge_hrs;
    bool charge_has_completed;
};


struct acPQ_struct
{
    double acP_kW;
    double acQ_kVAR;
};


std::ostream& operator<<(std::ostream& out, power_struct& x);

//#############################################################################
//                               LPF_kernel
//#############################################################################

enum LPF_window
{
    Hanning=0,
    Blackman=1
};

class LPF_kernel
{
private:
    std::vector<double> window;
    double window_area;
    
    std::vector<double> data;
    double cur_data_index;

public:
    LPF_kernel(int window_size, LPF_window window_type);
    double get_filtered_value(double next_input_value);
};


//#############################################################################
//                          log warnings and errors
//#############################################################################

class log_warnings_and_errors
{
private:
    static bool log_streams_have_been_initialized;
    static std::ofstream warnings_log;
    static std::ofstream errors_log;
    static void init_log_streams();
    
public:
    static std::ofstream& get_warnings_ofstream();
    static std::ofstream& get_errors_ofstream();
};


//#############################################################################
//                                 Functions
//#############################################################################

// splits a 'char' delimited string into tokens
std::vector<std::string> split(const std::string& line, char delim);

double get_real_time();




//#############################################################################
//                          linear_regression
//#############################################################################

struct xy_point 
{
    double x;
    double y;
    double w;        
};

struct lin_reg_slope_yinter
{
    double m;
    double b;
};

class linear_regression
{
public:
    static lin_reg_slope_yinter  non_weighted(const std::vector<xy_point> &points);
    static lin_reg_slope_yinter  weighted(const std::vector<xy_point> &points);
};


//#############################################################################
//                         P_vs_time_profile
//#############################################################################

class P_vs_time_profile
{    
    //  \ <--------- z ---------> \ <--------- z ---------> \ <--------- z ---------> \
    //  \      index = 0          \        index = 1        \         index = 2       \
    //  \                         \                         \                         \
    //  & <------- x -------> *   \                         \ <--- y ---> @           \
    
    //  & -> unix_ref_time
    //  * -> start_unix_time (time_now or arrival_time)
    //  @ -> end_unix_time    (depart_time or time_reached_target_soc if earlier than depart_time)
    //  x -> data_start_offset_sec
    //  y -> data_end_offset_sec
    //  z -> time_step_sec

private:
    double unix_ref_time, time_step_sec, data_start_offset_sec, data_end_offset_sec;
    bool is_charge_not_discharge_profile;
    
    std::vector<double> soc;
    std::vector<double> dcP_kW;
    std::vector<double> dcP_kW_bat_losses;
    std::vector<double> acP_kW;
    std::vector<double> acQ_kVAR;
    std::vector<double> unix_times;
    std::vector<double> min_time_to_charge_hrs;
    std::vector<double> aprox_charge_time_hrs;
    
    bool times_have_been_set, vectors_have_been_set;
    
public:
    P_vs_time_profile()
    {
        times_have_been_set = false;
        vectors_have_been_set = false;
    }

    bool get_is_charge_not_discharge_profile();
    double get_time_step_sec();
    double get_data_start_offset_sec();
    double get_data_end_offset_sec();
    double get_min_time_to_charge_val_hrs();
    
    const std::vector<double>& get_soc();
    const std::vector<double>& get_dcP_kW();
    const std::vector<double>& get_dcP_kW_bat_losses();
    const std::vector<double>& get_acP_kW();
    const std::vector<double>& get_acQ_kVAR();
    const std::vector<double>& get_unix_times();
    const std::vector<double>& get_min_time_to_charge_hrs();
    const std::vector<double>& get_aprox_charge_time_hrs();  // Used to calculate avgP from Energy

    void set_times(int num_timesteps, double time_step_sec_, double unix_ref_time_, double data_start_offset_sec_, double data_end_offset_sec_);
    void set_vectors(bool is_charge_not_discharge_profile_, const std::vector<double> &soc_, const std::vector<double> &dcP_kW_, const std::vector<double> &dcP_kW_bat_losses_,
                     const std::vector<double> &acP_kW_, const std::vector<double> &acQ_kVAR_, const std::vector<double> &min_time_to_charge_hrs_, 
                     const std::vector<double> &aprox_charge_time_hrs_);
};

std::ostream& operator<<(std::ostream& out, P_vs_time_profile& x);

#endif

*/

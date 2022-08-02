

#include "helper.h"

#include <cmath>		// abs(), exp(), log(), atan(), cos()
#include <algorithm>	// sort()
#include <sstream> 	  	// used to parse lines
#include <ctime>		// time


std::ostream& operator<<(std::ostream& out, line_segment& z)
{
	out << z.x_LB << "," << z.a*z.x_LB + z.b << std::endl;
	out << z.x_UB << "," << z.a*z.x_UB + z.b << std::endl;
	return out;
}

//#############################################################################
//                           poly_function_of_x
//#############################################################################

poly_function_of_x::poly_function_of_x(double x_tolerance_, bool take_abs_of_x_, bool if_x_is_out_of_bounds_print_warning_message_, const std::vector<poly_segment> &segments_, std::string warning_msg_poly_function_name_)
{
    segments = segments_;
    std::sort(segments.begin(), segments.end());
    
    x_tolerance = x_tolerance_;
    take_abs_of_x = take_abs_of_x_;
    if_x_is_out_of_bounds_print_warning_message = if_x_is_out_of_bounds_print_warning_message_;
    warning_msg_poly_function_name = warning_msg_poly_function_name_;

    index = 0;
    x_LB = segments[index].x_LB;
    x_UB = segments[index].x_UB;
    a = segments[index].a;
    b = segments[index].b;
    c = segments[index].c;
    d = segments[index].d;
    e = segments[index].e;
    degree = segments[index].degree;
}


double poly_function_of_x::get_val(double x)
{
	if(take_abs_of_x)
		x = std::abs(x);

    //--------------------
    
    bool x_out_of_bounds = false;
    
    if(index == 0 && x < x_LB)
    {
        x = x_LB;
        x_out_of_bounds = true;
    }
    else if(index == segments.size()-1 && x > x_UB)
    {
        x = x_UB;
        x_out_of_bounds = true;
    }
    else if(x < x_LB || x_UB < x)
    {
        bool index_set = false;
        for(unsigned int i = 0; i < segments.size(); i++)
        {
            if(segments[i].x_LB - x_tolerance <= x && x <= segments[i].x_UB + x_tolerance)
            {
                index = i;
                index_set = true;
                break;
            }
        }
        
        if(!index_set)
        {
            x_out_of_bounds = true;
            
        	if(x <= segments[0].x_LB)
            {
        		index = 0;
                x = segments[index].x_LB;
            }
        	else
            {
        		index = segments.size() - 1;
                x = segments[index].x_UB;
            }
        }
        
        x_LB = segments[index].x_LB;
        x_UB = segments[index].x_UB;
        a = segments[index].a;
        b = segments[index].b;
        c = segments[index].c;
        d = segments[index].d;
        e = segments[index].e;
        degree = segments[index].degree;
    }
    
    //-------------------------------------------
    
    if(x_out_of_bounds && if_x_is_out_of_bounds_print_warning_message)
    {
        std::string msg = "poly_function_of_x::get_val,  name:" + warning_msg_poly_function_name + "  msg:x_val out of range.  x_val:" + std::to_string(x);
        std::cout << msg << std::endl;
    }
    
    //-------------------------------------------    
    
    if(degree == first)
        return a*x + b;
    else if(degree == second)
        return a*x*x + b*x + c;
    else
    {
        double x_2 = x*x;
        double x_3 = x*x_2;
        
        if(degree == third)
            return a*x_3 + b*x_2 + c*x + d;
        else if(degree == fourth)
        {
            double x_4 = x*x_3;
            return a*x_4 + b*x_3 + c*x_2 + d*x + e;
        }
    }
    
    return -1;
}

//#############################################################################
//                              Functions
//#############################################################################

// splits a 'char' delimited string into tokens
std::vector<std::string> split(const std::string& line, char delim)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(line);
   
   while (std::getline(tokenStream, token, delim))
   {
      tokens.push_back(token);
   }
   
   return tokens;
}


//#############################################################################
//                                 Random
//#############################################################################

void rand_val::init_rand()
{
	srand(time(NULL));
}

double rand_val::rand_range(double min, double max)
{
    if(!rand_is_initialized)
    {
        rand_is_initialized = true;
        init_rand();
    }
    
	return min + rand_zero_to_one() * (max-min);
}
	
double rand_val::rand_zero_to_one()
{
    if(!rand_is_initialized)
    {
        rand_is_initialized = true;
        init_rand();
    }
    
	return (double)(rand() % RAND_MAX) / (double)RAND_MAX;
}

bool rand_val::rand_is_initialized = false;


//#############################################################################
//                               Low Pass Filter
//#############################################################################


LPF_kernel::LPF_kernel(int max_window_size, double initial_raw_data_value)
{
    this->cur_raw_data_index = 0;
    
    for(int i=0; i<max_window_size; i++)
        this->raw_data.push_back(initial_raw_data_value);
    
    LPF_parameters LPF_params;
    LPF_params.window_size = 1;
    LPF_params.window_type = Rectangular;
    update_LPF(LPF_params);
}


void LPF_kernel::update_LPF(LPF_parameters& LPF_params)
{
    this->window_size = LPF_params.window_size;
    this->window_type = LPF_params.window_type;
    this->window.clear();  // This is fixing a bug, is necessary when using window.resize(). 
    
    //----------------------
    
    if(this->window_size > this->raw_data.size())
        std::cout << "ERROR.  Low pass filter window size can not be greater than raw data size." << std::endl;
    
    if(this->window_type == Rectangular && this->window_size < 1)
    {
        this->window_size = 1;
        std::cout << "ERROR.  For a Rectangular Filter window size can not be less than 1." << std::endl;
    }
    
    if(this->window_size < 2 && (this->window_type == Hanning || this->window_type == Blackman))
    {    
        this->window_size = 2;
        std::cout << "ERROR.  For a Hanning and Blackman Filter window size can not be less than 2." << std::endl;
    }
    
	//----------------------
    
    if(this->window_type == Rectangular)
    {
        this->window.resize(this->window_size, 1.0);
        this->window_area = this->window_size;
    }
    else if(this->window_size == 2)
    {
        // For a Blackman and Hanning window the window size of 2 means that the
        // signal is not filtered at all.  It is the same as a Rectangular window
        // with a window of size 1.
        
        this->window_size = 1;
        this->window.resize(this->window_size, 1.0);
        this->window_area = this->window_size;
    }
    else
    {
        // When n=0 or n=N-1 the Hanning and Blackman Filter values are 0.
        
        this->window.resize(this->window_size, 0);
        
        // Blackman Window Constants
        double alpha, a0, a1, a2;
        alpha = 0.16;
        a0 = (1-alpha)/2;
        a1 = 0.5;
        a2 = alpha/2;
        
        //------------
        
        double window_val;
        double pi = 4*std::atan(1);
        double N_minus_1 = this->window_size - 1;
        this->window_area = 0;
        
        for(int n=0; n < this->window_size; n++) // When n=0 or n=N-1 the Hanning and Blackman Filter values are 0.
        {
            if(n==0 || n==N_minus_1)
                window_val = 0;
            else if(this->window_type == Hanning)
                window_val = 0.5*(1 - std::cos(2*pi*n/N_minus_1));
            else if(this->window_type == Blackman)
                window_val = a0 - a1*std::cos(2*pi*n/N_minus_1) + a2*std::cos(4*pi*n/N_minus_1);
            
            this->window_area += window_val;
            this->window[n] = window_val;
        }
    }
}


void LPF_kernel::add_raw_data_value(double next_input_value)
{
    int raw_data_size_minus_1 = this->raw_data.size()-1;
    
    //---------------------
    //   Update Raw Data
    //---------------------
    this->cur_raw_data_index = (this->cur_raw_data_index == raw_data_size_minus_1) ? 0 : this->cur_raw_data_index+1;
    this->raw_data[this->cur_raw_data_index] = next_input_value;
}


double LPF_kernel::get_filtered_value()
{
    double X = 0;    
    int data_index = this->cur_raw_data_index;
    int raw_data_size_minus_1 = this->raw_data.size()-1;
    
    for(int i=0; i < this->window_size; i++)
    {
        X += this->window[i] * this->raw_data[data_index];
        data_index = (data_index == 0) ? raw_data_size_minus_1 : data_index-1;
    }
 
    return X/this->window_area;
}


//#############################################################################
//                       Get Base Load Forecast
//#############################################################################

get_base_load_forecast::get_base_load_forecast(double data_start_unix_time_, int data_timestep_sec_, std::vector<double>& actual_load_akW_, std::vector<double>& forecast_load_akW_, double adjustment_interval_hrs_)
{
    this->adjustment_interval_hrs = adjustment_interval_hrs_;
    this->data_start_unix_time = data_start_unix_time_;
    this->data_timestep_sec = data_timestep_sec_;
    this->actual_load_akW = actual_load_akW_;
    this->forecast_load_akW = forecast_load_akW_;
    
    double sum_val = 0;
    for(double x : this->forecast_load_akW)
        sum_val += x;
    
    if(0 < this->forecast_load_akW.size())
        this->default_value_akW = sum_val / this->forecast_load_akW.size();
    else
        this->default_value_akW = 0;
}


std::vector<double> get_base_load_forecast::get_forecast_akW(double unix_start_time, int forecast_timestep_mins, double forecast_duration_hrs)
{
    int num_data_steps_to_aggregate_for_each_forecast_step;
    if(0.001 < std::abs(60*forecast_timestep_mins % this->data_timestep_sec))
    {
        // Should throw an error here.
        std::cout << "CALDERA ERROR: get_base_load_forecast::get_forecast_akW().  Incompatible data and forecast timestep durations." << std::endl;
        num_data_steps_to_aggregate_for_each_forecast_step = 1;
    }
    else
        num_data_steps_to_aggregate_for_each_forecast_step = (int)std::round(60*forecast_timestep_mins/this->data_timestep_sec);
    
    //------------------------------
    
    int data_index = (int)std::floor((unix_start_time - this->data_start_unix_time)/(double)this->data_timestep_sec);
    int num_timesteps_in_forecast = (int)std::floor(60*forecast_duration_hrs/(double)forecast_timestep_mins);
    
    std::vector<double> forecast_akW;
    forecast_akW.resize(num_timesteps_in_forecast);
    
    double tmp_time_hrs, w, sum_akW;
    
    for(int forecast_index=0; forecast_index<num_timesteps_in_forecast; forecast_index++)
    {
        tmp_time_hrs = (forecast_index + 0.5)*( ((double)forecast_timestep_mins)/60.0 );
        
        if(this->adjustment_interval_hrs < tmp_time_hrs)
            w = 1;
        else
            w = tmp_time_hrs / this->adjustment_interval_hrs;
        
        //-----------------
        
        sum_akW = 0;
        for(int i=0; i<num_data_steps_to_aggregate_for_each_forecast_step; i++)
        {
            if(data_index < this->actual_load_akW.size())
            {
                sum_akW += (1-w)*this->actual_load_akW[data_index] + w*this->forecast_load_akW[data_index];
                data_index += 1;
            }
            else
                sum_akW += this->default_value_akW;
        }
        
        forecast_akW[forecast_index] = sum_akW/(double)num_data_steps_to_aggregate_for_each_forecast_step;
    }
    
    return forecast_akW;
}

//#############################################################################
//                        Get Probability Values
//#############################################################################

//--------------------------------------
//  get_value_from_normal_distribution
//--------------------------------------
get_value_from_normal_distribution::get_value_from_normal_distribution(int seed, double mean_, double stdev_, double stdev_bounds_)
{
    this->mean = mean_;
    this->stdev = stdev_;
    this->stdev_bounds = stdev_bounds_;
    
    if(seed > 0)
    {
        std::mt19937 X1(seed);
        this->gen = X1;
    }
    else
    {
        std::random_device rd;
        std::mt19937 X1(rd());
        this->gen = X1;
    }
    
    std::normal_distribution<double> Y1(0, 1);  // mean = 0, stdev = 1 
    this->dis = Y1;
}


double get_value_from_normal_distribution::get_value()
{
    double random_val;
    while(true)
    {
        //#pragma omp critical
        //{
            random_val = this->dis(this->gen);
        //}
        if(std::abs(random_val) <= this->stdev_bounds)
            return this->stdev*random_val + this->mean;
    }
}

//--------------------------------------------
//  get_real_value_from_uniform_distribution
//--------------------------------------------
get_real_value_from_uniform_distribution::get_real_value_from_uniform_distribution(int seed, double LB, double UB)
{
    if(seed > 0)
    {
        std::mt19937 X1(seed);
        this->gen = X1;
    }
    else
    {
        std::random_device rd;
        std::mt19937 X1(rd());
        this->gen = X1;
    }
    
    std::uniform_real_distribution<double> Y1(LB, UB);
    this->dis = Y1;
}


double get_real_value_from_uniform_distribution::get_value()
{    
    double random_val = this->dis(this->gen);
    return random_val;
}

//--------------------------------------------
//  get_int_value_from_uniform_distribution
//--------------------------------------------
get_int_value_from_uniform_distribution::get_int_value_from_uniform_distribution(int seed, int LB, int UB)
{
    if(seed > 0)
    {
        std::mt19937 X1(seed);
        this->gen = X1;
    }
    else
    {
        std::random_device rd;
        std::mt19937 X1(rd());
        this->gen = X1;
    }
    
    std::uniform_int_distribution<int> Y1(LB, UB);
    this->dis = Y1;
}


int get_int_value_from_uniform_distribution::get_value()
{    
    int random_val = this->dis(this->gen);
    return random_val;
}


//--------------------------------------
//         get_bernoulli_success
//--------------------------------------
get_bernoulli_success::get_bernoulli_success(int seed, double p)
{
    this->p_val = p;
    
    if(seed > 0)
    {
        std::mt19937 X1(seed);
        this->gen = X1;
    }
    else
    {
        std::random_device rd;
        std::mt19937 X1(rd());
        this->gen = X1;
    }
    
    std::uniform_real_distribution<double> Y1(0.0, 1.0);
    this->dis = Y1;
}


bool get_bernoulli_success::is_success()
{   
    double random_val = this->dis(this->gen);
    return (random_val <= this->p_val);
}






/*

#include "helper.h"

std::ostream& operator<<(std::ostream& out, power_struct& x)
{
    out << x.soc_t1 << "," << x.acP_kW << "," << x.dcP_kW << "," << x.acQ_kVAR << "," << x.dcP_kW_bat_losses << "," << x.aprox_charge_time_hrs << "," << x.min_time_to_charge_hrs << "," << x.charge_has_completed;
    return out;
} 

std::ostream& operator<<(std::ostream& out, P_vs_time_profile& x)
{
    std::vector<double> unix_time = x.get_unix_times();
	std::vector<double> soc = x.get_soc();
	std::vector<double> dcP = x.get_dcP_kW(); 
	std::vector<double> acP = x.get_acP_kW();
	std::vector<double> acQ = x.get_acQ_kVAR();
	std::vector<double> dcP_loss = x.get_dcP_kW_bat_losses();
	std::vector<double> approx_time = x.get_aprox_charge_time_hrs();
	std::vector<double> min_time = x.get_min_time_to_charge_hrs(); 

    for (unsigned int i = 0; i < soc.size(); i++)
    	out << unix_time[i] << "," << soc[i] << "," << acP[i] << "," << dcP[i] << "," << acQ[i] << "," << dcP_loss[i] << "," << approx_time[i] << "," << min_time[i] << std::endl;
    
    return out;
}

//#############################################################################
//                               LPF_kernel
//#############################################################################


LPF_kernel::LPF_kernel(int window_size, LPF_window window_type)
{
	// Blackman Window Constants
	double alpha, a0, a1, a2;
	alpha = 0.16;
	a0 = (1-alpha)/2;
	a1 = 0.5;
	a2 = alpha/2;
	
	//----------------------

	double pi, window_val;
	int N_minus_1;
		
	pi = 4*std::atan(1);
	N_minus_1 = window_size - 1;

	cur_data_index = 0;
	window_area = 0;
	
	for(int n=1; n<window_size-1; n++) // When n=0 or n=N-1 the Hanning and Blackman Filter values are 0.
	{
		if(window_type == Hanning)
			window_val = 0.5*(1 - std::cos(2*pi*n/N_minus_1));
		else if(window_type == Blackman)
			window_val = a0 - a1*std::cos(2*pi*n/N_minus_1) + a2*std::cos(4*pi*n/N_minus_1);
		
		window_area += window_val;
		window.push_back(window_val);
		data.push_back(0);
	}
}


double LPF_kernel::get_filtered_value(double next_input_value)
{
	int window_size, data_index;
	double next_output_value = 0;
	window_size = window.size();
	
	cur_data_index = (cur_data_index == window_size-1) ? 0 : cur_data_index+1;
	data[cur_data_index] = next_input_value;
	
	data_index = cur_data_index;
	for(int i=0; i<window_size; i++)
	{
		next_output_value += window[i]*data[data_index];
		data_index = (data_index == window_size-1) ? 0 : data_index+1;
	}
	
	return next_output_value/window_area;
}


//#############################################################################
//                          log warnings and errors
//#############################################################################

void log_warnings_and_errors::init_log_streams()
{
	log_streams_have_been_initialized = true;
	
	warnings_log << "info, message" << std::endl;
	errors_log << "info, message" << std::endl;
}

std::ofstream& log_warnings_and_errors::get_warnings_ofstream() 
{
	if(!log_streams_have_been_initialized)
		init_log_streams();
		
	return warnings_log;
}

std::ofstream& log_warnings_and_errors::get_errors_ofstream()
{
	if(!log_streams_have_been_initialized)
		init_log_streams();
		
	return errors_log;
}

std::ofstream log_warnings_and_errors::warnings_log("zzz_Warnings.csv");
std::ofstream log_warnings_and_errors::errors_log("zzz_Errors.csv");
bool log_warnings_and_errors::log_streams_have_been_initialized = false;

//#############################################################################
//                         Enum Conversion
//#############################################################################

pev_enum  enum_conversion::get_pev_enum(const std::string pev_str, std::string invalid_enum_str)
{
	std::string tmp_str = "";
	
	// make comparison case-insensitive and ignore whitespace
	for (int i = 0; i < pev_str.length(); i++)
		if (!std::isspace(pev_str[i]))
			tmp_str += pev_str[i];

	if 		(tmp_str == "pev_52pkW_22kWh_2c_132A_LMO_LV") return pev_52pkW_22kWh_2c_132A_LMO_LV;
	else if (tmp_str == "pev_68pkW_54kWh_1c_176A_NMC_LV") return pev_68pkW_54kWh_1c_176A_NMC_LV;
	else if (tmp_str == "pev_203pkW_54kWh_3c_390A_NMC_LV") return pev_203pkW_54kWh_3c_390A_NMC_LV;
	else if (tmp_str == "pev_368pkW_98kWh_3c_360A_NMC_HV") return pev_368pkW_98kWh_3c_360A_NMC_HV;
	else if (tmp_str == "pev_431pkW_115kWh_3c_399A_NMC_HV") return pev_431pkW_115kWh_3c_399A_NMC_HV;
	else if (tmp_str == "pev_214pkW_38kWh_5c_200A_LTO_HV") return pev_214pkW_38kWh_5c_200A_LTO_HV;
	else if (tmp_str == "pev_188pkW_150kWh_1c_385A_NMC_LV") return pev_188pkW_150kWh_1c_385A_NMC_LV;
	else if (tmp_str == "pev_363pkW_150kWh_2c_385A_NMC_HV") return pev_363pkW_150kWh_2c_385A_NMC_HV;
	else
	{ 
        std::string msg = "enum_conversion::get_pev_enum, " + invalid_enum_str + " value:" + tmp_str;
        std::ofstream& errors_ofstream = log_warnings_and_errors::get_errors_ofstream();
        errors_ofstream << msg << std::endl;
        
        throw(std::invalid_argument(msg));
    }
}


bool enum_conversion::pev_is_HV(pev_enum pev_type)
{
    bool return_val = true;
        
    if(pev_type == pev_52pkW_22kWh_2c_132A_LMO_LV || pev_type == pev_68pkW_54kWh_1c_176A_NMC_LV || pev_type == pev_203pkW_54kWh_3c_390A_NMC_LV || pev_type == pev_188pkW_150kWh_1c_385A_NMC_LV)
        return_val = false;
    
    return return_val;
}


dcfc_enum  enum_conversion::get_dcfc_enum(const std::string dcfc_str, std::string invalid_enum_str)
{
	std::string tmp_str = "";
	
	// make comparison case-insensitive and ignore whitespace
	for (int i = 0; i < dcfc_str.length(); i++)
		if (!std::isspace(dcfc_str[i]))
			tmp_str += dcfc_str[i];

	if     (tmp_str == "dcfc_50kW_125A_LV_ABB_Terra53CJ") return dcfc_50kW_125A_LV_ABB_Terra53CJ;
	else if(tmp_str == "dcfc_320kW_500A_HV_ABB_TerraHP_SAE") return dcfc_320kW_500A_HV_ABB_TerraHP_SAE;
	else if(tmp_str == "dcfc_100kW_200A_LV_ABB_TerraHP_Chatemo") return dcfc_100kW_200A_LV_ABB_TerraHP_Chatemo;
	else if(tmp_str == "dcfc_200kW_300A_HV") return dcfc_200kW_300A_HV;
	else if(tmp_str == "dcfc_500kW_600A_HV") return dcfc_500kW_600A_HV;
	else
	{ 
        std::string msg = "enum_conversion::get_dcfc_enum, " + invalid_enum_str + " value:" + tmp_str;
        std::ofstream& errors_ofstream = log_warnings_and_errors::get_errors_ofstream();
        errors_ofstream << msg << std::endl;
        
        throw(std::invalid_argument(msg));
    }
}


bool enum_conversion::dcfc_is_HV(dcfc_enum dcfc_type)
{
    bool return_val = true;
        
    if(dcfc_type == dcfc_50kW_125A_LV_ABB_Terra53CJ || dcfc_type == dcfc_100kW_200A_LV_ABB_TerraHP_Chatemo)
        return_val = false;
    
    return return_val;
}


site_bat_enum  enum_conversion::get_site_bat_enum(const std::string site_bat_str, std::string invalid_enum_str)
{
	std::string tmp_str = "";
	
	// make comparison case-insensitive and ignore whitespace
	for (int i = 0; i < site_bat_str.length(); i++)
		if (!std::isspace(site_bat_str[i]))
			tmp_str += site_bat_str[i];

	if(tmp_str == "site_bat_100kWh_350kW_LMO") return site_bat_100kWh_350kW_LMO;	
	else
	{ 
        std::string msg = "enum_conversion::get_site_bat_enum, " + invalid_enum_str + " value:" + tmp_str;
        std::ofstream& errors_ofstream = log_warnings_and_errors::get_errors_ofstream();
        errors_ofstream << msg << std::endl;
        
        throw(std::invalid_argument(msg));
    }
}






//#############################################################################
//                    transition_segement_description
//#############################################################################

transition_segement_description::transition_segement_description(poly_segment segment)
{
    a = segment.a;
    b = segment.b;
    c = segment.c;
    d = segment.d;
    e = segment.e;
    degree = segment.degree;
};


double transition_segement_description::get_val(double segment_time_sec)
{
    double x = segment_time_sec;
    
    if(degree == first)
        return a*x + b;
    else if(degree == second)
        return a*x*x + b*x + c;
    else
    {
        double x_2 = x*x;
        double x_3 = x*x_2;
            
        if(degree == third)
            return a*x_3 + b*x_2 + c*x + d;
        else if(degree == fourth)
        {
            double x_4 = x*x_3;
            return a*x_4 + b*x_3 + c*x_2 + d*x + e;
        }
    }
}

//#############################################################################
//                          Linear Regression
//#############################################################################

lin_reg_slope_yinter  linear_regression::non_weighted(const std::vector<xy_point> &points)
{
    double sum_x, sum_xx, sum_y, sum_xy, x, y;
    
    sum_x = 0;
    sum_xx = 0;
    sum_y = 0;
    sum_xy = 0;
    
    for(int i=0; i<points.size(); i++)
    {
        x = points[i].x;
        y = points[i].y;
    
        sum_x += x;
        sum_xx += x*x;
        sum_y += y;
        sum_xy += x*y;
    }
    
    //-------------------------------------------------------------------------------------------------
    //            \  n     sum_x  \                          \ sum_xx  -sum_x \           \ sum_y  \
    //  K = AtA = \               \    inv(K) = (1/det(K)) * \                \     Atb = \        \
    //            \sum_x   sum_xx \                          \ -sum_x     n   \           \ sum_xy \
    //-------------------------------------------------------------------------------------------------
    //  u = inv(AtA)Atb = inv(K)Atb
    //  K = AtA
    //  t -> Transpose
    
    double det, invK_11, invK_12, invK_21, invK_22, n;
    
    n = points.size();
    det = n*sum_xx - sum_x*sum_x;
    
    invK_11 = sum_xx/det;
    invK_12 = -sum_x/det;
    invK_21 = invK_12;
    invK_22 = n/det;
    
    //------------------------------
    
    double m, y_intercept;
    
    y_intercept = invK_11*sum_y + invK_12*sum_xy;
    m = invK_21*sum_y + invK_22*sum_xy;
    
    //------------------------------
    
    lin_reg_slope_yinter return_val = {m, y_intercept};
    return return_val;
}


lin_reg_slope_yinter  linear_regression::weighted(const std::vector<xy_point> &points)
{
    double sum_w, sum_wx, sum_wxx, sum_wy, sum_wxy, x, y, w;
    
    sum_w = 0;
    sum_wx = 0;
    sum_wxx = 0;
    sum_wy = 0;
    sum_wxy = 0;
    
    for(int i=0; i<points.size(); i++)
    {
        x = points[i].x;
        y = points[i].y;
        w = points[i].w;
    
        sum_w += w;
        sum_wx += w*x;
        sum_wxx += w*x*x;
        sum_wy += w*y;
        sum_wxy += w*x*y;
    }
    
    //--------------------------------------------------------------------------------------------------------
    //             \ sum_w    sum_wx \                          \ sum_wxx  -sum_wx \            \ sum_wy  \
    //  K = AtCA = \                 \    inv(K) = (1/det(K)) * \                  \     AtCb = \         \
    //             \sum_wx   sum_wxx \                          \ -sum_wx   sum_w  \            \ sum_wxy \
    //--------------------------------------------------------------------------------------------------------    
    //  u = inv(AtCA)AtCb
    //  K = AtCA
    //  t -> Transpose
    
    double det, invK_11, invK_12, invK_21, invK_22;
    
    det = sum_w*sum_wxx - sum_wx*sum_wx;
    
    invK_11 = sum_wxx/det;
    invK_12 = -sum_wx/det;
    invK_21 = invK_12;
    invK_22 = sum_w/det;
    
    //------------------------------
    
    double m, y_intercept;
    
    y_intercept = invK_11*sum_wy + invK_12*sum_wxy;
    m = invK_21*sum_wy + invK_22*sum_wxy;
    
    //------------------------------
    
    lin_reg_slope_yinter return_val = {m, y_intercept};
    return return_val;
}


//#############################################################################
//                           P_vs_time_profile
//#############################################################################

bool P_vs_time_profile::get_is_charge_not_discharge_profile() {return is_charge_not_discharge_profile;}

double P_vs_time_profile::get_time_step_sec() {return time_step_sec;}

double P_vs_time_profile::get_data_start_offset_sec() {return data_start_offset_sec;}

double P_vs_time_profile::get_data_end_offset_sec() {return data_end_offset_sec;}


const std::vector<double>& P_vs_time_profile::get_soc() {return soc;}

const std::vector<double>& P_vs_time_profile::get_dcP_kW() {return dcP_kW;}

const std::vector<double>& P_vs_time_profile::get_dcP_kW_bat_losses() {return dcP_kW_bat_losses;}

const std::vector<double>& P_vs_time_profile::get_acP_kW() {return acP_kW;}

const std::vector<double>& P_vs_time_profile::get_acQ_kVAR() {return acQ_kVAR;}

const std::vector<double>& P_vs_time_profile::get_unix_times() {return unix_times;}

const std::vector<double>& P_vs_time_profile::get_min_time_to_charge_hrs() {return min_time_to_charge_hrs;}
 
const std::vector<double>& P_vs_time_profile::get_aprox_charge_time_hrs() {return aprox_charge_time_hrs;}


double P_vs_time_profile::get_min_time_to_charge_val_hrs()
{    
    double return_val = 0;
    
    for(int i=0; i<min_time_to_charge_hrs.size(); i++)
    {
        return_val += min_time_to_charge_hrs[i];
                                            
        if(min_time_to_charge_hrs[i] < 0)
        {
            return_val = -1;
            break;
        }
    }
    
    return return_val;        
}


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
void P_vs_time_profile::set_times(int num_timesteps, double time_step_sec_, double unix_ref_time_, double data_start_offset_sec_, double data_end_offset_sec_)
{
    if(!times_have_been_set)
    {
        times_have_been_set = true;
        unix_ref_time = unix_ref_time_;
        time_step_sec = time_step_sec_;
        data_start_offset_sec = data_start_offset_sec_;
        data_end_offset_sec = data_end_offset_sec_;
        
        // --------------------------------
        
        unix_times.resize(num_timesteps);
        
        double tmp_unix_time = unix_ref_time_;
        
        for(int i=0; i<num_timesteps; i++)
        {
            unix_times[i] = tmp_unix_time;
            tmp_unix_time += time_step_sec_;
        }
    }
}

void P_vs_time_profile::set_vectors(bool is_charge_not_discharge_profile_, const std::vector<double> &soc_, const std::vector<double> &dcP_kW_, const std::vector<double> &dcP_kW_bat_losses_,
                                    const std::vector<double> &acP_kW_, const std::vector<double> &acQ_kVAR_, const std::vector<double> &min_time_to_charge_hrs_, 
                                    const std::vector<double> &aprox_charge_time_hrs_)
{
    if(!vectors_have_been_set)
    {
        vectors_have_been_set = true;
        
        is_charge_not_discharge_profile = is_charge_not_discharge_profile_;
        soc = soc_;
        dcP_kW = dcP_kW_;
        dcP_kW_bat_losses = dcP_kW_bat_losses_;
        acP_kW = acP_kW_;
        acQ_kVAR = acQ_kVAR_;
        min_time_to_charge_hrs = min_time_to_charge_hrs_;
        aprox_charge_time_hrs = aprox_charge_time_hrs_;
    }
}


//#############################################################################
//                                 Functions
//#############################################################################

// splits a 'char' delimited string into tokens
std::vector<std::string> split(const std::string& line, char delim)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(line);
   
   while (std::getline(tokenStream, token, delim))
   {
      tokens.push_back(token);
   }
   
   return tokens;
}


double get_real_time()
{
	timeval curr_time;
	gettimeofday(&curr_time, NULL);
	
	return curr_time.tv_sec + curr_time.tv_usec*1e-6;
}

*/


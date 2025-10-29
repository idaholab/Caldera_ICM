#ifndef FACTORY_SOC_VS_P2_H
#define FACTORY_SOC_VS_P2_H

#include <map>
#include <unordered_map>

#include "EV_EVSE_inventory.h"

#include "helper.h"                 // line_segment,SOC_vs_P2 

#include "temperature_aware_profiles.h"

typedef double c_rate;
typedef double SOC;
typedef double power;

#define TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING 0

static std::map< std::string, std::unordered_map< std::pair<EV_type, EVSE_type>, temperature_aware::temperature_aware_profiles_data_store, pair_hash > > TA_DCFC_CURVES_CACHE;




// ******
// *********************
// ******************************************
// ***************************************************************

struct raw_tgrad_model_coeffs_data
{
    std::string tgradmodel_EV_type;
    double ambient_temperature_C_range_min;
    double ambient_temperature_C_range_max;
    double soft_min_battery_temperature_C;
    double soft_max_battery_temperature_C;
    double tgradmodel_c0_intercept;
    double tgradmodel_c1_power_kW;
    double tgradmodel_c2_temperature_C;
    double tgradmodel_c3_time_sec;
    double tgradmodel_c4_soc;
    
    raw_tgrad_model_coeffs_data() : 
        tgradmodel_EV_type(""),
        ambient_temperature_C_range_min(0.0),
        ambient_temperature_C_range_max(0.0),
        soft_min_battery_temperature_C(0.0),
        soft_max_battery_temperature_C(0.0),
        tgradmodel_c0_intercept(0.0),
        tgradmodel_c1_power_kW(0.0),
        tgradmodel_c2_temperature_C(0.0),
        tgradmodel_c3_time_sec(0.0),
        tgradmodel_c4_soc(0.0)
    {}
};

struct each_EV_type_raw_tgrad_model_data
{
    // Data for the battery temperature vs. max power curve.
    std::vector<double> TvsMAXPWR__battery_temperature_C;
    std::vector<double> TvsMAXPWR__max_power_kW;
    
    // Data for the SOC vs. max power curve.
    std::vector<double> SOCvsMAXPWR__soc;
    std::vector<double> SOCvsMAXPWR__max_power_kW;
    
    // The T-grad model coefficients and metadata for each temperature range.
    std::vector< raw_tgrad_model_coeffs_data > tgrad_models_vec;
};

// All the temperature-aware data, mirroring the input folder data.
struct raw_ta_data_store
{
    // Parameters for pre-computing all the curves.
    int n_curve_levels;
    double min_ambient_temperature_C;
    double max_ambient_temperature_C;
    double vary_ambient_temperature_step_C;
    double min_start_battery_temperature_C;
    double max_start_battery_temperature_C;
    double vary_start_battery_temperature_step_C;
    double min_start_SOC;
    double max_start_SOC;
    double vary_start_SOC_step;
    bool holds_data;
    
    // The data for each EV_type
    std::map< std::string, each_EV_type_raw_tgrad_model_data > each_EV_type_ta_data;
    
    raw_ta_data_store() :
        n_curve_levels(0),
        min_ambient_temperature_C(0.0),
        max_ambient_temperature_C(0.0),
        vary_ambient_temperature_step_C(0.0),
        min_start_battery_temperature_C(0.0),
        max_start_battery_temperature_C(0.0),
        vary_start_battery_temperature_step_C(0.0),
        min_start_SOC(0.0),
        max_start_SOC(0.0),
        vary_start_SOC_step(0.0),
        holds_data(false)
    {}
    
    static void load_ta_data( raw_ta_data_store& alltadata,
                              const std::string path_to_ta_directory,
                              const std::vector<std::string>& ev_types_to_load );
};

// ***************************************************************
// ******************************************
// *********************
// ******





enum class point_type
{
    interpolate,
    extend,
    use_directly
};

enum class battery_charge_mode
{
    charging = 0,
    discharging = 1
};

struct bat_objfun_constraints
{
    double a;
    double b;
};


typedef std::map<c_rate, std::map<SOC, std::pair<power, point_type> >, std::greater<c_rate> > curves_grouping;

class create_dcPkW_from_soc
{
private:
    
    const EV_EVSE_inventory& inventory;

    // all the curves are sorted in descending order by c_rate
    const curves_grouping curves;
    const battery_charge_mode mode;

    const double compute_min_non_zero_slope(const std::vector<line_segment>& charge_profile) const;
    const double compute_zero_slope_threshold_P2_vs_soc(const std::vector<line_segment>& charge_profile) const;

    const SOC_vs_P2 get_charging_dcfc_charge_profile( const EV_type& EV, 
                                                      const EVSE_type& EVSE,
                                                      const double c_rate_scale_factor = 1.0 ) const;
    const SOC_vs_P2 get_discharging_dcfc_charge_profile(const EV_type& EV, 
                                                        const EVSE_type& EVSE,
                                                        const double c_rate_scale_factor = 1.0 ) const;

public:
    create_dcPkW_from_soc(const EV_EVSE_inventory& inventory, 
                          const curves_grouping& curves,
                          const battery_charge_mode& mode);

    const SOC_vs_P2 get_dcfc_charge_profile( const battery_charge_mode& mode, 
                                             const EV_type& EV, 
                                             const EVSE_type& EVSE,
                                             const double c_rate_scale_factor = 1.0 ) const;
    const SOC_vs_P2 get_L1_or_L2_charge_profile(const EV_type& EV) const;
};

class factory_SOC_vs_P2
{
private:

    const EV_EVSE_inventory& inventory;

    const create_dcPkW_from_soc LMO_charge;
    const create_dcPkW_from_soc NMC_charge;
    const create_dcPkW_from_soc LTO_charge;

    std::vector<bat_objfun_constraints> constraints;

    const std::unordered_map<EV_type, SOC_vs_P2 > L1_L2_curves;
    const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > DCFC_curves;
    const std::unordered_map< std::pair<EV_type, EVSE_type>, temperature_aware::temperature_aware_profiles_data_store, pair_hash > TA_DCFC_curves;
    const SOC_vs_P2 error_case_curve; // <-- empty data structure the reference to which is returned in error cases.

    const create_dcPkW_from_soc load_LMO_charge();
    const create_dcPkW_from_soc load_NMC_charge();
    const create_dcPkW_from_soc load_LTO_charge();

    const std::unordered_map<EV_type, SOC_vs_P2 > load_L1_L2_curves();
    const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > load_DCFC_curves( const double c_rate_scale_factor = 1.0 );
    
    const std::unordered_map< std::pair<EV_type, EVSE_type>, temperature_aware::temperature_aware_profiles_data_store, pair_hash >& load_temperature_aware_DCFC_curves( 
                                                                                                                            const double max_c_rate_scale_factor,
                                                                                                                            const raw_ta_data_store& ta_raw_data = raw_ta_data_store()
                                                                                                                        );
    
    
public:
    factory_SOC_vs_P2( const EV_EVSE_inventory& inventory
#if TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING
                       , const raw_ta_data_store& ta_data = raw_ta_data_store()
#endif
                       , const double c_rate_scale_factor = 1.0
                    );

    const SOC_vs_P2& get_SOC_vs_P2_curves( const EV_type& EV, 
                                           const EVSE_type& EVSE,
                                           const double charge_start_battery_temperature_C,
                                           const double charge_start_SOC    // <-- In percent a.k.a. 45% SOC is 45.0.
                                      ) const;

    void write_charge_profile(const std::string& output_path) const;
};

#endif  // FACTORY_SOC_VS_P2_H
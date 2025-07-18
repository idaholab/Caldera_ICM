#include "factory_SOC_vs_P2.h"
#include <fstream>
#include <algorithm>
#include <unordered_set>



//##########################################################
//                      create_dcPkW_from_soc
//##########################################################


create_dcPkW_from_soc::create_dcPkW_from_soc(const EV_EVSE_inventory& inventory, 
                                             const curves_grouping& curves,
                                             const battery_charge_mode& mode) 
    : inventory{ inventory }, 
    curves{ curves }, 
    mode{ mode }
{
}

const double create_dcPkW_from_soc::compute_min_non_zero_slope(const std::vector<line_segment>& charge_profile) const
{
    double abs_min_non_zero_slope = 1000000;

    for (const line_segment& ls : charge_profile)
    {
        if ((0 < std::abs(ls.a)) && (std::abs(ls.a) < abs_min_non_zero_slope))
        {
            abs_min_non_zero_slope = std::abs(ls.a);
        }
    }
    return abs_min_non_zero_slope;
}

const double create_dcPkW_from_soc::compute_zero_slope_threshold_P2_vs_soc(const std::vector<line_segment>& charge_profile) const
{
    const double min_zero_slope_threshold_P2_vs_soc = 0.01;   // 1 kW change in P2 over 100 soc
    const double min_non_zero_slope = this->compute_min_non_zero_slope(charge_profile);

    double zero_slope_threshold_P2_vs_soc;
    if (min_non_zero_slope < min_zero_slope_threshold_P2_vs_soc)
    {
        zero_slope_threshold_P2_vs_soc = min_zero_slope_threshold_P2_vs_soc;
    }
    else
    {
        zero_slope_threshold_P2_vs_soc = 0.9 * min_non_zero_slope;
    }
    return zero_slope_threshold_P2_vs_soc;
}

const SOC_vs_P2 create_dcPkW_from_soc::get_L1_or_L2_charge_profile(const EV_type& EV) const
{

    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double AC_charge_rate_kW = this->inventory.get_EV_inventory().at(EV).get_AC_charge_rate_kW();

    const auto curve_1c_ptr = this->curves.find(1.0);
    
    ASSERT(curve_1c_ptr != this->curves.end(), "Error: 1c curve doesnot exist in charging profiles" << std::endl);
    ASSERT(curve_1c_ptr->second.size() >= 2, "Error: 1c curve should have atleast 2 points. currently there are " << curve_1c_ptr->second.size() << " points" << std::endl);

    auto elem_ptr = curve_1c_ptr->second.rbegin();      // iterated from back

    double soc_A = elem_ptr->first;
    double P_A = elem_ptr->second.first * battery_size_kWh;

    elem_ptr++;
    
    double soc_B = elem_ptr->first;
    double P_B = elem_ptr->second.first * battery_size_kWh;

    double m = (P_A - P_B) / (soc_A - soc_B);
    double b = P_A - m * soc_A;

    double soc_intersection = (AC_charge_rate_kW - b) / m;

    //------------------------------------------------

    const std::vector<line_segment> charge_profile = [&]() {
        // start and end of SOC below are supposed to be 0 and 100 but due to the following reason it gets adjusted
        // Don Scoffield's notes
        // P2 vs soc must be defigned a little below 0 soc and a little above 100 soc.

        // When a battery is approaching 0 soc or 100 soc there is a chance that energy continues to go into the battery while
        // the soc is not changing (fixed at 0 or 100 soc)
        //     - This is caused by the fact that when a battery stopps charging/discharging there is a ramp down period.
        //     - This problem (for small time steps) can be mitigated by the following:
        //        - Make sure the P2_vs_soc curve decreases to zero as soc approaches 0 or 100 soc
        //        - Make sure the ramp rate is large when a battery stops charging/discharging
        //      - Make sure the delay is small when battery stops charging/discharging

        std::vector<line_segment> charge_profile;

        if (soc_intersection < 99.5)
        {
            charge_profile.emplace_back( -0.1, soc_intersection, 0, AC_charge_rate_kW);
            charge_profile.emplace_back( soc_intersection, 100.1, m, b );
        }
        else
        {
            charge_profile.emplace_back( -0.1, 100.1, 0, AC_charge_rate_kW );
        }
        return charge_profile;
    }();
    
    //------------------------------------------------

    const double zero_slope_threshold_P2_vs_soc = this->compute_zero_slope_threshold_P2_vs_soc(charge_profile);
    
    return SOC_vs_P2{ charge_profile, zero_slope_threshold_P2_vs_soc };
}


const SOC_vs_P2 create_dcPkW_from_soc::get_dcfc_charge_profile( const battery_charge_mode& mode, 
                                                                const EV_type& EV, 
                                                                const EVSE_type& EVSE,
                                                                const double c_rate_scale_factor ) const
{
    if (mode == battery_charge_mode::charging)
    {
        return this->get_charging_dcfc_charge_profile(EV, EVSE, c_rate_scale_factor);
    }
    else if (mode == battery_charge_mode::discharging)
    {
        return this->get_discharging_dcfc_charge_profile(EV, EVSE, c_rate_scale_factor);
    }
    else
    {
        ASSERT(false, "Error : Unknown battery mode. Its not charging nor discharging");
        return SOC_vs_P2();
    }
}


const SOC_vs_P2 create_dcPkW_from_soc::get_charging_dcfc_charge_profile( const EV_type& EV, 
                                                                         const EVSE_type& EVSE,
                                                                         const double c_rate_scale_factor ) const
{
    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double battery_capacity_Ah_1C = this->inventory.get_EV_inventory().at(EV).get_battery_size_Ah_1C();
    const double EV_crate = c_rate_scale_factor * this->inventory.get_EV_inventory().at(EV).get_max_c_rate();
    const double EV_current_limit = EV_crate * battery_capacity_Ah_1C;

    const double EVSE_current_limit_A = this->inventory.get_EVSE_inventory().at(EVSE).get_current_limit_A();
    const double power_limit_kW = 10000;//this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();
    
    //=====================================================
    //    Apply current limits
    //=====================================================

    const double current_limit_A = std::min(EV_current_limit, EVSE_current_limit_A);

    //=====================================================
    //    Interpolate pu curves using dc_current_limit
    //=====================================================

    std::vector<line_segment> dcPkW_from_soc_input;
    double soc_0, soc_1, soc_tmp, P_0, P_1, w, P_tmp;
    double m, b, m1, b1;

    auto it = this->curves.begin();
    if (current_limit_A >= (it->first * battery_capacity_Ah_1C)) // crate * battery_capacity_Ah_1C
    {   
        const std::map<SOC, std::pair<power, point_type> >& points= it->second;
        
        double cur_SOC, next_SOC;
        double cur_Ppu, next_Ppu;
        double m, b;

        for (auto it_points = points.begin(); it_points != std::prev(points.end()); it_points++)
        {
            cur_SOC = it_points->first;
            cur_Ppu = it_points->second.first;  // second is a pair of power and point_type

            next_SOC = std::next(it_points)->first;
            next_Ppu = std::next(it_points)->second.first;

            m = (cur_Ppu - next_Ppu) / (cur_SOC - next_SOC);
            b = cur_Ppu - m * cur_SOC;

            dcPkW_from_soc_input.emplace_back(cur_SOC, next_SOC, m, b);
        }
    }
    else
    {
        //--------------------------------------    
        //     Select Curves to Interpolate
        //--------------------------------------

        auto upper_curve_ptr = this->curves.begin();    // temporary initialization
        auto lower_curve_ptr = this->curves.begin();
        
        for (it = std::next(this->curves.begin()); it != this->curves.end(); it++)
        {
            if ( (it->first * battery_capacity_Ah_1C) < current_limit_A)
            {
                upper_curve_ptr = std::prev(it);
                lower_curve_ptr = it;
                break;
            }
        }

        //--------------------------------------    
        //       Interpolate Curves
        //--------------------------------------

        // interpolated_val = weight_factor*high_val  +  (1 - weight_factor)*low_val;
        double weight_factor = (current_limit_A - lower_curve_ptr->first * battery_capacity_Ah_1C) / (upper_curve_ptr->first * battery_capacity_Ah_1C - lower_curve_ptr->first * battery_capacity_Ah_1C);

        const std::map < SOC, std::pair<power, point_type> >& upper_points = upper_curve_ptr->second;
        const std::map < SOC, std::pair<power, point_type> >& lower_points = lower_curve_ptr->second;
        
        //----------------- 

        auto upper_ptr = upper_points.begin();
        auto lower_ptr = lower_points.begin();

        double lower_soc{}, upper_soc{};
        double next_lower_soc{}, next_upper_soc{};
        double lower_power{}, upper_power{};
        double next_lower_power{}, next_upper_power{};
        point_type lower_point_type{}, next_lower_point_type{};
        point_type upper_point_type{}, next_upper_point_type{};

        bool extend_segment;

        auto update_lower = [&]() {
            lower_soc = lower_ptr->first;
            lower_power = lower_ptr->second.first;
            lower_point_type = lower_ptr->second.second;

            next_lower_soc = std::next(lower_ptr)->first;            
            next_lower_power = std::next(lower_ptr)->second.first;
            next_lower_point_type = std::next(lower_ptr)->second.second;
        };

        auto update_upper = [&]() {
            upper_soc = upper_ptr->first;
            upper_power = upper_ptr->second.first;
            upper_point_type = upper_ptr->second.second;

            next_upper_soc = std::next(upper_ptr)->first;
            next_upper_power = std::next(upper_ptr)->second.first;
            next_upper_point_type = std::next(upper_ptr)->second.second;
        };

        soc_0 = 0;
        P_0 = weight_factor * upper_ptr->second.first + (1 - weight_factor) * lower_ptr->second.first;

        // upper_points and lower_points are points not segments
        while (true)
        {
            update_lower();
            update_upper();

            extend_segment = false;

            if (std::abs(next_lower_soc - next_upper_soc) < 1)
            {
                if (next_upper_point_type == point_type::extend)
                    extend_segment = true;

                soc_1 = next_upper_soc;
                P_1 = weight_factor * next_upper_power + (1 - weight_factor) * next_lower_power;
                
                lower_ptr++;
                update_lower();
                upper_ptr++;
                update_upper();
            }
            else if (next_upper_soc < next_lower_soc)
            {
                if (next_upper_point_type == point_type::extend)
                    extend_segment = true;

                soc_1 = next_upper_soc;

                w = (soc_1 - lower_soc) / (next_lower_soc - lower_soc);
                P_tmp = w * next_lower_power + (1 - w) * lower_power;

                P_1 = weight_factor * next_upper_power + (1 - weight_factor) * P_tmp;
                upper_ptr++;
                update_upper();
            }
            else if (next_lower_soc < next_upper_soc)
            {
                soc_1 = next_lower_soc;

                w = (soc_1 - upper_soc) / (next_upper_soc - upper_soc);
                P_tmp = w * next_upper_power + (1 - w) * upper_power;

                P_1 = weight_factor * P_tmp + (1 - weight_factor) * next_lower_power;
                lower_ptr++;
                update_lower();
            }
            else
            {
                ASSERT(false, "ERROR: Not expecting this block to be active");
            }

            //-------------------------------

            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m * soc_1;

            if (extend_segment)
            {
                m1 = (next_upper_power - upper_power) / (next_upper_soc - upper_soc);
                b1 = upper_power - m1 * upper_soc;

                soc_1 = (b - b1) / (m1 - m);
                P_1 = m * soc_1 + b;
            }

            dcPkW_from_soc_input.emplace_back(soc_0, soc_1, m, b);

            soc_0 = soc_1;
            P_0 = P_1;

            if (extend_segment)
                break;
        }

        //--------------------------------------    
       //       Use Upper Curve Directly
       //--------------------------------------

        for (upper_ptr = std::next(upper_ptr); upper_ptr != upper_points.end(); upper_ptr++)
        {
            soc_1 = upper_ptr->first;
            P_1 = upper_ptr->second.first;

            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m * soc_1;

            dcPkW_from_soc_input.emplace_back(soc_0, soc_1, m, b);

            soc_0 = soc_1;
            P_0 = P_1;
        }
    }

    //=====================================================
    //            Scale Using Battery Energy
    //=====================================================

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        dcPkW_from_soc_input[i].a = dcPkW_from_soc_input[i].a * battery_size_kWh;
        dcPkW_from_soc_input[i].b = dcPkW_from_soc_input[i].b * battery_size_kWh;
    }

    //=====================================================
    //                Apply Power Limits
    //=====================================================

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        soc_0 = dcPkW_from_soc_input[i].x_LB;
        soc_1 = dcPkW_from_soc_input[i].x_UB;
        m = dcPkW_from_soc_input[i].a;
        b = dcPkW_from_soc_input[i].b;

        P_0 = m * soc_0 + b;
        P_1 = m * soc_1 + b;

        if (power_limit_kW <= P_0 && power_limit_kW <= P_1)
        {
            dcPkW_from_soc_input[i].a = 0;
            dcPkW_from_soc_input[i].b = power_limit_kW;
        }
        else if (P_0 < power_limit_kW && power_limit_kW < P_1)
        {
            soc_tmp = (power_limit_kW - b) / m;
            dcPkW_from_soc_input[i].x_UB = soc_tmp;

            dcPkW_from_soc_input.emplace_back(soc_tmp, soc_1, 0, power_limit_kW);
        }
        else if (power_limit_kW < P_0 && P_1 < power_limit_kW)
        {
            soc_tmp = (power_limit_kW - b) / m;
            dcPkW_from_soc_input[i].x_LB = soc_tmp;

            dcPkW_from_soc_input.emplace_back(soc_0, soc_tmp, 0, power_limit_kW);
        }
    }

    //--------------------------------------

    const double zero_slope_threshold_P2_vs_soc = this->compute_zero_slope_threshold_P2_vs_soc(dcPkW_from_soc_input);

    //=====================================================
    //       Calculate Objective Function Constraints
    //=====================================================

    int boundary_index;

    std::sort(dcPkW_from_soc_input.begin(), dcPkW_from_soc_input.end());

    boundary_index = -1;
    for (int i = (int)dcPkW_from_soc_input.size() - 1; i >= 0; i--)
    {
        if (0 <= dcPkW_from_soc_input[i].a)
        {
            boundary_index = i;
            break;
        }
    }

    //----------------------

    std::vector<xy_point> points_A;
    std::vector<xy_point> points_B;
    double x0, x1, y0, y1;

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        x0 = dcPkW_from_soc_input[i].x_LB;
        y0 = dcPkW_from_soc_input[i].a * x0 + dcPkW_from_soc_input[i].b;
        x1 = dcPkW_from_soc_input[i].x_UB;
        y1 = dcPkW_from_soc_input[i].a * x1 + dcPkW_from_soc_input[i].b;
        w = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

        xy_point point_0 = { x0, y0, w };
        xy_point point_1 = { x1, y1, w };

        if (i <= boundary_index)
        {
            points_A.push_back(point_0);
            points_A.push_back(point_1);
        }
        else
        {
            points_B.push_back(point_0);
            points_B.push_back(point_1);
        }
    }

    lin_reg_slope_yinter  val_A = linear_regression::weighted(points_A);
    lin_reg_slope_yinter  val_B = linear_regression::weighted(points_B);

    //----------------------                                               

    bat_objfun_constraints constraint_A = { val_A.m, val_A.b };
    bat_objfun_constraints constraint_B = { val_B.m, val_B.b };

    std::vector<bat_objfun_constraints> constraints_;
    constraints_.push_back(constraint_A);
    constraints_.push_back(constraint_B);

    //constraints = constraints_;
    
    //----------------------------------
    // adjust start and end soc
    //----------------------------------

    for (int i = 0; i < (int)dcPkW_from_soc_input.size(); i++)
    {
        if (dcPkW_from_soc_input[i].x_LB == 0) dcPkW_from_soc_input[i].x_LB = -0.1;
        if(dcPkW_from_soc_input[i].x_UB == 100) dcPkW_from_soc_input[i].x_UB = 100.1;
    }
    return SOC_vs_P2{ dcPkW_from_soc_input, zero_slope_threshold_P2_vs_soc };
}

const SOC_vs_P2 create_dcPkW_from_soc::get_discharging_dcfc_charge_profile( const EV_type& EV, 
                                                                            const EVSE_type& EVSE,
                                                                            const double c_rate_scale_factor ) const
{
    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double battery_capacity_Ah_1C = this->inventory.get_EV_inventory().at(EV).get_battery_size_Ah_1C();
    const double EV_crate = c_rate_scale_factor * this->inventory.get_EV_inventory().at(EV).get_max_c_rate();
    const double EV_current_limit = EV_crate * battery_capacity_Ah_1C;

    const double EVSE_current_limit_A = this->inventory.get_EVSE_inventory().at(EVSE).get_current_limit_A();
    const double power_limit_kW = 10000;//this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();

    //=====================================================
    //    Apply current limits
    //=====================================================

    const double current_limit_A = std::min(EV_current_limit, EVSE_current_limit_A);

    //=====================================================
    //    Interpolate pu curves using dc_current_limit
    //=====================================================

    std::vector<line_segment> dcPkW_from_soc_input;
    double soc_0, soc_1, soc_tmp, P_0, P_1, w, P_tmp;
    double m, b, m1, b1;

    auto it = this->curves.rbegin();
    if (current_limit_A <= it->first * battery_capacity_Ah_1C) 
    {
        const std::map< SOC, std::pair<power, point_type> >& points = it->second;

        double cur_SOC, next_SOC;
        double cur_Ppu, next_Ppu;

        for (auto it_points = points.begin(); it_points != std::prev(points.end()); it_points++)
        {
            cur_SOC = it_points->first;
            cur_Ppu = it_points->second.first;  // second is a pair of power and point_type

            next_SOC = std::next(it_points)->first;
            next_Ppu = std::next(it_points)->second.first;

            m = (cur_Ppu - next_Ppu) / (cur_SOC - next_SOC);
            b = cur_Ppu - m * cur_SOC;

            dcPkW_from_soc_input.emplace_back(cur_SOC, next_SOC, m, b);
        }
    }
    else
    {
        //--------------------------------------    
        //     Select Curves to Interpolate
        //--------------------------------------

        auto upper_curve_ptr = this->curves.rbegin();   // temporary initialization
        auto lower_curve_ptr = this->curves.rbegin();

        for (it = std::next(this->curves.rbegin()); it != this->curves.rend(); it++)
        {
            if (current_limit_A < it->first * battery_capacity_Ah_1C)
            {
                upper_curve_ptr = std::prev(it);
                lower_curve_ptr = it;
                break;
            }
        }

        //--------------------------------------    
        //       Interpolate Curves
        //--------------------------------------

        // interpolated_val = weight_factor*high_val  +  (1 - weight_factor)*low_val;
        double weight_factor = (current_limit_A - lower_curve_ptr->first * battery_capacity_Ah_1C) / (upper_curve_ptr->first * battery_capacity_Ah_1C - lower_curve_ptr->first * battery_capacity_Ah_1C);

        const std::map < SOC, std::pair<power, point_type> >& upper_points = upper_curve_ptr->second;
        const std::map < SOC, std::pair<power, point_type> >& lower_points = lower_curve_ptr->second;

        //----------------- 

        auto upper_ptr = upper_points.rbegin();
        auto lower_ptr = lower_points.rbegin();

        double lower_soc{}, upper_soc{};
        double next_lower_soc{}, next_upper_soc{};
        double lower_power{}, upper_power{};
        double next_lower_power{}, next_upper_power{};
        point_type lower_point_type{}, next_lower_point_type{};
        point_type upper_point_type{}, next_upper_point_type{};

        bool extend_segment;

        auto update_lower = [&]() {
            lower_soc = lower_ptr->first;
            lower_power = lower_ptr->second.first;
            lower_point_type = lower_ptr->second.second;

            next_lower_soc = std::next(lower_ptr)->first;
            next_lower_power = std::next(lower_ptr)->second.first;
            next_lower_point_type = std::next(lower_ptr)->second.second;
        };

        auto update_upper = [&]() {
            upper_soc = upper_ptr->first;
            upper_power = upper_ptr->second.first;
            upper_point_type = upper_ptr->second.second;

            next_upper_soc = std::next(upper_ptr)->first;
            next_upper_power = std::next(upper_ptr)->second.first;
            next_upper_point_type = std::next(upper_ptr)->second.second;
        };

        soc_0 = 100;
        P_0 = weight_factor * upper_ptr->second.first + (1 - weight_factor) * lower_ptr->second.first;

        // upper_points and lower_points are points not segments
        while (true)
        {
            update_lower();
            update_upper();

            extend_segment = false;

            if (std::abs(next_lower_soc - next_upper_soc) < 1)
            {
                if (next_upper_point_type == point_type::extend)
                    extend_segment = true;

                soc_1 = next_upper_soc;
                P_1 = weight_factor * next_upper_power + (1 - weight_factor) * next_lower_power;
                
                lower_ptr++;
                update_lower();
                upper_ptr++;
                update_upper();
            }
            else if (next_upper_soc > next_lower_soc)
            {
                if (next_upper_point_type == point_type::extend)
                    extend_segment = true;

                soc_1 = next_upper_soc;

                w = (soc_1 - lower_soc) / (next_lower_soc - lower_soc);
                P_tmp = w * next_lower_power + (1 - w) * lower_power;

                P_1 = weight_factor * next_upper_power + (1 - weight_factor) * P_tmp;
                upper_ptr++;
                update_upper();
            }
            else if (next_lower_soc > next_upper_soc)
            {
                soc_1 = next_lower_soc;

                w = (soc_1 - upper_soc) / (next_upper_soc - upper_soc);
                P_tmp = w * next_upper_power + (1 - w) * upper_power;

                P_1 = weight_factor * P_tmp + (1 - weight_factor) * next_lower_power;
                lower_ptr++;
                update_lower();
            }
            else
            {
                ASSERT(false, "ERROR: Not expecting this block to be active");
            }

            //-------------------------------

            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m * soc_1;

            if (extend_segment)
            {
                m1 = (next_upper_power - upper_power) / (next_upper_soc - upper_soc);
                b1 = upper_power - m1 * upper_soc;

                soc_1 = (b - b1) / (m1 - m);
                P_1 = m * soc_1 + b;
            }

            dcPkW_from_soc_input.emplace_back(soc_0, soc_1, m, b);

            soc_0 = soc_1;
            P_0 = P_1;

            if (extend_segment)
                break;
        }

       //--------------------------------------    
       //       Use Upper Curve Directly
       //--------------------------------------
        for (upper_ptr = std::next(upper_ptr); upper_ptr != upper_points.rend(); upper_ptr++)
        {
            soc_1 = upper_ptr->first;
            P_1 = upper_ptr->second.first;

            m = (P_1 - P_0) / (soc_1 - soc_0);
            b = P_1 - m * soc_1;

            dcPkW_from_soc_input.emplace_back(soc_1, soc_0, m, b);

            soc_0 = soc_1;
            P_0 = P_1;
        }
    }

    //=====================================================
    //            Scale Using Battery Energy
    //=====================================================

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        dcPkW_from_soc_input[i].a *= battery_size_kWh;
        dcPkW_from_soc_input[i].b *= battery_size_kWh;
    }

    //=====================================================
    //                Apply Power Limits
    //=====================================================

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        soc_0 = dcPkW_from_soc_input[i].x_LB;
        soc_1 = dcPkW_from_soc_input[i].x_UB;
        m = dcPkW_from_soc_input[i].a;
        b = dcPkW_from_soc_input[i].b;

        P_0 = m * soc_0 + b;
        P_1 = m * soc_1 + b;

        if (P_0 <= power_limit_kW && P_1 <= power_limit_kW)
        {
            dcPkW_from_soc_input[i].a = 0;
            dcPkW_from_soc_input[i].b = power_limit_kW;
        }
        else if (power_limit_kW < P_0 && P_1 < power_limit_kW)
        {
            soc_tmp = (power_limit_kW - b) / m;
            dcPkW_from_soc_input[i].x_UB = soc_tmp;

            dcPkW_from_soc_input.emplace_back(soc_tmp, soc_1, 0, power_limit_kW);
        }
        else if (P_0 < power_limit_kW && power_limit_kW < P_1)
        {
            soc_tmp = (power_limit_kW - b) / m;
            dcPkW_from_soc_input[i].x_LB = soc_tmp;

            dcPkW_from_soc_input.emplace_back(soc_0, soc_tmp, 0, power_limit_kW);
        }
    }

    //--------------------------------------

    const double zero_slope_threshold_P2_vs_soc = this->compute_zero_slope_threshold_P2_vs_soc(dcPkW_from_soc_input);

    //=====================================================
    //       Calculate Objective Function Constraints
    //=====================================================

    int boundary_index;

    std::sort(dcPkW_from_soc_input.begin(), dcPkW_from_soc_input.end());

    boundary_index = -1;
    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        if (0 <= dcPkW_from_soc_input[i].a)
        {
            boundary_index = i;
            break;
        }
    }

    //----------------------

    std::vector<xy_point> points_A;
    std::vector<xy_point> points_B;
    double x0, x1, y0, y1;

    for (int i = 0; i < dcPkW_from_soc_input.size(); i++)
    {
        x0 = dcPkW_from_soc_input[i].x_LB;
        y0 = dcPkW_from_soc_input[i].a * x0 + dcPkW_from_soc_input[i].b;
        x1 = dcPkW_from_soc_input[i].x_UB;
        y1 = dcPkW_from_soc_input[i].a * x1 + dcPkW_from_soc_input[i].b;
        w = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

        xy_point point_0 = { x0, y0, w };
        xy_point point_1 = { x1, y1, w };

        if (i < boundary_index)
        {
            points_A.push_back(point_0);
            points_A.push_back(point_1);
        }
        else
        {
            points_B.push_back(point_0);
            points_B.push_back(point_1);
        }
    }

    lin_reg_slope_yinter  val_A = linear_regression::weighted(points_A);
    lin_reg_slope_yinter  val_B = linear_regression::weighted(points_B);

    //----------------------                                               

    bat_objfun_constraints constraint_A = { val_A.m, val_A.b };
    bat_objfun_constraints constraint_B = { val_B.m, val_B.b };

    std::vector<bat_objfun_constraints> constraints_;
    constraints_.push_back(constraint_A);
    constraints_.push_back(constraint_B);

    //constraints = constraints_;
    return SOC_vs_P2{ dcPkW_from_soc_input, zero_slope_threshold_P2_vs_soc };
}


//##########################################################
//                      factory_SOC_vs_P2
//##########################################################


factory_SOC_vs_P2::factory_SOC_vs_P2( const EV_EVSE_inventory& inventory, const double c_rate_scale_factor ) 
    : inventory{ inventory },
    LMO_charge{ this->load_LMO_charge() },
    NMC_charge{ this->load_NMC_charge() },
    LTO_charge{ this->load_LTO_charge() },
    L1_L2_curves{ this->load_L1_L2_curves() },
    DCFC_curves{ this->load_DCFC_curves( c_rate_scale_factor ) }
#if TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING
    , TA_DCFC_curves{ this->load_temperature_aware_DCFC_curves( c_rate_scale_factor, 20, 10.0 ) }
#endif
#if TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING_V2
    , TA_DCFC_curves_v2{ this->load_temperature_aware_DCFC_curves_v2( c_rate_scale_factor, // const double max_c_rate_scale_factor,
                                                                      20,                  // const int n_curve_levels, 
                                                                      -20.0,               // const double min_start_temperature_C,
                                                                      40.0,                // const double max_start_temperature_C,
                                                                      6.0,                 // const double start_temperature_step,
                                                                      0.0,                 // const double min_start_SOC,
                                                                      90.0,                // const double max_start_SOC,
                                                                      6.0,                 // const double start_SOC_step,
                                                                      ""                   // const std::string cache_file_name
                                                                  ) }
#endif
{
}


const SOC_vs_P2& factory_SOC_vs_P2::get_SOC_vs_P2_curves(const EV_type& EV, 
                                                         const EVSE_type& EVSE) const
{
    const EVSE_level& level = this->inventory.get_EVSE_inventory().at(EVSE).get_level();

    if (level == EVSE_level::L1 || level == EVSE_level::L2)
    {
        if (this->L1_L2_curves.find(EV) != this->L1_L2_curves.end())
        {
            return this->L1_L2_curves.at(EV);
        }
        else
        {
            ASSERT(false, "Error: P2_vs_soc is not defined in the EV_charge_model_factory for EV_type:" << EV << " and SE_type:" << EVSE << std::endl);
            return this->error_case_curve;
        }
    }
#if TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING
    else if (level == EVSE_level::DCFC)
    {
        const std::pair<EV_type, EVSE_type> key = std::make_pair(EV, EVSE);

        if (this->TA_DCFC_curves.find(key) != this->TA_DCFC_curves.end())
        {
            return this->TA_DCFC_curves.at(key);
        }
        else
        {
            ASSERT(false, "Error: [TURN_ON_TEMPERATURE_AWARE_PROFILE_TESTING IS TRUE] P2_vs_soc is not defined in the EV_charge_model_factory for EV_type:" << EV << " and SE_type:" << EVSE << std::endl);
            return this->error_case_curve;
        }
    }
#else
    else if (level == EVSE_level::DCFC)
    {
        const std::pair<EV_type, EVSE_type> key = std::make_pair(EV, EVSE);

        if (this->DCFC_curves.find(key) != this->DCFC_curves.end())
        {
            return this->DCFC_curves.at(key);
        }
        else
        {
            ASSERT(false, "Error: P2_vs_soc is not defined in the EV_charge_model_factory for EV_type:" << EV << " and SE_type:" << EVSE << std::endl);
            return this->error_case_curve;
        }
    }
#endif
    else
    {
        ASSERT(false, "invalid EVSE_level");
        return this->error_case_curve;
    }
}


const create_dcPkW_from_soc factory_SOC_vs_P2::load_LMO_charge()
{
    std::map<SOC, std::pair<power, point_type> > points;
    curves_grouping curves;

    double C_rate;

    //========================
    //    LMO  Charging
    //========================

    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0.898, point_type::interpolate));
    points.emplace( 4.4, std::make_pair(1.056, point_type::interpolate));
    points.emplace( 11.3, std::make_pair(1.154, point_type::interpolate));
    points.emplace( 32.4, std::make_pair(1.215, point_type::interpolate));
    points.emplace( 76.1, std::make_pair(1.274, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.064, point_type::use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.742, point_type::interpolate));
    points.emplace( 4.0, std::make_pair(2.044, point_type::interpolate));
    points.emplace( 12.0, std::make_pair(2.249, point_type::interpolate));
    points.emplace( 55.0, std::make_pair(2.418, point_type::extend));
    points.emplace( 75.0, std::make_pair(1.190, point_type::use_directly));
    points.emplace( 100.0, std::make_pair(0.064, point_type::use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.667, point_type::interpolate));
    points.emplace( 6.0, std::make_pair(3.246, point_type::interpolate));
    points.emplace( 11.9, std::make_pair(3.436, point_type::interpolate));
    points.emplace( 37.6, std::make_pair(3.628, point_type::extend));
    points.emplace( 70.0, std::make_pair(1.440, point_type::use_directly));
    points.emplace( 100.0, std::make_pair(0.064, point_type::use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, point_type::interpolate));
    points.emplace( 100.0, std::make_pair(0, point_type::interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc LMO_charge{ this->inventory, curves, battery_charge_mode::charging };
    return LMO_charge;
}

const create_dcPkW_from_soc factory_SOC_vs_P2::load_NMC_charge()
{
    std::map<SOC, std::pair<power, point_type> > points;
    curves_grouping curves;

    double C_rate;

    bool is_charging_not_discharging = true;

    //========================
    //    NMC  Charging
    //========================

    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0.917, point_type::interpolate));
    points.emplace( 4.0, std::make_pair(1.048, point_type::interpolate));
    points.emplace( 10.0, std::make_pair(1.095, point_type::interpolate));
    points.emplace( 88.0, std::make_pair(1.250, point_type::extend));
    // points.push_back({ 93.0, 0.595, use_directly});  // This point violates one of the key rules.
    points.emplace( 100.0, std::make_pair(0.060, point_type::use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.750, point_type::interpolate));
    points.emplace( 3.0, std::make_pair(2.000, point_type::interpolate));
    points.emplace( 10.0, std::make_pair(2.143, point_type::interpolate));
    points.emplace( 78.5, std::make_pair(2.417, point_type::extend));
    points.emplace( 93.0, std::make_pair(0.595, point_type::use_directly));
    points.emplace( 100.0, std::make_pair(0.060, point_type::use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.798, point_type::interpolate));
    points.emplace( 3.0, std::make_pair(3.167, point_type::interpolate));
    points.emplace( 10.0, std::make_pair(3.393, point_type::interpolate));
    points.emplace( 67.0, std::make_pair(3.750, point_type::extend));
    points.emplace( 93.0, std::make_pair(0.595, point_type::use_directly));
    points.emplace( 100.0, std::make_pair(0.060, point_type::use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, point_type::interpolate));
    points.emplace( 100.0, std::make_pair(0, point_type::interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc NMC_charge{ this->inventory, curves, battery_charge_mode::charging };
    return NMC_charge;
}

const create_dcPkW_from_soc factory_SOC_vs_P2::load_LTO_charge()
{
    std::map<SOC, std::pair<power, point_type> > points;
    curves_grouping curves;

    double C_rate;

    bool is_charging_not_discharging = true;

    //========================
    //    LTO  Charging
    //========================
    curves.clear();

    // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0.798, point_type::interpolate));
    points.emplace( 2.0, std::make_pair(0.882, point_type::interpolate));
    points.emplace( 50.0, std::make_pair(0.966, point_type::interpolate));
    points.emplace( 64.0, std::make_pair(1.008, point_type::interpolate));
    points.emplace( 80.0, std::make_pair(1.040, point_type::interpolate));
    points.emplace( 90.0, std::make_pair(1.071, point_type::interpolate));
    points.emplace( 96.0, std::make_pair(1.134, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.057, point_type::use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.765, point_type::interpolate));
    points.emplace( 2.0, std::make_pair(1.828, point_type::interpolate));
    points.emplace( 50.0, std::make_pair(1.975, point_type::interpolate));
    points.emplace( 60.0, std::make_pair(2.038, point_type::interpolate));
    points.emplace( 80.0, std::make_pair(2.122, point_type::interpolate));
    points.emplace( 91.0, std::make_pair(2.227, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.057, point_type::use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.647, point_type::interpolate));
    points.emplace( 2.0, std::make_pair(2.794, point_type::interpolate));
    points.emplace( 50.0, std::make_pair(2.983, point_type::interpolate));
    points.emplace( 60.0, std::make_pair(3.109, point_type::interpolate));
    points.emplace( 80.0, std::make_pair(3.256, point_type::interpolate));
    points.emplace( 88.0, std::make_pair(3.361, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.085, point_type::use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(3.655, point_type::interpolate));
    points.emplace( 3.0, std::make_pair(3.782, point_type::interpolate));
    points.emplace( 50.0, std::make_pair(4.055, point_type::interpolate));
    points.emplace( 60.0, std::make_pair(4.202, point_type::interpolate));
    points.emplace( 80.0, std::make_pair(4.391, point_type::interpolate));
    points.emplace( 86.0, std::make_pair(4.517, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.113, point_type::use_directly));

    C_rate = 4;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(4.622, point_type::interpolate));
    points.emplace( 4.0, std::make_pair(4.832, point_type::interpolate));
    points.emplace( 50.0, std::make_pair(5.168, point_type::interpolate));
    points.emplace( 60.0, std::make_pair(5.357, point_type::interpolate));
    points.emplace( 84.0, std::make_pair(5.630, point_type::extend));
    points.emplace( 100.0, std::make_pair(0.063, point_type::use_directly));

    C_rate = 5;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, point_type::interpolate));
    points.emplace( 100.0, std::make_pair(0, point_type::interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc LTO_charge{ this->inventory, curves, battery_charge_mode::charging };
    return LTO_charge;
}


const std::unordered_map<EV_type, SOC_vs_P2 > factory_SOC_vs_P2::load_L1_L2_curves()
{
    std::unordered_map<EV_type, SOC_vs_P2 > return_val;

    const EV_inventory& EV_inv = this->inventory.get_EV_inventory();
    
    for (const auto& EVs : EV_inv)
    {
        const EV_type& EV = EVs.first;
        const EV_characteristics& EV_char = EVs.second;

        const battery_chemistry& chemistry = EV_char.get_chemistry();

        if (chemistry == battery_chemistry::LMO)
        {
            return_val.emplace(EV, this->LMO_charge.get_L1_or_L2_charge_profile(EV));
        }
        else if (chemistry == battery_chemistry::NMC)
        {
            return_val.emplace(EV, this->NMC_charge.get_L1_or_L2_charge_profile(EV));
        }
        else if (chemistry == battery_chemistry::LTO)
        {
            return_val.emplace(EV, this->LTO_charge.get_L1_or_L2_charge_profile(EV));
        }
        else
        {
            ASSERT(false, "Error: Invalid battery chemistry for EV_type:" << EV << std::endl);
        }
    }
    return return_val;
}


const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > factory_SOC_vs_P2::load_DCFC_curves( const double c_rate_scale_factor )
{
    std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > return_val;

    const EV_inventory& EV_inv = this->inventory.get_EV_inventory();
    const EVSE_inventory& EVSE_inv = this->inventory.get_EVSE_inventory();

    for (const auto& EVSE_elem : EVSE_inv)
    {
        const EVSE_type& EVSE = EVSE_elem.first;
        const EVSE_characteristics& EVSE_char = EVSE_elem.second;
        const EVSE_level& level = EVSE_char.get_level();

        if (level == EVSE_level::DCFC)
        {
            for (const auto& EV_elem : EV_inv)
            {
                const EV_type& EV = EV_elem.first;
                const EV_characteristics& EV_char = EV_elem.second;
                const bool& DCFC_capable = EV_char.get_DCFC_capable();

                if (DCFC_capable == true)
                {
                    const battery_chemistry& chemistry = EV_char.get_chemistry();
                    // // NOTE: I commentted these out because these are not being used here.
                    //const double& battery_size_kWh = EV_char.get_battery_size_kWh();
                    //const double& bat_size_Ah_1C = EV_char.get_battery_size_Ah_1C();
                    //const double& max_c_rate = EV_char.get_max_c_rate();
                    //const double& power_limit_kW = EVSE_char.get_power_limit_kW();
                    //const double& DC_current_limit = EVSE_char.get_current_limit_A();

                    const battery_charge_mode mode = battery_charge_mode::charging;

                    // charge profile is not a reference because the data is not stored by the object.
                    if (chemistry == battery_chemistry::LMO)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->LMO_charge.get_dcfc_charge_profile(mode, EV, EVSE, c_rate_scale_factor));
                    }
                    else if (chemistry == battery_chemistry::NMC)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->NMC_charge.get_dcfc_charge_profile(mode, EV, EVSE, c_rate_scale_factor));
                    }
                    else if (chemistry == battery_chemistry::LTO)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->LTO_charge.get_dcfc_charge_profile(mode, EV, EVSE, c_rate_scale_factor));
                    }
                    else
                    {
                        ASSERT(false, "Error: Invalid battery chemistry for EV_type:" << EV << std::endl);
                    }
                    //constraints = factory_obj.get_constraints()
                }
                else
                {
                    continue;
                }
            }
        }
        else
        {
            continue;
        }
    }

    return return_val;
}


const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > factory_SOC_vs_P2::load_temperature_aware_DCFC_curves( 
                                                                                                            const double max_c_rate_scale_factor,
                                                                                                            const int n_curve_levels,
                                                                                                            const double starting_battery_temperature_C )
{
    // ---------------------------------------------------------------------------------------------------
    // Step 1: Load 'n_curve_levels' curves at different c-rate levels.
    // ---------------------------------------------------------------------------------------------------
    
    std::vector< std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > > curves_each_level_array;
    std::unordered_set< std::pair<EV_type, EVSE_type>, pair_hash > ev_evse_pairs_set;
    for( int i = 0; i < n_curve_levels; i++ )
    {
        const double adjusted_c_rate_scale_factor = max_c_rate_scale_factor * ((double)(i+1))/((double)n_curve_levels);
        const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash >& curves_for_level_i = factory_SOC_vs_P2::load_DCFC_curves( adjusted_c_rate_scale_factor );
        curves_each_level_array.push_back( curves_for_level_i );
        for( const auto& evevsepair_curve_pair : curves_for_level_i )
        {
            ev_evse_pairs_set.insert( evevsepair_curve_pair.first );
        }
    }
    
    // ---------------------------------------------
    // Step 1.1:  Define things we will need below.
    // ---------------------------------------------
    
    // --------------------------------------------------------
    // Define the 'update_power_level_index_callback' callback.
    // --------------------------------------------------------
    auto update_power_level_index_callback_v4 = [] (
        const int current_power_level_index,
        const int max_power_level_index_at_current_SOC_and_temperature,
        const double current_temperature_C,
        const double current_temperature_grad,
        const double min_temperature_C, // <--- a.k.a. the temperature at which it's okay to heat up again (it's okay for the battery to be colder than this).
        const double max_temperature_C
    ) -> int {
        // Approaching the max temperature threshold:
        const double approaching_max_temp_threshold = min_temperature_C + (9.0/10.0)*(max_temperature_C - min_temperature_C);
        const double approaching_min_temp_threshold = min_temperature_C + (8.0/9.0)*(max_temperature_C - min_temperature_C);
        const double upper_gradient_threshold = (max_temperature_C - current_temperature_C)/60.0;
        const double lower_gradient_threshold = (min_temperature_C - current_temperature_C)/60.0;
        if(
            current_temperature_C >= max_temperature_C
            ||
            ( current_temperature_C > approaching_max_temp_threshold && current_temperature_grad > upper_gradient_threshold )
        )
        {
            // Reduce the power level.
            return std::min( std::max( current_power_level_index-7, 0 ), max_power_level_index_at_current_SOC_and_temperature );
        }
        else if(
            current_temperature_C <= min_temperature_C
            ||
            ( current_temperature_C < approaching_max_temp_threshold && current_temperature_grad < lower_gradient_threshold )
            ||
            current_temperature_C <= approaching_min_temp_threshold
        )
        {
            // Increase the power level.
            return std::min( current_power_level_index+7, max_power_level_index_at_current_SOC_and_temperature );
        }
        else
        {
            return std::min( current_power_level_index, max_power_level_index_at_current_SOC_and_temperature );
        }
    };
    
    
    // ---------------------------------------
    // FOR NOW, just HARD-CODING these values. <--------- TODO: Later, we need a way to input these for each EV_type (and ambient temperature, but that's for later).
    // ---------------------------------------
    // For ngp_hyundai_ioniq_5_longrange_awd:   
    const std::string tgradmodel_EV_type = "ngp_hyundai_ioniq_5_longrange_awd";
    const double tgradmodel_c0_intercept = 0.002023113474550346;
    const double tgradmodel_c1_power_kW = 0.0001736131802966143;
    const double tgradmodel_c2_temperature_C = -0.00043137615959905405;
    const double tgradmodel_c3_time_sec = 2.5671254825204444e-06;
    const double tgradmodel_c4_soc = 3.224134349228123e-06;
    temperature_aware::temperature_gradient_model_v1 temperature_grad_model(
                                        tgradmodel_c0_intercept,
                                        tgradmodel_c1_power_kW,
                                        tgradmodel_c2_temperature_C,
                                        tgradmodel_c3_time_sec,
                                        tgradmodel_c4_soc );
    // For ngp_hyundai_ioniq_5_longrange_awd:   
    std::vector<double> battery_temperature_C = {-1000,0,15,16,20,22,24,25,50,1000};
    std::vector<double> max_charging_power_kW_at_each_T_pts = {20,60,75,107,108,200,200,233,233,300}; 
    std::vector<double> battery_SOC = {-1000,0,15,55,77,82,100,1000};
    std::vector<double> max_charging_power_kW_at_each_SOC_pts = {0,219,224,238,156,131,5,0};
    temperature_aware::max_charging_power_model_v1 max_power_model( battery_temperature_C, max_charging_power_kW_at_each_T_pts, battery_SOC, max_charging_power_kW_at_each_SOC_pts );
    // For ngp_hyundai_ioniq_5_longrange_awd:  
    const double sim_min_battery_temperature_C = 39; // <--- a.k.a. the temperature at which it's okay to heat up again (not actually
                                                     //             the minimum allowed temperature; it's okay for the battery to be colder).
    const double sim_max_battery_temperature_C = 49;
    


    // ---------------------------------------------------------------------------------------------------
    // Step 2: For each (EV_type, EVSE_type) pair, compute the corresponding temperature-aware profile.
    // ---------------------------------------------------------------------------------------------------

    #define TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING 0
    
    const EV_inventory& EV_inv = this->inventory.get_EV_inventory();
    const EVSE_inventory& EVSE_inv = this->inventory.get_EVSE_inventory();

    std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > return_value;

    for( const auto& ev_evse_pair : ev_evse_pairs_set )
    {    
        // Other parameters
        const double time_step_sec = 5;
        const double battery_capacity_kWh = EV_inv.at(ev_evse_pair.first).get_usable_battery_size_kWh();
        const double start_soc = 34;              // <---------------------------------------------------------------- TODO: We technically should be doing this simulation
                                                  //                                                                         for each individual charge event since we won't
                                                  //                                                                         know the start_SOC until the specific charge event and
                                                  //                                                                         that affects the temperature and actual power profile curve!!
        const double end_soc = 96;
        const double start_temperature_C = starting_battery_temperature_C;
        
    
        std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high;
        for( int i = 0; i < n_curve_levels; i++ )
        {
            const SOC_vs_P2& socvsp2_for_level_i = curves_each_level_array.at(i).at( ev_evse_pair );
            power_profiles_sorted_low_to_high.push_back( socvsp2_for_level_i );
        }
        
        const int start_power_level_index = temperature_aware::TemperatureAwareProfiles::get_max_power_level_index_at_current_SOC_and_temperature(
                                                                                    power_profiles_sorted_low_to_high,
                                                                                    max_power_model,
                                                                                    start_temperature_C,
                                                                                    start_soc );
    
        const SOC_vs_P2 socVsP2_temperature_aware = temperature_aware::TemperatureAwareProfiles::generate_temperature_aware_power_profile(
                                                                power_profiles_sorted_low_to_high,     // const std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high,
                                                                temperature_grad_model,                // const temperature_gradient_model& temperature_grad_model,
                                                                max_power_model,                       // const max_charging_power_model& max_power_model,
                                                                time_step_sec,                         // const double time_step_sec,
                                                                battery_capacity_kWh,                  // const double battery_capacity_kWh,
                                                                start_soc,                             // const double start_soc,
                                                                end_soc,                               // const double end_soc,
                                                                start_temperature_C,                   // const double start_temperature_C,
                                                                sim_min_battery_temperature_C,         // const double min_temperature_C,
                                                                sim_max_battery_temperature_C,         // const double max_temperature_C,
                                                                start_power_level_index,               // const int start_power_level_index,
                                                                time_step_sec*3,                       // const double update_power_level_delay_sec,
                                                                #if TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING
                                                                    std::string("TAP_TEMPORARY_output_for_testing.csv"),      // const std::string output_file_name,
                                                                #else
                                                                    std::string(""),      // const std::string output_file_name,
                                                                #endif
                                                                update_power_level_index_callback_v4   // std::function<int(
                                                                                                       //               const int current_power_level_index,
                                                                                                       //               const int max_power_level_index_at_current_temperature,
                                                                                                       //               const double current_temperature_C,
                                                                                                       //               const double current_temperature_grad,
                                                                                                       //               const double min_temperature_C,
                                                                                                       //               const double max_temperature_C
                                                                                                       //           )> update_power_level_index_callback
                                                            );
        
        
        //  ----------------------------------------------------------------------
        //  TEMPORARY FOR TESTING. TEMPORARY FOR TESTING. TEMPORARY FOR TESTING.
        //  ----------------------------------------------------------------------
        #if TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING
        if( ev_evse_pair.first == "bev250_350kW" && ev_evse_pair.second == "xfc_350" )   
        {
            std::cout << "ev_evse_pair: " << ev_evse_pair.first << ",  " << ev_evse_pair.second << std::endl;
            std::cout << "    Result 'socVsP2_temperature_aware': " << socVsP2_temperature_aware << std::endl;
            
            //
            // Plot these results to see if you think it's doing it right!!
            //
            const std::string file_name = "TEMPORARY_output_for_testing.csv";
            std::ofstream opfile;
            opfile.open(file_name);
            opfile << "soc,power_kW" << std::endl;
            const int N = 1000;
            const double h = ( socVsP2_temperature_aware.xmax() - socVsP2_temperature_aware.xmin() ) / N;
            for( int i = 0; i < N; i++ )
            {
                const double x = socVsP2_temperature_aware.xmin() + i*h + 0.5*h;
                const double y = socVsP2_temperature_aware.eval( x );
                opfile << x << "," << y << std::endl;
            }
            // Close the file
            opfile.close();
            __builtin_debugtrap();
        }
        #endif
            
        
        //  --------------------------------------------------------
        //  Saving the result in the 'return_value' data structure.
        //  --------------------------------------------------------
        return_value.emplace( ev_evse_pair, socVsP2_temperature_aware );
    }
    
    // Return the result.
    return return_value;
}





const std::unordered_map< std::pair<EV_type, EVSE_type>, temperature_aware::temperature_aware_profiles_data_store, pair_hash >
factory_SOC_vs_P2::load_temperature_aware_DCFC_curves_v2( 
                                            const double max_c_rate_scale_factor,
                                            const int n_curve_levels,
                                            const double min_start_temperature_C,
                                            const double max_start_temperature_C,
                                            const double start_temperature_step,
                                            const double min_start_SOC,
                                            const double max_start_SOC,
                                            const double start_SOC_step,
                                            const std::string cache_file_name )
{
    std::cout << "Calling 'factory_SOC_vs_P2::load_temperature_aware_DCFC_curves_v2'...." << std::endl;
    
    //
    //
    // If the 'cache_file_name' file exists, then we load that instead.  <------------------  TODO  TODO  TODO  TODO  TODO  TODO
    // TODO.
    //
    //
    
    
    
    
    // For each (EV_type,EVSE_type) pair, we generate a matrix of 'SOC_vs_P2' profiles, one for each (start_temperature_C,start_SOC) pair.
    // We store these in the 'temperature_aware::temperature_aware_profiles_data_store'.
    
    
    
    // ---------------------------------------------------------------------------------------------------
    // Step 1: Load 'n_curve_levels' curves at different c-rate levels.
    // ---------------------------------------------------------------------------------------------------
    
    std::vector< std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > > curves_each_level_array;
    std::unordered_set< std::pair<EV_type, EVSE_type>, pair_hash > ev_evse_pairs_set;
    for( int i = 0; i < n_curve_levels; i++ )
    {
        const double adjusted_c_rate_scale_factor = max_c_rate_scale_factor * ((double)(i+1))/((double)n_curve_levels);
        const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash >& curves_for_level_i = factory_SOC_vs_P2::load_DCFC_curves( adjusted_c_rate_scale_factor );
        curves_each_level_array.push_back( curves_for_level_i );
        for( const auto& evevsepair_curve_pair : curves_for_level_i )
        {
            ev_evse_pairs_set.insert( evevsepair_curve_pair.first );
        }
    }
    
    // ---------------------------------------------
    // Step 1.1:  Define things we will need below.
    // ---------------------------------------------
    
    // --------------------------------------------------------
    // Define the 'update_power_level_index_callback' callback.
    // --------------------------------------------------------
    auto update_power_level_index_callback_v4 = [] (
        const int current_power_level_index,
        const int max_power_level_index_at_current_SOC_and_temperature,
        const double current_temperature_C,
        const double current_temperature_grad,
        const double min_temperature_C, // <--- a.k.a. the temperature at which it's okay to heat up again (it's okay for the battery to be colder than this).
        const double max_temperature_C
    ) -> int {
        // Approaching the max temperature threshold:
        const double approaching_max_temp_threshold = min_temperature_C + (9.0/10.0)*(max_temperature_C - min_temperature_C);
        const double approaching_min_temp_threshold = min_temperature_C + (8.0/9.0)*(max_temperature_C - min_temperature_C);
        const double upper_gradient_threshold = (max_temperature_C - current_temperature_C)/60.0;
        const double lower_gradient_threshold = (min_temperature_C - current_temperature_C)/60.0;
        if(
            current_temperature_C >= max_temperature_C
            ||
            ( current_temperature_C > approaching_max_temp_threshold && current_temperature_grad > upper_gradient_threshold )
        )
        {
            // Reduce the power level.
            return std::min( std::max( current_power_level_index-7, 0 ), max_power_level_index_at_current_SOC_and_temperature );
        }
        else if(
            current_temperature_C <= min_temperature_C
            ||
            ( current_temperature_C < approaching_max_temp_threshold && current_temperature_grad < lower_gradient_threshold )
            ||
            current_temperature_C <= approaching_min_temp_threshold
        )
        {
            // Increase the power level.
            return std::min( current_power_level_index+7, max_power_level_index_at_current_SOC_and_temperature );
        }
        else
        {
            return std::min( current_power_level_index, max_power_level_index_at_current_SOC_and_temperature );
        }
    };
    
    
    
    // ---------------------------------------
    // FOR NOW, just HARD-CODING these values. <--------- TODO: Later, we need a way to input these for each EV_type (and ambient temperature, but that's for later).
    // ---------------------------------------
    // For ngp_hyundai_ioniq_5_longrange_awd:   
    const std::string tgradmodel_EV_type = "ngp_hyundai_ioniq_5_longrange_awd";
    const double tgradmodel_c0_intercept = 0.002023113474550346;
    const double tgradmodel_c1_power_kW = 0.0001736131802966143;
    const double tgradmodel_c2_temperature_C = -0.00043137615959905405;
    const double tgradmodel_c3_time_sec = 2.5671254825204444e-06;
    const double tgradmodel_c4_soc = 3.224134349228123e-06;
    temperature_aware::temperature_gradient_model_v1 temperature_grad_model(
                                        tgradmodel_c0_intercept,
                                        tgradmodel_c1_power_kW,
                                        tgradmodel_c2_temperature_C,
                                        tgradmodel_c3_time_sec,
                                        tgradmodel_c4_soc );
    // For ngp_hyundai_ioniq_5_longrange_awd:   
    std::vector<double> battery_temperature_C = {-1000,0,15,16,20,22,24,25,50,1000};
    std::vector<double> max_charging_power_kW_at_each_T_pts = {20,60,75,107,108,200,200,233,233,300}; 
    std::vector<double> battery_SOC = {-1000,0,15,55,77,82,100,1000};
    std::vector<double> max_charging_power_kW_at_each_SOC_pts = {0,219,224,238,156,131,5,0};
    temperature_aware::max_charging_power_model_v1 max_power_model( battery_temperature_C, max_charging_power_kW_at_each_T_pts, battery_SOC, max_charging_power_kW_at_each_SOC_pts );
    // For ngp_hyundai_ioniq_5_longrange_awd:  
    const double sim_min_battery_temperature_C = 39; // <--- a.k.a. the temperature at which it's okay to heat up again (not actually
                                                     //             the minimum allowed temperature; it's okay for the battery to be colder).
    const double sim_max_battery_temperature_C = 49;
    
    
    
    // -----------------------------------------------------------------------------------------------------------
    // Step 2: For each (EV_type, EVSE_type) pair, compute the corresponding matrix of temperature-aware profiles.
    // -----------------------------------------------------------------------------------------------------------

    #define TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING_V2 0
    
    const EV_inventory& EV_inv = this->inventory.get_EV_inventory();
    const EVSE_inventory& EVSE_inv = this->inventory.get_EVSE_inventory();

    std::unordered_map< std::pair<EV_type, EVSE_type>, temperature_aware::temperature_aware_profiles_data_store, pair_hash > return_value;

    // The timestep used in the temperature-aware-profile algorithm below.
    const double time_step_sec = 5;
    
    // We always end with SOC=100
    const double end_soc = 100;

    for( const auto& ev_evse_pair : ev_evse_pairs_set )
    {
        // Create an instance of 'temperature_aware::temperature_aware_profiles_data_store'
        temperature_aware::temperature_aware_profiles_data_store TAP_data_store;
        
        // Get the clean profile curves at each energy level, sorted low-to-high.
        const std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high = [&] () {
            std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high;
            for( int i = 0; i < n_curve_levels; i++ )
            {
                const SOC_vs_P2& socvsp2_for_level_i = curves_each_level_array.at(i).at( ev_evse_pair );
                power_profiles_sorted_low_to_high.push_back( socvsp2_for_level_i );
            }
            return power_profiles_sorted_low_to_high;
        }();
        
        // Other parameters
        const double battery_capacity_kWh = EV_inv.at(ev_evse_pair.first).get_usable_battery_size_kWh();
        
        // Loop over each pair of values in the matrix, and build the profile for each.
        for( double start_temperature_C = min_start_temperature_C; start_temperature_C <= (max_start_temperature_C + 1e-8); start_temperature_C += start_temperature_step )
        {
            for( double start_soc = min_start_SOC; start_soc <= (max_start_SOC + 1e-8); start_soc += start_SOC_step )
            {
                const int start_power_level_index = temperature_aware::TemperatureAwareProfiles::get_max_power_level_index_at_current_SOC_and_temperature(
                                                                                            power_profiles_sorted_low_to_high,
                                                                                            max_power_model,
                                                                                            start_temperature_C,
                                                                                            start_soc );
                
                const SOC_vs_P2 socVsP2_temperature_aware = temperature_aware::TemperatureAwareProfiles::generate_temperature_aware_power_profile(
                                                                        power_profiles_sorted_low_to_high,     // const std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high,
                                                                        temperature_grad_model,                // const temperature_gradient_model& temperature_grad_model,
                                                                        max_power_model,                       // const max_charging_power_model& max_power_model,
                                                                        time_step_sec,                         // const double time_step_sec,
                                                                        battery_capacity_kWh,                  // const double battery_capacity_kWh,
                                                                        start_soc,                             // const double start_soc,
                                                                        end_soc,                               // const double end_soc,
                                                                        start_temperature_C,                   // const double start_temperature_C,
                                                                        sim_min_battery_temperature_C,         // const double min_temperature_C,
                                                                        sim_max_battery_temperature_C,         // const double max_temperature_C,
                                                                        start_power_level_index,               // const int start_power_level_index,
                                                                        time_step_sec*3,                       // const double update_power_level_delay_sec,
                                                                        #if TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING_V2
                                                                            std::string("TAP_TEMPORARY_output_for_testing.csv"),      // const std::string output_file_name,
                                                                        #else
                                                                            std::string(""),      // const std::string output_file_name,
                                                                        #endif
                                                                        update_power_level_index_callback_v4   // std::function<int(
                                                                                                               //               const int current_power_level_index,
                                                                                                               //               const int max_power_level_index_at_current_temperature,
                                                                                                               //               const double current_temperature_C,
                                                                                                               //               const double current_temperature_grad,
                                                                                                               //               const double min_temperature_C,
                                                                                                               //               const double max_temperature_C
                                                                                                               //           )> update_power_level_index_callback
                                                                    );
                
                //  ----------------------------------------------------------------------
                //  TEMPORARY FOR TESTING. TEMPORARY FOR TESTING. TEMPORARY FOR TESTING.
                //  ----------------------------------------------------------------------
                #if TEMPORARY_TEMPERATURE_AWARE_OUTPUTS_FOR_TESTING_V2
                if( ev_evse_pair.first == "bev250_350kW" && ev_evse_pair.second == "xfc_350" )   
                {
                    std::cout << "ev_evse_pair: " << ev_evse_pair.first << ",  " << ev_evse_pair.second << std::endl;
                    std::cout << "    Result 'socVsP2_temperature_aware': " << socVsP2_temperature_aware << std::endl;
                    
                    //
                    // Plot these results to see if you think it's doing it right!!
                    //
                    const std::string file_name = "TEMPORARY_output_for_testing.csv";
                    std::ofstream opfile;
                    opfile.open(file_name);
                    opfile << "soc,power_kW" << std::endl;
                    const int N = 1000;
                    const double h = ( socVsP2_temperature_aware.xmax() - socVsP2_temperature_aware.xmin() ) / N;
                    for( int i = 0; i < N; i++ )
                    {
                        const double x = socVsP2_temperature_aware.xmin() + i*h + 0.5*h;
                        const double y = socVsP2_temperature_aware.eval( x );
                        opfile << x << "," << y << std::endl;
                    }
                    // Close the file
                    opfile.close();
                    __builtin_debugtrap();
                }
                #endif
                
                //  ----------------------------------------------------------------------------------------------------
                //  Saving the profile in the 'temperature_aware::temperature_aware_profiles_data_store' data structure.
                //  ----------------------------------------------------------------------------------------------------
                TAP_data_store.add( start_temperature_C, start_soc, socVsP2_temperature_aware );
            }
        }
        
        // ----------------------------------------------------------------------------------------------
        // Double-check that the 'temperature_aware::temperature_aware_profiles_data_store' is complete.
        // ----------------------------------------------------------------------------------------------
        if( !TAP_data_store.complete() )
        {
            std::cout << "ERROR: The data store of temperature-aware profiles is not complete even though it should." << std::endl;
            exit(1);
        }
        
        //  --------------------------------------------------------
        //  Saving the result in the 'return_value' data structure.
        //  --------------------------------------------------------
        return_value.emplace( ev_evse_pair, TAP_data_store );
    }
    
    std::cout << "  ... done with 'factory_SOC_vs_P2::load_temperature_aware_DCFC_curves_v2'." << std::endl;
    
    // Return the result.
    return return_value;
}













void factory_SOC_vs_P2::write_charge_profile(const std::string& output_path) const
{

    std::string filename, header, data;
    std::ofstream file_handle;


    filename = output_path + "/dcfc_charge_profile.csv";

    file_handle = std::ofstream(filename);

    header = "soc, ";

    std::vector<double> soc_vals = [&]() {
        std::vector<double> soc_vals;
        for (int i = 0; i <= 100; i++)
        {
            soc_vals.push_back(i);
        }
        return soc_vals;
    }();

    std::vector<std::vector<double> > all_dcfc_profiles = [&]() {
        
        std::vector<std::vector<double> > all_dcfc_profiles;

        for (const auto& curves : this->DCFC_curves)
        {
            const EV_type& EV = curves.first.first;
            const EVSE_type& EVSE = curves.first.second;
            const std::vector<line_segment>& profile = curves.second.curve;

            header += EV + "_" + EVSE + ", ";

            int i = 0;
            double soc = 0.0;
            double a, b;
            int plot_soc_vs_P2_soc_step_size = 1;

            std::vector<double> P2_vals;

            while (true)
            {
                while (true)
                {
                    if (soc <= profile[i].x_UB) break;
                    else i += 1;
                }
                a = profile[i].a;
                b = profile[i].b;
                P2_vals.push_back(a * soc + b);
                soc += plot_soc_vs_P2_soc_step_size;

                if (soc > 100)
                    break;
            }
            all_dcfc_profiles.push_back(P2_vals);
        }
        return all_dcfc_profiles;
    }();

    header += "\n";

    file_handle << header;

    for (int i = 0; i < soc_vals.size(); i++)
    {
        file_handle << std::to_string(soc_vals[i]) + ", ";

        for (int j = 0; j < all_dcfc_profiles.size(); j++)
        {
            file_handle << std::to_string(all_dcfc_profiles[j][i]) + ", ";
        }
        file_handle << "\n ";
    }

    file_handle.close();

    //====================================================

    filename = output_path + "/L1_L2_charge_profile.csv";

    file_handle = std::ofstream(filename);

    header = "soc, ";

    std::vector<std::vector<double> > all_L1_L2_profiles = [&]() {

        std::vector<std::vector<double> > all_L1_L2_profiles;

        for (const auto& curves : this->L1_L2_curves)
        {
            const EV_type& EV = curves.first;
            const std::vector<line_segment>& profile = curves.second.curve;

            header += EV + ", ";

            int i = 0;
            double soc = 0.0;
            double a, b;
            int plot_soc_vs_P2_soc_step_size = 1;

            std::vector<double> P2_vals;

            while (true)
            {
                while (true)
                {
                    if (soc <= profile[i].x_UB) break;
                    else i += 1;
                }
                a = profile[i].a;
                b = profile[i].b;
                P2_vals.push_back(a * soc + b);
                soc += plot_soc_vs_P2_soc_step_size;

                if (soc > 100)
                    break;
            }
            all_L1_L2_profiles.push_back(P2_vals);
        }
        return all_L1_L2_profiles;
    }();

    header += "\n";

    file_handle << header;

    for (int i = 0; i < soc_vals.size(); i++)
    {
        file_handle << std::to_string(soc_vals[i]) + ", ";

        for (int j = 0; j < all_L1_L2_profiles.size(); j++)
        {
            file_handle << std::to_string(all_L1_L2_profiles[j][i]) + ", ";
        }
        file_handle << "\n ";
    }

    file_handle.close();
}
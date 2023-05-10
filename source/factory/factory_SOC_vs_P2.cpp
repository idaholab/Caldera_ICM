#include "factory_SOC_vs_P2.h"
#include <fstream>

//##########################################################
//                      SOC_vs_P2
//##########################################################

SOC_vs_P2::SOC_vs_P2(const std::vector<line_segment>& curve,
                     const double& zero_slope_threashold)
    : curve{ curve }, 
    zero_slope_threashold{ zero_slope_threashold }
{
}


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

const double create_dcPkW_from_soc::compute_zero_slope_threashold_P2_vs_soc(const std::vector<line_segment>& charge_profile) const
{
    const double min_zero_slope_threashold_P2_vs_soc = 0.01;   // 1 kW change in P2 over 100 soc
    const double min_non_zero_slope = this->compute_min_non_zero_slope(charge_profile);

    double zero_slope_threashold_P2_vs_soc;
    if (min_non_zero_slope < min_zero_slope_threashold_P2_vs_soc)
    {
        zero_slope_threashold_P2_vs_soc = min_zero_slope_threashold_P2_vs_soc;
    }
    else
    {
        zero_slope_threashold_P2_vs_soc = 0.9 * min_non_zero_slope;
    }
    return zero_slope_threashold_P2_vs_soc;
}

const SOC_vs_P2 create_dcPkW_from_soc::get_L1_or_L2_charge_profile(const EV_type& EV) const
{

    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double AC_charge_rate_kW = this->inventory.get_EV_inventory().at(EV).get_AC_charge_rate_kW();

    const auto curve_1c_ptr = this->curves.find(1.0);
    
    ASSERT(curve_1c_ptr != this->curves.end(), "Error: 1c curve doesnot exist in charging profiles" << std::endl);
    ASSERT(curve_1c_ptr->second.size() >= 2, "Error: 1c curve should have atleast 2 points. currently there are " << curve_1c_ptr->second.size() << " points" << std::endl);

    auto elem_ptr = curve_1c_ptr->second.begin();

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
        // 	- This is caused by the fact that when a battery stopps charging/discharging there is a ramp down period.
        // 	- This problem (for small time steps) can be mitigated by the following:
        //		- Make sure the P2_vs_soc curve decreases to zero as soc approaches 0 or 100 soc
        //		- Make sure the ramp rate is large when a battery stops charging/discharging
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

    const double zero_slope_threashold_P2_vs_soc = this->compute_zero_slope_threashold_P2_vs_soc(charge_profile);
    
    return SOC_vs_P2{ charge_profile, zero_slope_threashold_P2_vs_soc };
}


const SOC_vs_P2 create_dcPkW_from_soc::get_dcfc_charge_profile(const battery_charge_mode& mode, 
                                                               const EV_type& EV, 
                                                               const EVSE_type& EVSE) const
{
    if (mode == charging)
    {
        return this->get_charging_dcfc_charge_profile(EV, EVSE);
    }
    else if (mode == discharging)
    {
        return this->get_discharging_dcfc_charge_profile(EV, EVSE);
    }
    else
    {
        ASSERT(false, "Error : Unknown battery mode. Its not charging nor discharging");
    }
}


const SOC_vs_P2 create_dcPkW_from_soc::get_charging_dcfc_charge_profile(const EV_type& EV, 
                                                                        const EVSE_type& EVSE) const
{
    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double battery_capacity_Ah_1C = this->inventory.get_EV_inventory().at(EV).get_battery_size_Ah_1C();
    const double current_limit_A = this->inventory.get_EVSE_inventory().at(EVSE).get_current_limit_A();
    const double power_limit_kW = 10000;//this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();

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
                if (next_upper_point_type == extend)
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
                if (next_upper_point_type == extend)
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

        for (upper_ptr = std::next(upper_points.begin()); upper_ptr != upper_points.end(); upper_ptr++)
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

    const double zero_slope_threashold_P2_vs_soc = this->compute_zero_slope_threashold_P2_vs_soc(dcPkW_from_soc_input);

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

    return SOC_vs_P2{ dcPkW_from_soc_input, zero_slope_threashold_P2_vs_soc };
}

const SOC_vs_P2 create_dcPkW_from_soc::get_discharging_dcfc_charge_profile(const EV_type& EV, const EVSE_type& EVSE) const
{
    const double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_kWh();
    const double battery_capacity_Ah_1C = this->inventory.get_EV_inventory().at(EV).get_battery_size_Ah_1C();
    const double current_limit_A = this->inventory.get_EVSE_inventory().at(EVSE).get_current_limit_A();
    const double power_limit_kW = this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();

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
                if (next_upper_point_type == extend)
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
                if (next_upper_point_type == extend)
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
        for (upper_ptr = std::next(upper_points.rbegin()); upper_ptr != upper_points.rend(); upper_ptr++)
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

    const double zero_slope_threashold_P2_vs_soc = this->compute_zero_slope_threashold_P2_vs_soc(dcPkW_from_soc_input);

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
    return SOC_vs_P2{ dcPkW_from_soc_input, zero_slope_threashold_P2_vs_soc };
}


//##########################################################
//                      factory_SOC_vs_P2
//##########################################################


factory_SOC_vs_P2::factory_SOC_vs_P2(const EV_EVSE_inventory& inventory) 
    : inventory{ inventory },
    LMO_charge{ this->load_LMO_charge() },
    NMC_charge{ this->load_NMC_charge() },
    LTO_charge{ this->load_LTO_charge() },
    L1_L2_curves{ this->load_L1_L2_curves() },
    DCFC_curves{ this->load_DCFC_curves() }
{
}


const SOC_vs_P2& factory_SOC_vs_P2::get_SOC_vs_P2_curves(const EV_type& EV, 
                                                         const EVSE_type& EVSE) const
{
    const EVSE_level& level = this->inventory.get_EVSE_inventory().at(EVSE).get_level();

    if (level == L1 || level == L2)
    {
        if (this->L1_L2_curves.find(EV) != this->L1_L2_curves.end())
        {
            return this->L1_L2_curves.at(EV);
        }
        else
        {
            ASSERT(false, "Error: P2_vs_soc is not defined in the EV_charge_model_factory for EV_type:" << EV << " and SE_type:" << EVSE << std::endl);
        }
    }
    else if (level == DCFC)
    {
        const std::pair<EV_type, EVSE_type> key = std::make_pair(EV, EVSE);

        if (this->DCFC_curves.find(key) != this->DCFC_curves.end())
        {
            return this->DCFC_curves.at(key);
        }
        else
        {
            ASSERT(false, "Error: P2_vs_soc is not defined in the EV_charge_model_factory for EV_type:" << EV << " and SE_type:" << EVSE << std::endl);
        }
    }
    else
    {
        ASSERT(false, "invalid EVSE_level");
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
    points.emplace( 0.0, std::make_pair(0.898, interpolate));
    points.emplace( 4.4, std::make_pair(1.056, interpolate));
    points.emplace( 11.3, std::make_pair(1.154, interpolate));
    points.emplace( 32.4, std::make_pair(1.215, interpolate));
    points.emplace( 76.1, std::make_pair(1.274, extend));
    points.emplace( 100.0, std::make_pair(0.064, use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.742, interpolate));
    points.emplace( 4.0, std::make_pair(2.044, interpolate));
    points.emplace( 12.0, std::make_pair(2.249, interpolate));
    points.emplace( 55.0, std::make_pair(2.418, extend));
    points.emplace( 75.0, std::make_pair(1.190, use_directly));
    points.emplace( 100.0, std::make_pair(0.064, use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.667, interpolate));
    points.emplace( 6.0, std::make_pair(3.246, interpolate));
    points.emplace( 11.9, std::make_pair(3.436, interpolate));
    points.emplace( 37.6, std::make_pair(3.628, extend));
    points.emplace( 70.0, std::make_pair(1.440, use_directly));
    points.emplace( 100.0, std::make_pair(0.064, use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, interpolate));
    points.emplace( 100.0, std::make_pair(0, interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc LMO_charge{ this->inventory, curves, charging };
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
    points.emplace( 0.0, std::make_pair(0.917, interpolate));
    points.emplace( 4.0, std::make_pair(1.048, interpolate));
    points.emplace( 10.0, std::make_pair(1.095, interpolate));
    points.emplace( 88.0, std::make_pair(1.250, extend));
    // points.push_back({ 93.0, 0.595, use_directly});  // This point violates one of the key rules.
    points.emplace( 100.0, std::make_pair(0.060, use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.750, interpolate));
    points.emplace( 3.0, std::make_pair(2.000, interpolate));
    points.emplace( 10.0, std::make_pair(2.143, interpolate));
    points.emplace( 78.5, std::make_pair(2.417, extend));
    points.emplace( 93.0, std::make_pair(0.595, use_directly));
    points.emplace( 100.0, std::make_pair(0.060, use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.798, interpolate));
    points.emplace( 3.0, std::make_pair(3.167, interpolate));
    points.emplace( 10.0, std::make_pair(3.393, interpolate));
    points.emplace( 67.0, std::make_pair(3.750, extend));
    points.emplace( 93.0, std::make_pair(0.595, use_directly));
    points.emplace( 100.0, std::make_pair(0.060, use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, interpolate));
    points.emplace( 100.0, std::make_pair(0, interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc NMC_charge{ this->inventory, curves, charging };
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
    points.emplace( 0.0, std::make_pair(0.798, interpolate));
    points.emplace( 2.0, std::make_pair(0.882, interpolate));
    points.emplace( 50.0, std::make_pair(0.966, interpolate));
    points.emplace( 64.0, std::make_pair(1.008, interpolate));
    points.emplace( 80.0, std::make_pair(1.040, interpolate));
    points.emplace( 90.0, std::make_pair(1.071, interpolate));
    points.emplace( 96.0, std::make_pair(1.134, extend));
    points.emplace( 100.0, std::make_pair(0.057, use_directly));

    C_rate = 1;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(1.765, interpolate));
    points.emplace( 2.0, std::make_pair(1.828, interpolate));
    points.emplace( 50.0, std::make_pair(1.975, interpolate));
    points.emplace( 60.0, std::make_pair(2.038, interpolate));
    points.emplace( 80.0, std::make_pair(2.122, interpolate));
    points.emplace( 91.0, std::make_pair(2.227, extend));
    points.emplace( 100.0, std::make_pair(0.057, use_directly));

    C_rate = 2;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(2.647, interpolate));
    points.emplace( 2.0, std::make_pair(2.794, interpolate));
    points.emplace( 50.0, std::make_pair(2.983, interpolate));
    points.emplace( 60.0, std::make_pair(3.109, interpolate));
    points.emplace( 80.0, std::make_pair(3.256, interpolate));
    points.emplace( 88.0, std::make_pair(3.361, extend));
    points.emplace( 100.0, std::make_pair(0.085, use_directly));

    C_rate = 3;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(3.655, interpolate));
    points.emplace( 3.0, std::make_pair(3.782, interpolate));
    points.emplace( 50.0, std::make_pair(4.055, interpolate));
    points.emplace( 60.0, std::make_pair(4.202, interpolate));
    points.emplace( 80.0, std::make_pair(4.391, interpolate));
    points.emplace( 86.0, std::make_pair(4.517, extend));
    points.emplace( 100.0, std::make_pair(0.113, use_directly));

    C_rate = 4;
    curves.emplace(C_rate, points);

    //------------------------------------

                   // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(4.622, interpolate));
    points.emplace( 4.0, std::make_pair(4.832, interpolate));
    points.emplace( 50.0, std::make_pair(5.168, interpolate));
    points.emplace( 60.0, std::make_pair(5.357, interpolate));
    points.emplace( 84.0, std::make_pair(5.630, extend));
    points.emplace( 100.0, std::make_pair(0.063, use_directly));

    C_rate = 5;
    curves.emplace(C_rate, points);

    //------------------------------------

                  // {soc, P, (interpolate, extend, use_directly)}
    points.clear();
    points.emplace( 0.0, std::make_pair(0, interpolate));
    points.emplace( 100.0, std::make_pair(0, interpolate));

    C_rate = 0;
    curves.emplace(C_rate, points);

    //------------------------------------

    const create_dcPkW_from_soc LTO_charge{ this->inventory, curves, charging };
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

        if (chemistry == LMO)
        {
            return_val.emplace(EV, this->LMO_charge.get_L1_or_L2_charge_profile(EV));
        }
        else if (chemistry == NMC)
        {
            return_val.emplace(EV, this->NMC_charge.get_L1_or_L2_charge_profile(EV));
        }
        else if (chemistry == LTO)
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


const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > factory_SOC_vs_P2::load_DCFC_curves()
{
    std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > return_val;

    const EV_inventory& EV_inv = this->inventory.get_EV_inventory();
    const EVSE_inventory& EVSE_inv = this->inventory.get_EVSE_inventory();

    for (const auto& EVSE_elem : EVSE_inv)
    {
        const EVSE_type& EVSE = EVSE_elem.first;
        const EVSE_characteristics& EVSE_char = EVSE_elem.second;
        const EVSE_level& level = EVSE_char.get_level();

        if (level == DCFC)
        {
            for (const auto& EV_elem : EV_inv)
            {
                const EV_type& EV = EV_elem.first;
                const EV_characteristics& EV_char = EV_elem.second;
                const bool& DCFC_capable = EV_char.get_DCFC_capable();

                if (DCFC_capable == true)
                {
                    const battery_chemistry& chemistry = EV_char.get_chemistry();
                    const double& battery_size_kWh = EV_char.get_battery_size_kWh();
                    const double& bat_size_Ah_1C = EV_char.get_battery_size_Ah_1C();
                    const double& max_c_rate = EV_char.get_max_c_rate();
                    const double& power_limit_kW = EVSE_char.get_power_limit_kW();
                    const double& DC_current_limit = EVSE_char.get_current_limit_A();

                    const battery_charge_mode mode = charging;

                    // charge profile is not a reference because the data is not stored by the object.
                    if (chemistry == LMO)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->LMO_charge.get_dcfc_charge_profile(mode, EV, EVSE));
                    }
                    else if (chemistry == NMC)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->NMC_charge.get_dcfc_charge_profile(mode, EV, EVSE));
                    }
                    else if (chemistry == LTO)
                    {
                        return_val.emplace(std::make_pair(EV, EVSE), this->LTO_charge.get_dcfc_charge_profile(mode, EV, EVSE));
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
}
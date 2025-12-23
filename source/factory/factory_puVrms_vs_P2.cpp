#include "factory_puVrms_vs_P2.h"

//###########################################
//              factory_puVrms_vs_P2
//###########################################

factory_puVrms_vs_P2::factory_puVrms_vs_P2(const EV_EVSE_inventory& inventory) 
    : inventory{ inventory }, 
    puVrms_vs_P2_curves{ this->load_puVrms_vs_P2_curves() }
{
}


const std::unordered_map<EVSE_level, std::map<puVrms, P2> > factory_puVrms_vs_P2::load_puVrms_vs_P2_curves()
{
    // The points on the P2_vs_pu_Vrms plot are all multiplied by SE_P2_limit_atNominalV_kW
    // The P2_vs_pu_Vrms plot must pass through the point (1, 1)
    //        At nominal voltage Vrms = 1 the final curve is 1*SE_P2_limit_atNominalV_kW or the limit at nominal voltage

    std::unordered_map<EVSE_level, std::map<puVrms, P2> > data;

    data.emplace(EVSE_level::L1, []() {
        std::vector<puVrms> puVrms_vec = { 0.0, 0.69, 0.7, 2.0 };
        std::vector<P2> P2_vec = { 0.0, 0.00, 0.7, 2.0 };

        std::map<puVrms, P2> curve;

        for (int i = 0; i < (int)puVrms_vec.size(); i++)
        {
            curve.emplace(puVrms_vec[i], P2_vec[i]);
        }
        return curve;
    }());

    data.emplace(EVSE_level::L2, []() {
        std::vector<puVrms> puVrms_vec = { 0.0, 0.34, 0.35, 0.94, 2.0 };
        std::vector<P2> P2_vec = { 0.0, 0.0, 0.373, 1.0, 1.0 };

        std::map<puVrms, P2> curve;

        for (int i = 0; i < (int)puVrms_vec.size(); i++)
        {
            curve.emplace(puVrms_vec[i], P2_vec[i]);
        }
        return curve;
    }());

    data.emplace(EVSE_level::DCFC, []() {
        std::vector<puVrms> puVrms_vec = { 0.0, 0.79, 0.80, 1.20, 1.21, 2.0 };
        std::vector<P2> P2_vec = { 0.0, 0.0, 1.0, 1.0, 0.0, 0.0 };

        std::map<puVrms, P2> curve;

        for (int i = 0; i < (int)puVrms_vec.size(); i++)
        {
            curve.emplace(puVrms_vec[i], P2_vec[i]);
        }
        return curve;
    }());
    return data;
}

const poly_function_of_x factory_puVrms_vs_P2::get_puVrms_vs_P2(const EVSE_type& EVSE, 
                                                                const double& SE_P2_limit_atNominalV_kW) const
{
    const EVSE_level& level = this->inventory.get_EVSE_inventory().at(EVSE).get_level();

    const std::map<puVrms, P2>& curve = this->puVrms_vs_P2_curves.at(level);

    const std::map<puVrms, P2> curve_scaled = [&]() {

        std::map<puVrms, P2> curve_scaled;
        
        for (const auto& elem : curve)
        {
            curve_scaled.emplace(elem.first, elem.second * SE_P2_limit_atNominalV_kW);
        }
        return curve_scaled;
    }();

    
    const poly_function_of_x puVrms_vs_P2 = [&]() {

        std::vector<poly_segment> segments;
        double a, b;

        double prev_puVrms, cur_puVrms;
        double prev_P2, cur_P2;

        auto it = curve_scaled.begin();
        prev_puVrms = it->first;
        prev_P2 = it->second;

        for (it = std::next(it); it != curve_scaled.end(); it++)
        {
            cur_puVrms = it->first;
            cur_P2 = it->second;

            a = (prev_P2 - cur_P2) / (prev_puVrms - cur_puVrms);
            b = cur_P2 - a * cur_puVrms;
            segments.emplace_back(prev_puVrms, cur_puVrms, poly_degree::first, a, b, 0, 0, 0 );      // x_LB, x_UB, degree, a, b, c, d, e

            prev_puVrms = cur_puVrms;
            prev_P2 = cur_P2;
        }
        //--------

        double x_tolerance = 0.0001;
        bool take_abs_of_x = false;
        bool if_x_is_out_of_bounds_print_warning_message = true;
        return poly_function_of_x{ x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, segments, "P2_vs_puVrms" };
    }();

    return puVrms_vs_P2;
}
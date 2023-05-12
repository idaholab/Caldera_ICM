#include "factory_ac_to_dc_converter.h"

//#############################################################################
//                      AC to DC Converter Factory
//#############################################################################

ac_to_dc_converter* factory_ac_to_dc_converter::alloc_get_ac_to_dc_converter(ac_to_dc_converter_enum converter_type, 
																			 EVSE_type EVSE, 
																			 EV_type EV, 
																			 charge_event_P3kW_limits& P3kW_limits)
{
	std::vector<poly_segment> inv_eff_from_P2_vec;
	std::vector<poly_segment> inv_pf_from_P3_vec;

	//=============================================================
	//=============================================================

	if (this->inventory.get_EVSE_inventory().at(EVSE).get_level() == L1)
	{
		// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({ -20, 0, first, (1.1 - 1) / (-20 - 0), 1, 0, 0, 0 });
		inv_eff_from_P2_vec.push_back({ 0, 1.4, second, -0.09399, 0.26151, 0.71348, 0, 0 });
		inv_eff_from_P2_vec.push_back({ 1.4, 20, first, 0, 0.895374, 0, 0, 0 });

		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({ -20, 0, first, (-1 + 0.9) / (-20 - 0), -0.90, 0, 0, 0 });
		inv_pf_from_P3_vec.push_back({ 0, 1.3, first, -0.0138, -0.9793, 0, 0, 0 });
		inv_pf_from_P3_vec.push_back({ 1.3, 20, first, 0, -0.99724, 0, 0, 0 });
	}
	else if (this->inventory.get_EVSE_inventory().at(EVSE).get_level() == L2)
	{
		// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({ -20, 0, first, (1.1 - 1) / (-20 - 0), 1, 0, 0, 0 });
		inv_eff_from_P2_vec.push_back({ 0, 4, second, -0.005, 0.045, 0.82, 0, 0 });
		inv_eff_from_P2_vec.push_back({ 4, 20, first, 0, 0.92, 0, 0, 0 });

		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({ -20, 0, first, (-1 + 0.9) / (-20 - 0), -0.90, 0, 0, 0 });
		inv_pf_from_P3_vec.push_back({ 0, 6, third, -0.00038737, 0.00591216, -0.03029164, -0.9462841, 0 });
		inv_pf_from_P3_vec.push_back({ 6, 20, first, 0, -0.9988681, 0, 0, 0 });
	}
	else if (this->inventory.get_EVSE_inventory().at(EVSE).get_level() == DCFC)  // Copied data from L2 Charger
	{
		if(this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW() <= 20)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -30, 0, first, (1.1 - 1) / (-30 - 0), 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 4, second, -0.005, 0.045, 0.82, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 4, 30, first, 0, 0.92, 0, 0, 0 });

			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -30, 0, first, (-1 + 0.9) / (-30 - 0), -0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 6, third, -0.00038737, 0.00591216, -0.03029164, -0.9462841, 0 });
			inv_pf_from_P3_vec.push_back({ 6, 30, first, 0, -0.9988681, 0, 0, 0 });
		}
		else if (this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW() > 20 && this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW() <= 100)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 10, second, -0.0023331, 0.04205, 0.7284, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 10, 20, second, -0.00035233, 0.01454, 0.7755, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 20, 30, second, -0.00015968, 0.01006, 0.7698, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 30, 40, second, -0.000083167, 0.007314, 0.7697, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 40, 1000, first, 0, 0.9292, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 10, second, 0.0037161, -0.1109, -0.06708, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 10, 18.6, first, -0.01474, -0.6568, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 18.6, 28, first, -0.003804, -0.8601, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 28, 42, first, -0.001603, -0.9218, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 42, 1000, first, 0, -0.99, 0, 0, 0 });
		}
		else if (this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW() > 100)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 25, second, -0.0007134, 0.03554, 0.4724, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 25, 130, first, 0.0003331, 0.9067, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 130, 1000, first, 0, 0.95, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 60, third, -0.0000053284, 0.00071628, -0.03361, -0.3506, 0 });
			inv_pf_from_P3_vec.push_back({ 60, 80, first, -0.001034, -0.8775, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 80, 133, second, 0.0000027985, -0.0008177, -0.9127, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 133, 1000, first, 0, -0.972, 0, 0, 0 });
		}
		else
		{
			ASSERT(false, "error");
		}
	}
	else
	{
		ASSERT(false, "error");
	}

	//------------------------------------

	double S3kVA_from_max_nominal_P3kW_multiplier = 1;

	double x_tolerance = 0.0001;
	bool take_abs_of_x = false;
	bool if_x_is_out_of_bounds_print_warning_message = false;

	poly_function_of_x  inv_eff_from_P2(x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, inv_eff_from_P2_vec, "inv_eff_from_P2");
	poly_function_of_x  inv_pf_from_P3(x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, inv_pf_from_P3_vec, "inv_pf_from_P3");

	ac_to_dc_converter* return_val = NULL;

	if (converter_type == pf)
		return_val = new ac_to_dc_converter_pf(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2, inv_pf_from_P3);

	else if (converter_type == Q_setpoint)
		return_val = new ac_to_dc_converter_Q_setpoint(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2);

	else
	{
		std::cout << "ERROR:  In factory_ac_to_dc_converter undefigned converter_type." << std::endl;
		return_val = new ac_to_dc_converter_pf(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2, inv_pf_from_P3);
	}

	return return_val;
}

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

	const EVSE_level level = this->inventory.get_EVSE_inventory().at(EVSE).get_level();
	const double power_limit = this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();

	if (level == L1)
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
	else if (level == L2)
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
	else if (level == DCFC)  // Copied data from L2 Charger
	{
		if(power_limit <= 20)
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
		else if (power_limit > 20 && power_limit <= 100)
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
		else if (power_limit > 100 && power_limit < 500)
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
		else if (power_limit >= 500 && power_limit < 1000)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 9.9509951, fourth, -5.32294E-05, 0.00161341, -0.020375254, 0.153430262, 0.1 });
			inv_eff_from_P2_vec.push_back({ 9.9509951, 49.9549955, fourth, -1.41183E-07, 2.19219E-05, -0.001317196, 0.038812335, 0.40120321 });
			inv_eff_from_P2_vec.push_back({ 49.9549955, 124.9624962, fourth, -8.95362E-10, 3.96468E-07, -6.90956E-05, 0.005879087, 0.741555738 });
			inv_eff_from_P2_vec.push_back({ 124.9624962,	500, fourth, -2.58346E-12, 4.05019E-09, -2.42506E-06, 0.000615186, 0.906293666 });
			inv_eff_from_P2_vec.push_back({ 500, 1000, first, 0, 0.952430816, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 9.9509951, fourth, 3.30449E-08, -1.64406E-06, 4.81836E-05, -0.001229586, -0.96 });
			inv_pf_from_P3_vec.push_back({ 9.9509951, 49.9549955, fourth, 1.83122E-09, -3.19941E-07, 2.3503E-05, -0.000997071, -0.960868555 });
			inv_pf_from_P3_vec.push_back({ 49.9549955, 124.9624962, fourth, 4.9509E-11, -2.28852E-08, 4.27128E-06, -0.000415058, -0.967886798 });
			inv_pf_from_P3_vec.push_back({ 124.9624962, 500, fourth, 2.75481E-13, -4.37287E-10, 2.67801E-07, -7.93935E-05, -0.979114283 });
			inv_pf_from_P3_vec.push_back({ 500, 1000, first, 0, -0.989303981, 0, 0, 0 });
		}
		else if (power_limit >= 1000 && power_limit < 2000)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 3.300330033, fourth, -3.07232E-05, 0.000587331, -0.007217139, 0.080972819, 0.1 });
			inv_eff_from_P2_vec.push_back({ 3.300330033, 19.9019902, fourth, -1.99155E-06, 0.000137261, -0.004017833, 0.069501611, 0.115784348 });
			inv_eff_from_P2_vec.push_back({ 19.9019902, 99.909991, fourth, -8.82395E-09, 2.74024E-06, -0.000329299, 0.019406168, 0.40120321 });
			inv_eff_from_P2_vec.push_back({ 99.909991, 1000, fourth, -6.01897E-13, 1.66165E-09, -1.68483E-06, 0.000728381, 0.84911447 });
			inv_eff_from_P2_vec.push_back({ 1000, 10000, first, 0, 0.952427626, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 19.9019902, fourth, 2.0653E-09, -2.05508E-07, 1.20459E-05, -0.000614793, -0.96 });
			inv_pf_from_P3_vec.push_back({ 19.9019902, 99.909991, fourth, 1.14451E-10, -3.99927E-08, 5.87574E-06, -0.000498536, -0.960868555 });
			inv_pf_from_P3_vec.push_back({ 99.909991, 249.9249925, fourth, 3.09432E-12, -2.86066E-09, 1.06782E-06, -0.000207529, -0.967886798 });
			inv_pf_from_P3_vec.push_back({ 249.9249925, 1000, fourth, 1.72176E-14, -5.46609E-11, 6.69504E-08, -3.96968E-05, -0.979114283 });
			inv_pf_from_P3_vec.push_back({ 1000, 10000, first, 0, -0.989303981, 0, 0, 0 });
		}
		else if (power_limit >= 2000 && power_limit < 3000)
		{
			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 6.600660066, fourth, -1.9202E-06, 7.34164E-05, -0.001804285, 0.04048641, 0.1 });
			inv_eff_from_P2_vec.push_back({ 6.600660066, 39.8039804, fourth, -1.24472E-07, 1.71577E-05, -0.001004458, 0.034750806, 0.115784348 });
			inv_eff_from_P2_vec.push_back({ 39.8039804, 199.819982, fourth, -5.51497E-10, 3.4253E-07, -8.23247E-05, 0.009703084, 0.40120321 });
			inv_eff_from_P2_vec.push_back({ 199.819982, 2000, fourth, -3.76186E-14, 2.07707E-10, -4.21206E-07, 0.00036419, 0.84911447 });
			inv_eff_from_P2_vec.push_back({ 2000, 10000, first, 0, 0.952427626, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 39.8039804, fourth, 1.29082E-10, -2.56885E-08, 3.01148E-06, -0.000307397, -0.96 });
			inv_pf_from_P3_vec.push_back({ 39.8039804, 199.819982, fourth, 7.1532E-12, -4.99908E-09, 1.46893E-06, -0.000249268, -0.960868555 });
			inv_pf_from_P3_vec.push_back({ 199.819982, 499.849985, fourth, 1.93395E-13, -3.57582E-10, 2.66955E-07, -0.000103765, -0.967886798 });
			inv_pf_from_P3_vec.push_back({ 499.849985, 2000, fourth, 1.0761E-15, -6.83261E-12, 1.67376E-08, -1.98484E-05, -0.979114283 });
			inv_pf_from_P3_vec.push_back({ 2000, 10000, first, 0, -0.989303981, 0, 0, 0 });
		}
		else if (power_limit >= 3000)
		{
			inv_eff_from_P2_vec.clear();
			inv_eff_from_P2_vec.push_back({ -1000, 0, first, (1.1 - 1) / 1000, 1, 0, 0, 0 });
			inv_eff_from_P2_vec.push_back({ 0, 9.900990099, fourth, -3.79299E-07, 2.1753E-05, -0.000801904, 0.02699094, 0.1 });
			inv_eff_from_P2_vec.push_back({ 9.900990099, 59.7059706, fourth, -2.45871E-08, 5.08376E-06, -0.000446426, 0.023167204, 0.115784348 });
			inv_eff_from_P2_vec.push_back({ 59.7059706, 299.729973, fourth, -1.08938E-10, 1.0149E-07, -3.65888E-05, 0.006468723, 0.40120321 });
			inv_eff_from_P2_vec.push_back({ 299.729973, 3000, fourth, -7.43083E-15, 6.15428E-11, -1.87203E-07, 0.000242794, 0.84911447 });
			inv_eff_from_P2_vec.push_back({ 3000, 10000, first, 0, 0.952427626, 0, 0, 0 });

			// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
			inv_pf_from_P3_vec.clear();
			inv_pf_from_P3_vec.push_back({ -1000, 0, first, (0.9 - 1) / 1000, 0.90, 0, 0, 0 });
			inv_pf_from_P3_vec.push_back({ 0, 59.7059706, fourth, 2.54976E-11, -7.61141E-09, 1.33843E-06, -0.000204931, -0.96 });
			inv_pf_from_P3_vec.push_back({ 59.7059706, 299.729973, fourth, 1.41298E-12, -1.48121E-09, 6.5286E-07, -0.000166179, -0.960868555 });
			inv_pf_from_P3_vec.push_back({ 299.729973, 749.7749775, fourth, 3.82014E-14, -1.0595E-10, 1.18647E-07, -6.91763E-05, -0.967886798 });
			inv_pf_from_P3_vec.push_back({ 749.7749775, 3000, fourth, 2.12563E-16, -2.02448E-12, 7.43893E-09, -1.32323E-05, -0.979114283 });
			inv_pf_from_P3_vec.push_back({ 3000, 10000, first, 0, -0.989303981, 0, 0, 0 });
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

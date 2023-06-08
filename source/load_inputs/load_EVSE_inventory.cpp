#include "load_EVSE_inventory.h"
#include "helper.h"

#include <fstream>
#include <filesystem>
#include <algorithm>

load_EVSE_inventory::load_EVSE_inventory(const std::string& inputs_dir) :
	EVSE_inv{ this->load(inputs_dir) }
{
}

const EVSE_inventory& load_EVSE_inventory::get_EVSE_inventory() const
{
	return this->EVSE_inv;
}

const EVSE_level load_EVSE_inventory::string_to_EVSE_level(const std::string& str) const
{
	const std::unordered_map<std::string, EVSE_level> map = {
		{"L1", L1},
		{"L2", L2},
		{"DCFC", DCFC}
	};

	auto it = map.find(str);
	if (it != map.end()) {
		return it->second;
	}
	else {
		throw std::invalid_argument("Invalid EVSE level string");
	}
}

const EVSE_phase load_EVSE_inventory::string_to_EVSE_phase(const std::string& str) const
{
	const std::unordered_map<std::string, EVSE_phase> map = {
		{"1", singlephase},
		{"one", singlephase},
		{"3", threephase},
		{"three", threephase}
	};

	auto it = map.find(str);
	if (it != map.end()) {
		return it->second;
	}
	else {
		throw std::invalid_argument("Invalid EVSE level string");
	}
}

EVSE_inventory load_EVSE_inventory::load(const std::string& inputs_dir)
{
	EVSE_inventory EVSE_inv;

	std::string EVSE_inputs_file = inputs_dir + "/EVSE_inputs.csv";
	ASSERT(std::filesystem::exists(EVSE_inputs_file), EVSE_inputs_file << " file does not exist");
	std::ifstream EVSE_inputs_file_handle(EVSE_inputs_file);

	//------------------------------

	std::string line;
	std::vector<std::string> elements_in_line;
	int line_number = 1;

	//-------------------------------

	// column names
	std::getline(EVSE_inputs_file_handle, line);
	elements_in_line = tokenize(line);

	// check number of columns
	ASSERT(elements_in_line.size() == 8, EVSE_inputs_file << " invalid number of columns. Need 8 \
		columns but " << elements_in_line.size() << " provided in line number " << line_number);

	// check if columns equal to { EVSE_type, EVSE_level, EVSE_phase_connection, AC/DC_power_limit_kW, 
	// AC/DC_voltage_limits_V, AC/DC_current_limit_A, standby_real_power_kW, standby_reactive_power_kVAR }
	std::vector<std::string> column_names = { "EVSE_type", "EVSE_level", "EVSE_phase_connection", 
		"AC/DC_power_limit_kW", "AC/DC_voltage_limits_V", "AC/DC_current_limit_A", "standby_real_power_kW", 
		"standby_reactive_power_kVAR" };

	ASSERT(elements_in_line == column_names, EVSE_inputs_file << " column names doesn\'t match. " <<
		"Expected column names are { EVSE_type, EVSE_level, EVSE_phase_connection, AC/DC_power_limit_kW, " <<
		"AC/DC_voltage_limits_V, AC/DC_current_limit_A, standby_real_power_kW, standby_reactive_power_kVAR }");

	while (std::getline(EVSE_inputs_file_handle, line))
	{
		line_number += 1;
		elements_in_line = tokenize(line);
		ASSERT(elements_in_line.size() == 8, EVSE_inputs_file << " invalid number of columns. Need 8 \
			columns but " << elements_in_line.size() << " provided in line number " << line_number);


		const EVSE_type type = [&]() {

			// check if EVSE_type is unique
			std::string str = elements_in_line[0];
			ASSERT(EVSE_inv.find(str) == EVSE_inv.end(), EVSE_inputs_file << " " << column_names[0] << " " <<
				str << " is not unique in line number " << line_number);
			return str;

		}();

		const EVSE_level level = [&]() {

			// check if battery_chemistry is equal to LTO, LMO or NMC
			std::vector<std::string> options = { "L1", "L2", "DCFC" };
			std::string str = elements_in_line[1];
			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EVSE_inputs_file << " " << column_names[1] << " " << str << 
				" is not one of {L1, L2, DCFC} in line number " << line_number);

			return this->string_to_EVSE_level(str);

		}();

		const EVSE_phase phase = [&]() {

			// check if battery_chemistry is equal to LTO, LMO or NMC
			std::vector<std::string> options = { "1", "3", "one", "three"};
			std::string str = elements_in_line[2];
			std::transform(str.begin(), str.end(), str.begin(), ::tolower);

			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EVSE_inputs_file << " " << column_names[2] << " " << str <<
				" is not one of {1, 3, one, three} in line number " << line_number);

			return this->string_to_EVSE_phase(str);

		}();

		const double power_limit_kW = [&]() {

			// check if power_limit_kW can be converted to double and also non negative
			std::string str = elements_in_line[3];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EVSE_inputs_file << " " << column_names[3] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val > 0, EVSE_inputs_file << " " << column_names[3] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);
			return val;

		}();

		const double volatge_limit_V = [&]() {

			// check if volatge_limit_V is double and also non negative
			std::string str = elements_in_line[4];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EVSE_inputs_file << " " << column_names[4] << " " <<
				str << " is not a double in line number " << line_number);

			if(level == DCFC)
			{
				ASSERT(val > 0, EVSE_inputs_file << " " << column_names[4] << " " << str <<
					   " is less than or equal to 0 in line number " << line_number);
			}
			else
			{
				ASSERT(val > 0 || val == -1, EVSE_inputs_file << " " << column_names[4] << " " << str <<
					   " is less than or equal to 0 in line number " << line_number);
			}

			return val;

		}();

		const double current_limit_A = [&]() {

			// check if current_limit_A is double and also non negative
			std::string str = elements_in_line[5];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}

			if (level == DCFC)
			{
				ASSERT(is_conversion_successful, EVSE_inputs_file << " " << column_names[5] << " " <<
					   str << " is not a double in line number " << line_number);
			}
			else
			{
				ASSERT(val > 0 || val == -1, EVSE_inputs_file << " " << column_names[5] << " " << str <<
					   " is less than or equal to 0 in line number " << line_number);
			}

			return val;

		}();

		const double standby_real_power_kW = [&]() {

			// check standby_real_power_kW is double and non negative
			std::string str = elements_in_line[6];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EVSE_inputs_file << " " << column_names[6] << " " << str <<
				" is not a int in line number " << line_number);

			ASSERT(val >= 0, EVSE_inputs_file << " " << column_names[6] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double standby_reactive_power_kW = [&]() {

			// check AC_charge_rate_kW is double
			std::string str = elements_in_line[7];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EVSE_inputs_file << " " << column_names[7] << " " <<
				str << " is not a double in line number " << line_number);

			return val;
		}();

		// other checks

		EVSE_inv.emplace(type, EVSE_characteristics(type, level, phase, power_limit_kW, volatge_limit_V, 
			current_limit_A, standby_real_power_kW, standby_reactive_power_kW));

	}

	return std::move(EVSE_inv);
}
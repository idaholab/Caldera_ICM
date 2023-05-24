#include "load_EV_inventory.h"
#include "helper.h"

#include <fstream>
#include <filesystem>

load_EV_inventory::load_EV_inventory(const std::string& inputs_dir) :
	EV_inv(this->load(inputs_dir))
{}

const EV_inventory& load_EV_inventory::get_EV_inventory() const
{
	return this->EV_inv;
}

battery_chemistry load_EV_inventory::string_to_battery_chemistry(const std::string& str)
{
	const std::unordered_map<std::string, battery_chemistry> map = {
		{"LTO", LTO},
		{"LMO", LMO},
		{"NMC", NMC}
	};

	auto it = map.find(str);
	if (it != map.end()) {
		return it->second;
	}
	else {
		throw std::invalid_argument("Invalid battery chemistry string");
	}
}

bool load_EV_inventory::string_to_DCFC_capable(const std::string& str) {
	if (str == "true" || str == "t") {
		return true;
	}
	else if (str == "false" || str == "f") {
		return false;
	}
	else {
		throw std::invalid_argument("Invalid DCFC capable string");
	}
}

EV_inventory load_EV_inventory::load(const std::string& inputs_dir)
{
	EV_inventory EV_inv;

	std::string EV_inputs_file = inputs_dir + "/EV_inputs.csv";
	ASSERT(std::filesystem::exists(EV_inputs_file), EV_inputs_file << " file does not exist");
	std::ifstream EV_inputs_file_handle(EV_inputs_file);

	//------------------------------

	std::string line;
	std::vector<std::string> elements_in_line;
	int line_number = 1;

	//-------------------------------

	std::vector<std::string> column_names = { "EV_type", "battery_chemistry", "usable_battery_size_kWh",
		"range_miles", "efficiency_Wh/Mile", "AC_charge_rate_kW", "DCFC_capable", "max_c_rate", 
		"pack_voltage_at_peak_power_V" };

	int num_columns = (int)column_names.size();

	//-------------------------------

	// column names
	std::getline(EV_inputs_file_handle, line);
	elements_in_line = tokenize(line);

	// check number of columns
	ASSERT(elements_in_line.size() == num_columns, EV_inputs_file << " invalid number of columns. Need " 
		<< num_columns << " columns but " << elements_in_line.size() << " provided in line number " 
		<< line_number);
	
	ASSERT(elements_in_line == column_names, EV_inputs_file << " column names doesn\'t match. " <<
		"Expected column names are { EV_type, battery_chemistry, usable_battery_size_kWh, " <<
		"range_miles, efficiency_Wh/Mile, AC_charge_rate_kW, DCFC_capable, max_c_rate, " <<
		"pack_voltage_at_peak_power_V}");

	while (std::getline(EV_inputs_file_handle, line))
	{
		line_number += 1;
		elements_in_line = tokenize(line);
		ASSERT(elements_in_line.size() == num_columns, EV_inputs_file << " invalid number of columns. Need "
			<< num_columns << " columns but " << elements_in_line.size() << " provided in line number "
			<< line_number);
		
		const EV_type type = [&]() {

			int column_num = 0;

			// check if EV_type is unique
			std::string str = elements_in_line[column_num];
			ASSERT(EV_inv.find(str) == EV_inv.end(), EV_inputs_file << " " << column_names[column_num] 
				<< " " << str << " is not unique in line number " << line_number);
			return str;

		}();

		const battery_chemistry chemistry = [&]() {

			int column_num = 1;

			// check if battery_chemistry is equal to LTO, LMO or NMC
			std::vector<std::string> options = { "LTO", "LMO", "NMC" };
			std::string str = elements_in_line[column_num];
			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EV_inputs_file << " " << column_names[column_num] << " " << str << " is not \
				one of {LMO, LTO, NMC} in line number " << line_number);

			return this->string_to_battery_chemistry(str);

		}();

		const double usable_battery_size_kWh = [&]() {

			int column_num = 2;

			// check if battery_size_kWh is double and also non negative
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val >= 0, EV_inputs_file << " " << column_names[column_num] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double range_miles = [&]() {

			int column_num = 3;

			// check if range_miles is double and also non negative
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val >= 0, EV_inputs_file << " " << column_names[column_num] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double efficiency_Wh_per_mile = [&]() {

			int column_num = 4;

			// check efficiency_Wh_per_mile is double and non negative
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " << str <<
				" is not a double in line number " << line_number);

			ASSERT(val >= 0, EV_inputs_file << " " << column_names[column_num] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double AC_charge_rate_kW = [&]() {

			int column_num = 5;

			// check AC_charge_rate_kW is double and non negative
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val >= 0, EV_inputs_file << " " << column_names[column_num] << " " << str
				<< " is less than or equal to 0 in line number " << line_number);

			return val;
		}();

		const bool DCFC_capable = [&]() {

			int column_num = 6;

			// check if DCFC_capable is yes or no
			std::vector<std::string> options = { "true", "false", "t", "f"};
			std::string str = elements_in_line[column_num];
			std::transform(str.begin(), str.end(), str.begin(), ::tolower);

			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EV_inputs_file << " " << column_names[column_num] << " " << str <<
				" is not one of {TRUE, FALSE, T, F} case insensitive in line number " << line_number);

			return this->string_to_DCFC_capable(str);

		}();

		
		const double max_c_rate = [&]() {

			int column_num = 7;

			// check if max_c_rate can be converted to double
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " <<
				str << " is not a double in line number " << line_number);
			
			return val;

		}();

		const double pack_voltage_at_peak_power_V = [&]() {

			int column_num = 8;

			// check pack_voltage_at_peak_power_V is double and non negative
			std::string str = elements_in_line[column_num];
			double val;
			bool is_conversion_successful;
			try {
				val = std::stod(str);
				is_conversion_successful = true;
			}
			catch (...) {
				is_conversion_successful = false;
			}
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[column_num] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val >= 0, EV_inputs_file << " " << column_names[column_num] << " " << str
				<< " is less than or equal to 0 in line number " << line_number);

			return val;
		}();
		
		
		// other checks

		EV_inv.emplace(type, EV_characteristics(type, chemistry, usable_battery_size_kWh, range_miles, 
			efficiency_Wh_per_mile, AC_charge_rate_kW, DCFC_capable, max_c_rate, pack_voltage_at_peak_power_V));
	}

	return std::move(EV_inv);
}
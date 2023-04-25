#include "load_EV_inventory.h"
#include "load_helper.h"

#include <fstream>
#include <filesystem>

load_EV_inventory::load_EV_inventory(std::string inputs_dir) :
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

EV_inventory load_EV_inventory::load(std::string inputs_dir)
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

	// column names
	std::getline(EV_inputs_file_handle, line);
	elements_in_line = tokenize(line);

	// check number of columns
	ASSERT(elements_in_line.size() == 8, EV_inputs_file << " invalid number of columns. Need 8 \
		columns but " << elements_in_line.size() << " provided in line number " << line_number);

	// check if columns equal to { EV_type, battery_chemistry, "DCFC_capable", max_c_rate, 
	// battery_size_kWh, range_miles, efficiency_Wh/Mile, AC_charge_rate_kW }
	std::vector<std::string> column_names = { "EV_type", "battery_chemistry", "DCFC_capable",
		"max_c_rate", "battery_size_kWh", "range_miles", "efficiency_Wh/Mile", "AC_charge_rate_kW" };

	ASSERT(elements_in_line == column_names, EV_inputs_file << " column names doesn\'t match. " <<
		"Expected column names are {EV_type, battery_chemistry, DCFC_capable, max_c_rate, " <<
		"battery_size_kWh, range_miles, efficiency_Wh/Mile, AC_charge_rate_kW }");

	while (std::getline(EV_inputs_file_handle, line))
	{
		line_number += 1;
		elements_in_line = tokenize(line);
		ASSERT(elements_in_line.size() == 8, EV_inputs_file << " invalid number of columns. Need 8 \
			columns but " << elements_in_line.size() << " provided in line number " << line_number);
		
		
		const EV_type type = [&]() {

			// check if EV_type is unique
			std::string str = elements_in_line[0];
			ASSERT(EV_inv.find(str) == EV_inv.end(), EV_inputs_file << " " << column_names[0] << " " <<
				str << " is not unique in line number " << line_number);
			return str;

		}();

		const battery_chemistry chemistry = [&]() {

			// check if battery_chemistry is equal to LTO, LMO or NMC
			std::vector<std::string> options = { "LTO", "LMO", "NMC" };
			std::string str = elements_in_line[1];
			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EV_inputs_file << " " << column_names[1] << " " << str << " is not \
				one of {LMO, LTO, NMC} in line number " << line_number);

			return this->string_to_battery_chemistry(str);

		}();

		const bool DCFC_capable = [&]() {

			// check if DCFC_capable is yes or no
			std::vector<std::string> options = { "true", "false", "t", "f"};
			std::string str = elements_in_line[2];
			std::transform(str.begin(), str.end(), str.begin(), ::tolower);

			bool is_valid = std::any_of(options.begin(), options.end(),
				[&str](const std::string& s) { return str == s; });
			ASSERT(is_valid, EV_inputs_file << " " << column_names[2] << " " << str << 
				" is not one of {TRUE, FALSE, T, F} case insensitive in line number " << line_number);

			return this->string_to_DCFC_capable(str);

		}();

		
		const double max_c_rate = [&]() {

			// check if max_c_rate can be converted to double
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
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[3] << " " << 
				str << " is not a double in line number " << line_number);
			
			return val;

		}();

		const double battery_size_kWh = [&]() {

			// check if battery_size_kWh is double and also non negative
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
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[4] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val > 0, EV_inputs_file << " " << column_names[4] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double range_miles = [&]() {

			// check if range_miles is double and also non negative
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
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[5] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val > 0, EV_inputs_file << " " << column_names[5] << " " << str <<
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double efficiency_Wh_per_mile = [&]() {

			// check efficiency_Wh_per_mile is double and non negative
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
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[6] << " " << str <<
				" is not a double in line number " << line_number);

			ASSERT(val > 0, EV_inputs_file << " " << column_names[6] << " " << str << 
				" is less than or equal to 0 in line number " << line_number);

			return val;

		}();

		const double AC_charge_rate_kW = [&](){

			// check AC_charge_rate_kW is double and non negative
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
			ASSERT(is_conversion_successful, EV_inputs_file << " " << column_names[7] << " " <<
				str << " is not a double in line number " << line_number);

			ASSERT(val > 0, EV_inputs_file << " " << column_names[7] << " " << str
				<< " is less than or equal to 0 in line number " << line_number);

			return val;
		}();
		
		// other checks

		EV_inv.emplace(type, EV_characteristics(type, chemistry, DCFC_capable, max_c_rate, battery_size_kWh,
			range_miles, efficiency_Wh_per_mile, AC_charge_rate_kW));

	}

	return std::move(EV_inv);
}
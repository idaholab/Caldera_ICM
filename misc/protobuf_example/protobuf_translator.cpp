
#include "protobuf_translator.h"
#include "protobuf_datatypes.pb.h"

#include <vector>


bool protobuf_translator::GM0085_aggregator_charging_forecast_serialize(GM0085_aggregator_charging_forecast& cpp_object, std::string& serialized_object)
{
	std::vector<double>& a_time = cpp_object.arrival_time;
	std::vector<double>& d_time = cpp_object.departure_time;
	std::vector<double>& e_remain = cpp_object.e_charge_remain_kWh;
	std::vector<double>& e_step = cpp_object.e_step_max_kWh;

	protobuf::GM0085_aggregator_charging_forecast pb_object;
	
	int size_val = a_time.size();
	for(int i=0; i<size_val; i++)
	{
		pb_object.add_arrival_time(a_time[i]);
		pb_object.add_departure_time(d_time[i]);
		pb_object.add_e_charge_remain_kwh(e_remain[i]);
		pb_object.add_e_step_max_kwh(e_step[i]);
	}

	//---------------------------------------

	bool success = pb_object.SerializeToString(&serialized_object);

	if(!success)
		serialized_object = "";

	return success;
}


bool protobuf_translator::GM0085_aggregator_charging_forecast_deserialize(std::string& serialized_object, GM0085_aggregator_charging_forecast& cpp_object)
{
	std::vector<double>& a_time = cpp_object.arrival_time;
	std::vector<double>& d_time = cpp_object.departure_time;
	std::vector<double>& e_remain = cpp_object.e_charge_remain_kWh;
	std::vector<double>& e_step = cpp_object.e_step_max_kWh;

	a_time.clear();
	d_time.clear();
	e_remain.clear();
	e_step.clear();

	//--------------------------

	protobuf::GM0085_aggregator_charging_forecast pb_object;
	bool success = pb_object.ParseFromString(serialized_object);

	if(!success)
		return success;

	//-------------------------

	int size_val = pb_object.arrival_time_size();
	for(int i=0; i<size_val; i++)
	{
		a_time.push_back(pb_object.arrival_time(i));
		d_time.push_back(pb_object.departure_time(i));
		e_remain.push_back(pb_object.e_charge_remain_kwh(i));
		e_step.push_back(pb_object.e_step_max_kwh(i));
	}

	return success;
}

	
bool protobuf_translator::GM0085_aggregator_charging_needs_serialize(GM0085_aggregator_charging_needs& cpp_object, std::string& serialized_object)
{
	std::vector<int>& SE_id = cpp_object.SE_id;
	std::vector<double>& d_time = cpp_object.departure_time;
	std::vector<double>& e_remain = cpp_object.e_charge_remain_kWh;
	std::vector<double>& e_max = cpp_object.e_step_max_kWh;
	std::vector<double>& e_target = cpp_object.e_step_target_kWh;
	std::vector<double>& r_charge_time = cpp_object.min_remaining_charge_time_hrs;
	std::vector<double>& t_charge_time = cpp_object.min_time_to_complete_entire_charge_hrs;
	std::vector<double>& r_park_time = cpp_object.remaining_park_time_hrs;
	std::vector<double>& t_park_time = cpp_object.total_park_time_hrs;

	protobuf::GM0085_aggregator_charging_needs pb_object;
	pb_object.set_next_aggregator_timestep_start_time(cpp_object.next_aggregator_timestep_start_time);

	//----------------------------------------

	int size_val = SE_id.size();
	for(int i=0; i<size_val; i++)
	{
		pb_object.add_se_id(SE_id[i]);
		pb_object.add_departure_time(d_time[i]);
		pb_object.add_e_charge_remain_kwh(e_remain[i]);
		pb_object.add_e_step_max_kwh(e_max[i]);
		pb_object.add_e_step_target_kwh(e_target[i]);
		pb_object.add_min_remaining_charge_time_hrs(r_charge_time[i]);
		pb_object.add_min_time_to_complete_entire_charge_hrs(t_charge_time[i]);
		pb_object.add_remaining_park_time_hrs(r_park_time[i]);
		pb_object.add_total_park_time_hrs(t_park_time[i]);
	}

	//----------------------------------------

	bool success = pb_object.SerializeToString(&serialized_object);

	if(!success)
		serialized_object = "";

	return success;
}


bool protobuf_translator::GM0085_aggregator_charging_needs_deserialize(std::string& serialized_object, GM0085_aggregator_charging_needs& cpp_object)
{
	std::vector<int>& SE_id = cpp_object.SE_id;
	std::vector<double>& d_time = cpp_object.departure_time;
	std::vector<double>& e_remain = cpp_object.e_charge_remain_kWh;
	std::vector<double>& e_max = cpp_object.e_step_max_kWh;
	std::vector<double>& e_target = cpp_object.e_step_target_kWh;
	std::vector<double>& r_charge_time = cpp_object.min_remaining_charge_time_hrs;
	std::vector<double>& t_charge_time = cpp_object.min_time_to_complete_entire_charge_hrs;
	std::vector<double>& r_park_time = cpp_object.remaining_park_time_hrs;
	std::vector<double>& t_park_time = cpp_object.total_park_time_hrs;

	SE_id.clear();
	d_time.clear();
	e_remain.clear();
	e_max.clear();
	e_target.clear();
	r_charge_time.clear();
	t_charge_time.clear();
	r_park_time.clear();
	t_park_time.clear();

	//----------------------------------

	protobuf::GM0085_aggregator_charging_needs pb_object;
	bool success = pb_object.ParseFromString(serialized_object);

	if(!success)
		return success;

	//----------------------------------
    
    cpp_object.next_aggregator_timestep_start_time = pb_object.next_aggregator_timestep_start_time();

	int size_val = pb_object.se_id_size();
	for(int i=0; i<size_val; i++)
	{
		SE_id.push_back(pb_object.se_id(i));
		d_time.push_back(pb_object.departure_time(i));
		e_remain.push_back(pb_object.e_charge_remain_kwh(i));
		e_max.push_back(pb_object.e_step_max_kwh(i));
		e_target.push_back(pb_object.e_step_target_kwh(i));
		r_charge_time.push_back(pb_object.min_remaining_charge_time_hrs(i));
		t_charge_time.push_back(pb_object.min_time_to_complete_entire_charge_hrs(i));
		r_park_time.push_back(pb_object.remaining_park_time_hrs(i));
		t_park_time.push_back(pb_object.total_park_time_hrs(i));
	}

	return success;
}


bool protobuf_translator::GM0085_aggregator_e_step_setpoints_serialize(GM0085_aggregator_e_step_setpoints& cpp_object, std::string& serialized_object)
{
	std::vector<int>& SE_id = cpp_object.SE_id;
	std::vector<double>& e_step = cpp_object.e_step_kWh;
	std::vector<double>& cp = cpp_object.charge_progression;
	
	protobuf::GM0085_aggregator_e_step_setpoints pb_object;
	pb_object.set_next_aggregator_timestep_start_time(cpp_object.next_aggregator_timestep_start_time);
	
	int size_val = SE_id.size();
	for(int i=0; i<size_val; i++)
	{
		pb_object.add_se_id(SE_id[i]);
		pb_object.add_e_step_kwh(e_step[i]);
		pb_object.add_charge_progression(cp[i]);
	}

	//---------------------------------------

	bool success = pb_object.SerializeToString(&serialized_object);

	if(!success)
		serialized_object = "";

	return success;
}


bool protobuf_translator::GM0085_aggregator_e_step_setpoints_deserialize(std::string& serialized_object, GM0085_aggregator_e_step_setpoints& cpp_object)
{
	std::vector<int>& SE_id = cpp_object.SE_id;
	std::vector<double>& e_step = cpp_object.e_step_kWh;
	std::vector<double>& cp = cpp_object.charge_progression;

	SE_id.clear();
	e_step.clear();
	cp.clear();

	//--------------------------

	protobuf::GM0085_aggregator_e_step_setpoints pb_object;
	bool success = pb_object.ParseFromString(serialized_object);

	if(!success)
		return success;

	//-------------------------
    
    cpp_object.next_aggregator_timestep_start_time = pb_object.next_aggregator_timestep_start_time();

	int size_val = pb_object.se_id_size();
	for(int i=0; i<size_val; i++)
	{
		SE_id.push_back(pb_object.se_id(i));
		e_step.push_back(pb_object.e_step_kwh(i));
		cp.push_back(pb_object.charge_progression(i));
	}

	return success;
}


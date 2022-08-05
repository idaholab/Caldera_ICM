
#ifndef protobuf_translator_H
#define protobuf_translator_H

#include <string>
#include "global_datatypes.h"

class protobuf_translator
{
private:

public:
	bool GM0085_aggregator_charging_forecast_serialize(GM0085_aggregator_charging_forecast& cpp_object, std::string& serialized_object);
	bool GM0085_aggregator_charging_forecast_deserialize(std::string& serialized_object, GM0085_aggregator_charging_forecast& cpp_object);
	
	bool GM0085_aggregator_charging_needs_serialize(GM0085_aggregator_charging_needs& cpp_object, std::string& serialized_object);	
	bool GM0085_aggregator_charging_needs_deserialize(std::string& serialized_object, GM0085_aggregator_charging_needs& cpp_object);

	bool GM0085_aggregator_e_step_setpoints_serialize(GM0085_aggregator_e_step_setpoints& cpp_object, std::string& serialized_object);	
	bool GM0085_aggregator_e_step_setpoints_deserialize(std::string& serialized_object, GM0085_aggregator_e_step_setpoints& cpp_object);
};


#endif



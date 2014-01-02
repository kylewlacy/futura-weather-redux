#ifndef futura_weather_redux_utils_h
#define futura_weather_redux_utils_h

#include "pebble.h"

uint32_t get_resource_for_weather_conditions(uint32_t conditions);
uint32_t get_resource_for_battery_state(BatteryChargeState battery);

#endif

#ifndef futura_weather_redux_weather_h
#define futura_weather_redux_weather_h

#include "pebble.h"

typedef struct {
	time_t last_update_time;
	int temperature;
	int conditions;
} Weather;

Weather* weather_load_cache();
bool weather_save_cache(Weather *weather);

bool weather_set(Weather *weather, DictionaryIterator *iter);
bool weather_needs_update(Weather *weather, time_t update_freq);
void weather_request_update();

int weather_convert_temperature(int kelvin_temperature, TempFormat format);

#endif

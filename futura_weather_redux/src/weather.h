#ifndef futura_weather_redux_weather_h
#define futura_weather_redux_weather_h

#include "pebble.h"

typedef struct {
	time_t last_update_time;
	int temperature;
	int conditions;
} Weather;

Weather* weather_create();
bool weather_set(Weather *weather, DictionaryIterator *iter);
bool weather_needs_update(Weather *weather, time_t update_freq);
void weather_request_update();

#endif

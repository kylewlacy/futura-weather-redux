#ifndef futura_weather_redux_weather_h
#define futura_weather_redux_weather_h

#include "pebble.h"

typedef enum {
	WEATHER_CONDITIONS_UNAVAILABLE   = 0,
	WEATHER_CONDITIONS_CLEAR         = 1,
	WEATHER_CONDITIONS_PARTLY_CLOUDY = 2,
	WEATHER_CONDITIONS_CLOUDY        = 3,
	WEATHER_CONDITIONS_FOG           = 4,
	WEATHER_CONDITIONS_DRIZZLE       = 5,
	WEATHER_CONDITIONS_RAIN          = 6,
	WEATHER_CONDITIONS_RAIN_SLEET    = 7,
	WEATHER_CONDITIONS_RAIN_SNOW     = 8,
	WEATHER_CONDITIONS_SLEET         = 9,
	WEATHER_CONDITIONS_SNOW_SLEET    = 10,
	WEATHER_CONDITIONS_SNOW          = 11,
	WEATHER_CONDITIONS_THUNDER       = 12,
	WEATHER_CONDITIONS_WIND          = 13,
	WEATHER_CONDITIONS_HOT           = 14,
	WEATHER_CONDITIONS_COLD          = 15
} WeatherConditionCode;

typedef enum {
	WEATHER_CONDITION_FLAGS_NONE   = 0,
	WEATHER_CONDITION_FLAGS_IS_DAY = 1 << 0
} WeatherConditionFlags;

typedef struct {
	WeatherConditionCode code;
	WeatherConditionFlags flags;
} WeatherConditions;

typedef struct {
	time_t last_update_time;
	int temperature;
	WeatherConditions conditions;
} Weather;



Weather* weather_load_cache();
bool weather_save_cache(Weather *weather);

void weather_set(Weather *weather, DictionaryIterator *iter);
bool weather_needs_update(Weather *weather, time_t update_freq);
void weather_request_update();

int weather_convert_temperature(int kelvin_temperature, TempFormat format);

#endif

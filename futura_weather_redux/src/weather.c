#include "pebble.h"
#include "config.h"
#include "weather.h"

bool force_update = false;

Weather* weather_load_cache() {
	static Weather weather = {
		.last_update_time = 0,
		.temperature = -46100,				// Below absolute zero in Kelvin, Celsius, Fahrenheit, Rankine, Delisle, Newton, Rèaumur, and Rømer
		.conditions = 0
	};
	
	if(persist_exists(WEATHER_CACHE_LAST_UPDATE_PERSIST_KEY))
		weather.last_update_time = persist_read_int(WEATHER_CACHE_LAST_UPDATE_PERSIST_KEY);
	if(persist_exists(WEATHER_CACHE_TEMPERATURE_PERSIST_KEY))
		weather.temperature = persist_read_int(WEATHER_CACHE_TEMPERATURE_PERSIST_KEY);
	if(persist_exists(WEATHER_CACHE_CONDITIONS_PERSIST_KEY))
		weather.conditions = persist_read_int(WEATHER_CACHE_CONDITIONS_PERSIST_KEY);
	
	return &weather;
}

bool weather_save_cache(Weather *weather) {
	status_t save_last_update = persist_write_int(WEATHER_CACHE_LAST_UPDATE_PERSIST_KEY, (int)weather->last_update_time);
	status_t save_temperature = persist_write_int(WEATHER_CACHE_TEMPERATURE_PERSIST_KEY, weather->temperature);
	status_t save_conditions = persist_write_int(WEATHER_CACHE_CONDITIONS_PERSIST_KEY, weather->conditions);
	
	return save_last_update >= 0 && save_temperature >= 0 && save_conditions >= 0;
}



bool weather_needs_update(Weather *weather, time_t update_freq) {
	time_t now = time(NULL);
	return force_update || (now - weather->last_update_time >= update_freq);
}

void weather_request_update() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    Tuplet request = TupletInteger(REQUEST_WEATHER_KEY, 1);
    dict_write_tuplet(iter, &request);
    
    app_message_outbox_send();
}

void weather_set(Weather *weather, DictionaryIterator *iter) {
	Tuple *conditions = dict_find(iter, WEATHER_CONDITIONS_KEY);
	Tuple *temperature = dict_find(iter, WEATHER_TEMPERATURE_KEY);
	
	if(conditions)
		weather->conditions = conditions->value->int32;
	if(temperature)
		weather->temperature = temperature->value->int32;
	
	time_t now = time(NULL);
	weather->last_update_time = now;
	weather_save_cache(weather);
}



int weather_convert_temperature(int kelvin_temperature, TempFormat format) {
	float true_temperature = kelvin_temperature / 100.0f;				// We receive the temperature as an int for simplicity, but *100 to maintain accuracy
	switch(format) {
		case TEMP_FORMAT_CELCIUS:
			return true_temperature - 273.15f;
		case TEMP_FORMAT_FAHRENHEIT:
			return (9.0f/5.0f)*true_temperature - 459.67f;
	}
	
	APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown temperature format %d, using Kelvin", format);
	return true_temperature;
}


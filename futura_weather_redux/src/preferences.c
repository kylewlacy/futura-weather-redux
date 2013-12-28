#include "pebble.h"
#include "config.h"
#include "weather.h"
#include "preferences.h"

Preferences* preferences_load() {
	static Preferences prefs = {
		.temp_format = TEMP_FORMAT_CELCIUS,
		.weather_update_frequency = 10*60
	};
	
	if(persist_exists(TEMP_PREFERENCE_KEY))
		prefs.temp_format = persist_read_int(TEMP_PREFERENCE_KEY);
	if(persist_exists(WEATHER_UPDATE_PREFERENCE_KEY))
		prefs.weather_update_frequency = persist_read_int(WEATHER_UPDATE_PREFERENCE_KEY);
	
	return &prefs;
}

bool preferences_save(Preferences *prefs) {
	status_t save_temp = persist_write_int(TEMP_PREFERENCE_KEY, prefs->temp_format);
	status_t save_weather_update = persist_write_int(WEATHER_UPDATE_PREFERENCE_KEY, (int)prefs->weather_update_frequency);
	
	return save_temp >= 0 && save_weather_update >= 0;
}

void preferences_send(Preferences *prefs) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	Tuplet request = TupletInteger(SET_PREFERENCES_KEY, 1);
	dict_write_tuplet(iter, &request);
	
	Tuplet temp_format = TupletInteger(TEMP_PREFERENCE_KEY, prefs->temp_format);
	dict_write_tuplet(iter, &temp_format);
	
	app_message_outbox_send();
}

bool preferences_set(Preferences *prefs, DictionaryIterator *iter) {
	Tuple *temp_format = dict_find(iter, TEMP_PREFERENCE_KEY);
	Tuple *weather_update_frequency = dict_find(iter, WEATHER_UPDATE_PREFERENCE_KEY);
	
	if(temp_format && (temp_format->value->int32 != prefs->temp_format))
		prefs->temp_format = temp_format->value->int32;
	if(weather_update_frequency && (weather_update_frequency->value->int32 != (int)prefs->weather_update_frequency))
		prefs->weather_update_frequency = weather_update_frequency->value->int32;
	
	return temp_format || weather_update_frequency;
}

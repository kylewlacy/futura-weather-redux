#include "pebble.h"
#include "config.h"
#include "weather.h"
#include "preferences.h"

Preferences* preferences_load() {
	static Preferences prefs = {
		.temp_format = TEMP_FORMAT_CELCIUS,
		.weather_update_freq = 10*60
	};
	
	if(persist_exists(PREF_TEMP_FORMAT_PERSIST_KEY))
		prefs.temp_format = persist_read_int(PREF_TEMP_FORMAT_PERSIST_KEY);
	if(persist_exists(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY))
		prefs.weather_update_freq = persist_read_int(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY);
	
	return &prefs;
}

bool preferences_save(Preferences *prefs) {
	status_t save_temp = persist_write_int(PREF_TEMP_FORMAT_PERSIST_KEY, prefs->temp_format);
	status_t save_weather_update = persist_write_int(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY, (int)prefs->weather_update_freq);
	
	if(save_temp < 0 || save_weather_update < 0) {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to save preferences");
		return false;
	}
	return true;
}

void preferences_send(Preferences *prefs) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	Tuplet request = TupletInteger(SET_PREFERENCES_MSG_KEY, 1);
	dict_write_tuplet(iter, &request);
	
	Tuplet temp_format = TupletInteger(TEMP_FORMAT_MSG_KEY, prefs->temp_format);
	dict_write_tuplet(iter, &temp_format);
	
	Tuplet weather_update_frequency = TupletInteger(WEATHER_UPDATE_FREQ_MSG_KEY, (int)prefs->weather_update_freq);
	dict_write_tuplet(iter, &weather_update_frequency);
	
	app_message_outbox_send();
}

void preferences_set(Preferences *prefs, DictionaryIterator *iter) {
	Tuple *temp_format = dict_find(iter, TEMP_FORMAT_MSG_KEY);
	Tuple *weather_update_frequency = dict_find(iter, WEATHER_UPDATE_FREQ_MSG_KEY);
	
	if(temp_format)
		prefs->temp_format = temp_format->value->int32;
	if(weather_update_frequency)
		prefs->weather_update_freq = weather_update_frequency->value->int32;
}

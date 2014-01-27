#include "pebble.h"
#include "config.h"
#include "weather.h"
#include "preferences.h"

Preferences* preferences_load() {
	static Preferences prefs = {
		.temp_format = TEMP_FORMAT_CELCIUS,
		.weather_update_freq = 10*60,
		.statusbar = false,
		.weather_provider = 1,
		.weather_outdated_time = 60*60,
		.language_code = 0,
		.translation = "Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,Mon,Tue,Wed,Thu,Fri,Sat,Sun"
	};
	
	if(persist_exists(PREF_TEMP_FORMAT_PERSIST_KEY))
		prefs.temp_format = persist_read_int(PREF_TEMP_FORMAT_PERSIST_KEY);
	if(persist_exists(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY))
		prefs.weather_update_freq = persist_read_int(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY);
	if(persist_exists(PREF_STATUSBAR_PERSIST_KEY))
		prefs.statusbar = persist_read_bool(PREF_STATUSBAR_PERSIST_KEY);
	if(persist_exists(PREF_WEATHER_PROVIDER_PERSIST_KEY))
		prefs.weather_provider = persist_read_int(PREF_WEATHER_PROVIDER_PERSIST_KEY);
	if(persist_exists(PREF_WEATHER_OUTDATED_TIME_PERSIST_KEY))
		prefs.weather_outdated_time = persist_read_int(PREF_WEATHER_OUTDATED_TIME_PERSIST_KEY);
	if(persist_exists(PREF_LANGUAGE_CODE_PERSIST_KEY))
		prefs.language_code = persist_read_int(PREF_LANGUAGE_CODE_PERSIST_KEY);
	if(persist_exists(PREF_TRANSLATION_PERSIST_KEY))
		persist_read_string(PREF_TRANSLATION_PERSIST_KEY, prefs.translation, sizeof(prefs.translation));
	
	
	return &prefs;
}

bool preferences_save(Preferences *prefs) {
	status_t temp_format = persist_write_int(PREF_TEMP_FORMAT_PERSIST_KEY, prefs->temp_format);
	status_t weather_update_freq = persist_write_int(PREF_WEATHER_UPDATE_FREQ_PERSIST_KEY, (int)prefs->weather_update_freq);
	status_t statusbar = persist_write_bool(PREF_STATUSBAR_PERSIST_KEY, prefs->statusbar);
	status_t weather_provider = persist_write_bool(PREF_WEATHER_PROVIDER_PERSIST_KEY, prefs->weather_provider);
	status_t weather_outdated_time = persist_write_int(PREF_WEATHER_OUTDATED_TIME_PERSIST_KEY, (int)prefs->weather_outdated_time);
	status_t language_code = persist_write_int(PREF_LANGUAGE_CODE_PERSIST_KEY, prefs->language_code);
	status_t translation = persist_write_string(PREF_TRANSLATION_PERSIST_KEY, prefs->translation);
	
	if(
	   temp_format < 0 || weather_update_freq < 0 || statusbar < 0 || weather_provider < 0 ||
	   weather_outdated_time < 0 || language_code < 0 || translation < 0
	) {
		APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to save preferences");
		return false;
	}
	return true;
}

void preferences_send(Preferences *prefs) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	Tuplet request = TupletInteger(SET_PREFERENCES_MSG_KEY, 1);
	Tuplet temp_format = TupletInteger(TEMP_FORMAT_MSG_KEY, prefs->temp_format);
	Tuplet weather_update_frequency = TupletInteger(WEATHER_UPDATE_FREQ_MSG_KEY, (int)prefs->weather_update_freq);
	Tuplet statusbar = TupletInteger(STATUSBAR_MSG_KEY, prefs->statusbar ? 1 : 0);
	Tuplet weather_provider = TupletInteger(WEATHER_PROVIDER_MSG_KEY, prefs->weather_provider);
	Tuplet weather_outdated_time = TupletInteger(WEATHER_OUTDATED_TIME_MSG_KEY, (int)prefs->weather_outdated_time);
	Tuplet language_code = TupletInteger(LANGUAGE_CODE_MSG_KEY, prefs->language_code);
	
	dict_write_tuplet(iter, &request);
	dict_write_tuplet(iter, &temp_format);
	dict_write_tuplet(iter, &weather_update_frequency);
	dict_write_tuplet(iter, &statusbar);
	dict_write_tuplet(iter, &weather_provider);
	dict_write_tuplet(iter, &weather_outdated_time);
	dict_write_tuplet(iter, &language_code);
	
	app_message_outbox_send();
}

void preferences_set(Preferences *prefs, DictionaryIterator *iter) {
	Tuple *temp_format = dict_find(iter, TEMP_FORMAT_MSG_KEY);
	Tuple *weather_update_frequency = dict_find(iter, WEATHER_UPDATE_FREQ_MSG_KEY);
	Tuple *statusbar = dict_find(iter, STATUSBAR_MSG_KEY);
	Tuple *weather_provider = dict_find(iter, WEATHER_PROVIDER_MSG_KEY);
	Tuple *weather_outdated_time = dict_find(iter, WEATHER_OUTDATED_TIME_MSG_KEY);
	Tuple *language_code = dict_find(iter, LANGUAGE_CODE_MSG_KEY);
	Tuple *translation = dict_find(iter, TRANSLATION_MSG_KEY);
	
	if(temp_format != NULL)
		prefs->temp_format = temp_format->value->int32;
	if(weather_update_frequency != NULL)
		prefs->weather_update_freq = weather_update_frequency->value->int32;
	if(statusbar != NULL)
		prefs->statusbar = (statusbar->value->int32 != 0);
	if(weather_provider != NULL)
		prefs->weather_provider = weather_provider->value->int32;
	if(weather_outdated_time != NULL)
		prefs->weather_outdated_time = weather_outdated_time->value->int32;
	if(language_code != NULL)
		prefs->language_code = language_code->value->int32;
	if(translation != NULL)
		strcpy(prefs->translation, translation->value->cstring);
	
	APP_LOG(APP_LOG_LEVEL_INFO, "%s", prefs->translation);
}

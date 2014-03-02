#ifndef futura_weather_redux_preferences_h
#define futura_weather_redux_preferences_h

#include "pebble.h"

typedef struct {
	TempFormat temp_format;
	time_t weather_update_freq;
	bool statusbar;
	int weather_provider;
	time_t weather_outdated_time;
	int language_code;
	char translation[256];
} Preferences;

Preferences* preferences_load();
bool preferences_save(Preferences *prefs);
void preferences_send(Preferences *prefs);
void preferences_set(Preferences *prefs, DictionaryIterator *iter);

#endif

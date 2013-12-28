#ifndef futura_weather_redux_main_h
#define futura_weather_redux_main_h

#include "pebble.h"

enum AppMessageKey {
	REQUEST_WEATHER_KEY = 1,
	SET_WEATHER_KEY = 2,
    WEATHER_TEMPERATURE_KEY = 3,
    WEATHER_CONDITIONS_KEY = 4,
	REQUEST_PREFERENCES_KEY = 5,
	SET_PREFERENCES_KEY = 6,
	TEMP_PREFERENCE_KEY = 7,
	WEATHER_UPDATE_PREFERENCE_KEY = 8
};

typedef enum {
	TEMP_FORMAT_CELCIUS = 1,
	TEMP_FORMAT_FAHRENHEIT = 2
} TempFormat;

typedef struct {
	TempFormat temp_format;
	time_t weather_update_frequency;
} Preferences;



void load_preferences();
void save_preferences();
void send_preferences();

bool need_weather_update();
void request_weather_update();
void update_weather_info(int conditions, int temperature);
uint32_t get_resource_for_weather_conditions(uint32_t conditions);

void out_sent_handler(DictionaryIterator *sent, void *context);
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
void in_received_handler(DictionaryIterator *received, void *context);
void in_dropped_handler(AppMessageResult reason, void *context);

int main();
void init();
void window_load(Window *window);
void window_unload(Window *window);
void deinit();

void handle_tick(struct tm *now, TimeUnits units_changed);

#endif

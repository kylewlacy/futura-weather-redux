#ifndef futura_weather_redux_main_h
#define futura_weather_redux_main_h

#include "pebble.h"

void load_preferences();
void save_preferences();
void send_preferences();

void update_weather_info(Weather *weather);
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

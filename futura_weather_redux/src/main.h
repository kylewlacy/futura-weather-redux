#ifndef futura_weather_redux_main_h
#define futura_weather_redux_main_h

#include "pebble.h"

#define ALL_UNITS SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT

void load_preferences();
void save_preferences();
void send_preferences();

uint32_t get_resource_for_weather_conditions(uint32_t conditions);
uint32_t get_resource_for_battery_state(BatteryChargeState battery);
uint32_t get_resource_for_bluetooth_connection(bool connected);

GRect get_statusbar_frame(Preferences *prefs);
GRect get_time_frame(Preferences *prefs, bool weather_visible);
GRect get_date_frame(Preferences *prefs, bool weather_visible);
GRect get_weather_frame(bool weather_visible);

void change_preferences(Preferences *old_prefs, Preferences *new_prefs);
void set_weather_visible(bool visible, bool animate);
void set_weather_visible_animation_stopped_handler(Animation *animation, bool finished, void *context);


void update_weather_info(Weather *weather);

void out_sent_handler(DictionaryIterator *sent, void *context);
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
void in_received_handler(DictionaryIterator *received, void *context);
void in_dropped_handler(AppMessageResult reason, void *context);

void animation_stopped_handler(Animation *animation, bool finished, void *context);

int main();
void init();
void window_load(Window *window);
void window_unload(Window *window);
void deinit();

void handle_tick(struct tm *now, TimeUnits units_changed);
void handle_battery(BatteryChargeState battery);
void handle_bluetooth(bool connected);

void force_tick(TimeUnits units_changed);

#endif

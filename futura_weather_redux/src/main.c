#include "pebble.h"

#include "config.h"
#include "preferences.h"
#include "weather.h"
#include "main.h"

static Window *window;

GFont futura_18;
GFont futura_25;
GFont futura_28;
GFont futura_35;
GFont futura_40;
GFont futura_53;

static Layer *statusbar_layer;
static BitmapLayer *statusbar_battery_layer;
static BitmapLayer *statusbar_connection_layer;
static uint32_t statusbar_battery_resource;
static uint32_t statusbar_connection_resource;
static GBitmap *statusbar_battery_bitmap = NULL;
static GBitmap *statusbar_connection_bitmap = NULL;
static GRect default_statusbar_frame;

static TextLayer *time_layer;
GRect default_time_frame;

static TextLayer *date_layer;
GRect default_date_frame;

static Layer *weather_layer;
static TextLayer *weather_temperature_layer;
static BitmapLayer *weather_icon_layer;
static GBitmap *weather_icon_bitmap = NULL;
GRect default_weather_frame;

static Preferences *prefs;
static Weather *weather;



uint32_t get_resource_for_weather_conditions(WeatherConditions  conditions) {
	bool is_day = conditions.flags & WEATHER_CONDITION_FLAGS_IS_DAY;
    switch(conditions.code) {
		case WEATHER_CONDITIONS_CLEAR:
			return is_day ? RESOURCE_ID_WEATHER_CLEAR_DAY : RESOURCE_ID_WEATHER_CLEAR_NIGHT;
		case WEATHER_CONDITIONS_PARTLY_CLOUDY:
			return is_day ? RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY : RESOURCE_ID_WEATHER_PARTLY_CLOUDY_NIGHT;
		case WEATHER_CONDITIONS_CLOUDY:
			return RESOURCE_ID_WEATHER_CLOUDY;
		case WEATHER_CONDITIONS_FOG:
			return RESOURCE_ID_WEATHER_FOG;
		case WEATHER_CONDITIONS_DRIZZLE:
			return RESOURCE_ID_WEATHER_DRIZZLE;
		case WEATHER_CONDITIONS_RAIN:
			return RESOURCE_ID_WEATHER_RAIN;
		case WEATHER_CONDITIONS_RAIN_SLEET:
			return RESOURCE_ID_WEATHER_RAIN_SLEET;
		case WEATHER_CONDITIONS_RAIN_SNOW:
			return RESOURCE_ID_WEATHER_RAIN_SNOW;
		case WEATHER_CONDITIONS_SLEET:
			return RESOURCE_ID_WEATHER_SLEET;
		case WEATHER_CONDITIONS_SNOW_SLEET:
			return RESOURCE_ID_WEATHER_SNOW_SLEET;
		case WEATHER_CONDITIONS_SNOW:
			return RESOURCE_ID_WEATHER_SNOW;
		case WEATHER_CONDITIONS_THUNDER:
			return RESOURCE_ID_WEATHER_THUNDER;
		case WEATHER_CONDITIONS_WIND:
			return RESOURCE_ID_WEATHER_WIND;
		case WEATHER_CONDITIONS_HOT:
			return RESOURCE_ID_WEATHER_HOT;
		case WEATHER_CONDITIONS_COLD:
			return RESOURCE_ID_WEATHER_COLD;
		case WEATHER_CONDITIONS_UNAVAILABLE:
			APP_LOG(APP_LOG_LEVEL_WARNING, "Weather conditions unavailable");
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown weather condition code: %d", conditions.code);
			break;
    }
    
    return RESOURCE_ID_ICON_CLOUD_ERROR;
}

uint32_t get_resource_for_battery_state(BatteryChargeState battery) {
	if(battery.is_plugged && !battery.is_charging)
		return RESOURCE_ID_PLUGGED_IN;
	
	if(battery.charge_percent >= 100)
		return battery.is_charging ? RESOURCE_ID_CHARGING_100 : RESOURCE_ID_BATTERY_100;
	if(battery.charge_percent >= 90)
		return battery.is_charging ? RESOURCE_ID_CHARGING_90  : RESOURCE_ID_BATTERY_90;
	if(battery.charge_percent >= 80)
		return battery.is_charging ? RESOURCE_ID_CHARGING_80  : RESOURCE_ID_BATTERY_80;
	if(battery.charge_percent >= 70)
		return battery.is_charging ? RESOURCE_ID_CHARGING_70  : RESOURCE_ID_BATTERY_70;
	if(battery.charge_percent >= 60)
		return battery.is_charging ? RESOURCE_ID_CHARGING_60  : RESOURCE_ID_BATTERY_60;
	if(battery.charge_percent >= 50)
		return battery.is_charging ? RESOURCE_ID_CHARGING_50  : RESOURCE_ID_BATTERY_50;
	if(battery.charge_percent >= 40)
		return battery.is_charging ? RESOURCE_ID_CHARGING_40  : RESOURCE_ID_BATTERY_40;
	if(battery.charge_percent >= 30)
		return battery.is_charging ? RESOURCE_ID_CHARGING_30  : RESOURCE_ID_BATTERY_30;
	if(battery.charge_percent >= 20)
		return battery.is_charging ? RESOURCE_ID_CHARGING_20  : RESOURCE_ID_BATTERY_20;
	if(battery.charge_percent >= 10)
		return battery.is_charging ? RESOURCE_ID_CHARGING_10  : RESOURCE_ID_BATTERY_10;
	
	return battery.is_charging ? RESOURCE_ID_CHARGING_0 : RESOURCE_ID_BATTERY_0;
}

uint32_t get_resource_for_connection(bool bluetooth, bool internet) {
	if(!bluetooth)
		return RESOURCE_ID_NO_BLUETOOTH;
	return internet ? RESOURCE_ID_BLUETOOTH_INTERNET : RESOURCE_ID_BLUETOOTH_NO_INTERNET;
}



GRect get_statusbar_frame(Preferences *prefs) {
	GRect frame = default_statusbar_frame;
	
	if(!prefs->statusbar)
		frame.origin.y -= frame.size.h;
	return frame;
}

GRect get_time_frame(Preferences *prefs, bool weather_visible) {
	GRect frame = default_time_frame;
	
	if(weather_visible && prefs->statusbar)
		frame.origin.y += 8;
	else if(!weather_visible)
		frame.origin.y = 30;
	
	return frame;
}

GRect get_date_frame(Preferences *prefs, bool weather_visible) {
	GRect frame = default_date_frame;
	
	if(weather_visible && prefs->statusbar)
		frame.origin.y += 4;
	else if(!weather_visible)
		frame.origin.y = 103;
	
	return frame;
}

GRect get_weather_frame(bool weather_visible) {
	GRect frame = default_weather_frame;
	
	if(!weather_visible)
		frame.origin.y = 168; // Pebble screen height
	return frame;
}



bool has_internet_connection(Weather *weather) {
	// If weather has needed an update for a minute or over, then the
	// user likely doesn't have an internet connection (since the
	// phone should have already responded with new weather info).
	bool has_internet = !weather_needs_update(weather, prefs->weather_update_freq + 60);
	return has_internet;
}



void change_preferences(Preferences *old_prefs, Preferences *new_prefs) {
	// old_prefs will be NULL for initialization (app first loaded)
	if(old_prefs == NULL || old_prefs->temp_format != new_prefs->temp_format) {
		if(!weather_needs_update(weather, new_prefs->weather_update_freq))
			update_weather_info(weather, old_prefs != NULL);
	}
	if(old_prefs == NULL || old_prefs->weather_provider != new_prefs->weather_provider) {
		weather_request_update();
	}
	if(old_prefs != NULL && old_prefs->language_code != new_prefs->language_code) {
		// Update the current date (but not the time)
		force_tick(DAY_UNIT | MONTH_UNIT | YEAR_UNIT);
	}
	if(old_prefs == NULL || old_prefs->statusbar != new_prefs->statusbar) {
		GRect statusbar_frame = get_statusbar_frame(new_prefs);
		GRect time_frame = get_time_frame(new_prefs, true);
		GRect date_frame = get_date_frame(new_prefs, true);
		
		if(old_prefs == NULL) {
			layer_set_frame(statusbar_layer, statusbar_frame);
			layer_set_frame(text_layer_get_layer(time_layer), time_frame);
			layer_set_frame(text_layer_get_layer(date_layer), date_frame);
			
			set_weather_visible(!weather_needs_update(weather, new_prefs->weather_update_freq), false);
		}
		else {
			PropertyAnimation *statusbar_animation = property_animation_create_layer_frame(statusbar_layer, NULL, &statusbar_frame);
			PropertyAnimation *time_animation = property_animation_create_layer_frame(text_layer_get_layer(time_layer), NULL, &time_frame);
			PropertyAnimation *date_animation = property_animation_create_layer_frame(text_layer_get_layer(date_layer), NULL, &date_frame);
			
			animation_set_delay(&statusbar_animation->animation, 0);
			animation_set_delay(&time_animation->animation, 100);
			animation_set_delay(&date_animation->animation, 200);
			
			animation_set_duration(&statusbar_animation->animation, 250);
			animation_set_duration(&time_animation->animation, 250);
			animation_set_duration(&date_animation->animation, 250);
			
			animation_set_curve(&statusbar_animation->animation, new_prefs->statusbar ? AnimationCurveEaseIn : AnimationCurveEaseOut);
			
			AnimationHandlers animation_handlers = { .stopped = animation_stopped_handler };
			animation_set_handlers(&statusbar_animation->animation, animation_handlers, NULL);
			animation_set_handlers(&time_animation->animation, animation_handlers, NULL);
			animation_set_handlers(&date_animation->animation, animation_handlers, NULL);
			
			animation_schedule(&statusbar_animation->animation);
			animation_schedule(&time_animation->animation);
			animation_schedule(&date_animation->animation);
		}
	}
}

void set_weather_visible(bool visible, bool animate) {
	GRect time_frame = get_time_frame(prefs, visible);
	GRect date_frame = get_date_frame(prefs, visible);
	GRect weather_frame = get_weather_frame(visible);
	
	if(animate) {
		if(visible)
			layer_set_hidden(weather_layer, false);
		
		PropertyAnimation *time_animation = property_animation_create_layer_frame(text_layer_get_layer(time_layer), NULL, &time_frame);
		PropertyAnimation *date_animation = property_animation_create_layer_frame(text_layer_get_layer(date_layer), NULL, &date_frame);
		PropertyAnimation *weather_animation = property_animation_create_layer_frame(weather_layer, NULL, &weather_frame);
		
		animation_set_delay(&time_animation->animation, 300);
		animation_set_delay(&date_animation->animation, 150);
		animation_set_delay(&weather_animation->animation, 0);
		
		animation_set_duration(&time_animation->animation, 350);
		animation_set_duration(&date_animation->animation, 350);
		animation_set_duration(&weather_animation->animation, 500);
		
		animation_set_curve(&weather_animation->animation, visible ? AnimationCurveEaseOut : AnimationCurveEaseIn);
		
		AnimationHandlers animation_handlers = { .stopped = animation_stopped_handler };
		animation_set_handlers(&time_animation->animation, animation_handlers, NULL);
		animation_set_handlers(&date_animation->animation, animation_handlers, NULL);
		
		AnimationHandlers weather_animation_handlers = { .stopped = set_weather_visible_animation_stopped_handler };
		animation_set_handlers(&weather_animation->animation, weather_animation_handlers, visible ? (void*)1 : (void*)0);
		
		animation_schedule(&time_animation->animation);
		animation_schedule(&date_animation->animation);
		animation_schedule(&weather_animation->animation);
	}
	else {
		layer_set_frame(text_layer_get_layer(time_layer), time_frame);
		layer_set_frame(text_layer_get_layer(date_layer), date_frame);
		layer_set_frame(weather_layer, weather_frame);
		layer_set_hidden(weather_layer, !visible);
	}
}

void set_weather_visible_animation_stopped_handler(Animation *animation, bool finished, void *context) {
	if(finished)
		layer_set_hidden(weather_layer, context == 0);
	animation_stopped_handler(animation, finished, context);
}



void update_weather_info(Weather *weather, bool animate) {
    if(weather->conditions.code != WEATHER_CONDITIONS_UNAVAILABLE) {
        static char temperature_text[8];
		int temperature = weather_convert_temperature(weather->temperature, prefs->temp_format);
		
        snprintf(temperature_text, 8, "%d\u00B0", temperature);
        text_layer_set_text(weather_temperature_layer, temperature_text);
        
		// TODO: Move this block to another method
        if(10 <= temperature && temperature <= 99) {
            layer_set_frame(text_layer_get_layer(weather_temperature_layer), GRect(70, 19+3, 72, 80));
            text_layer_set_font(weather_temperature_layer, futura_35);
        }
        else if((0 <= temperature && temperature <= 9) || (-9 <= temperature && temperature <= -1)) {
            layer_set_frame(text_layer_get_layer(weather_temperature_layer), GRect(70, 19, 72, 80));
            text_layer_set_font(weather_temperature_layer, futura_40);
        }
        else if((100 <= temperature) || (-99 <= temperature && temperature <= -10)) {
            layer_set_frame(text_layer_get_layer(weather_temperature_layer), GRect(70, 19+3, 72, 80));
            text_layer_set_font(weather_temperature_layer, futura_28);
        }
        else {
            layer_set_frame(text_layer_get_layer(weather_temperature_layer), GRect(70, 19+6, 72, 80));
            text_layer_set_font(weather_temperature_layer, futura_25);
        }
        
        if(weather_icon_bitmap)
            gbitmap_destroy(weather_icon_bitmap);
        weather_icon_bitmap = gbitmap_create_with_resource(get_resource_for_weather_conditions(weather->conditions));
        bitmap_layer_set_bitmap(weather_icon_layer, weather_icon_bitmap);
		
		set_weather_visible(true, animate);
    }
}

void update_connection_info(bool bluetooth, bool internet) {
	uint32_t new_connection_resource = get_resource_for_connection(bluetooth, internet);
	
	if(!statusbar_connection_bitmap || new_connection_resource != statusbar_connection_resource) {
		statusbar_connection_resource = new_connection_resource;
		
		if(statusbar_connection_bitmap)
			gbitmap_destroy(statusbar_connection_bitmap);
		statusbar_connection_bitmap = gbitmap_create_with_resource(statusbar_connection_resource);
		bitmap_layer_set_bitmap(statusbar_connection_layer, statusbar_connection_bitmap);
		
		layer_mark_dirty(bitmap_layer_get_layer(statusbar_connection_layer));
		layer_mark_dirty(statusbar_layer);
	}
}



void animation_stopped_handler(Animation *animation, bool finished, void *context) {
	animation_destroy(animation);
}



void out_sent_handler(DictionaryIterator *sent, void *context) {
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Sending message failed (reason: %d)", (int)reason);
}

void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *set_weather = dict_find(received, SET_WEATHER_MSG_KEY);
	Tuple *set_preferences = dict_find(received, SET_PREFERENCES_MSG_KEY);
	
	if(set_weather) {
		weather_set(weather, received);
		update_weather_info(weather, true);
		
		// Receiving weather info means we (probably) have internet!
		update_connection_info(bluetooth_connection_service_peek(), has_internet_connection(weather));
	}
	
	if(set_preferences) {
		Preferences old_prefs = *prefs;
		
		preferences_set(prefs, received);
		change_preferences(&old_prefs, prefs);
		
		preferences_save(prefs);
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {	
    APP_LOG(APP_LOG_LEVEL_ERROR, "Received message dropped (reason: %d)", (int)reason);
}



// TODO: Move this somewhere else
unsigned long get_string_between_delimiters_at_index(char* buffer, size_t buffer_size, char* str, char delim, unsigned long index) {
	char* start = str;
	unsigned long str_length = strlen(str);
	char* end = str + str_length;
	
	memset(buffer, 0, buffer_size);
	
	while(index > 0 && start < end) {
		if(start[0] == delim)
			index--;
		start++;
	}
	
	unsigned long length = 0;
	while(start[length] != delim && start[length] != 0) {
		length++;
	}
	
	length = (length > str_length) ? str_length : length;           // Prevent reading beyond the string
	length++;                                                       // Make room for null character
	length = (length + 1 > buffer_size) ? buffer_size - 1 : length; // Prevent buffer overflow
	
	if(length > 1) {
		memcpy(buffer, start, length - 1);
		buffer[length] = 0; // Ensure that buffer ends with a null-terminating character
	}
	else {
		buffer[0] = 0;
		length = 0;
	}
	
	return length;
}

void format_time(char* buffer, size_t buffer_length, struct tm *now, bool is_24h) {
	strftime(buffer, buffer_length, is_24h ? "%H:%M" : "%I:%M", now);
}

void format_date(char* buffer, size_t buffer_length, struct tm *now, Preferences *prefs) {
	int month, day_of_month, day_of_week;
	char weekday[10], month_name[10];
	
	month = now->tm_mon;
	day_of_month = now->tm_mday;
	day_of_week = ((now->tm_wday + 6) % 7); // tm_wday defines 0 as Sunday, we want 0 as Monday
	
	get_string_between_delimiters_at_index(weekday, sizeof(weekday), prefs->translation, ',', day_of_week + 12);
	get_string_between_delimiters_at_index(month_name, sizeof(month_name), prefs->translation, ',', month);
	
	snprintf(buffer, buffer_length, "%s %s %d", weekday, month_name, day_of_month);
	
	if(strlen(buffer) <= 0) {
		APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to interpret translation %d (%s)", prefs->language_code, prefs->translation);
		strftime(buffer, buffer_length, "%a %b %d", now); // Fallback
	}
}



int main() {
    init();
    app_event_loop();
    deinit();
}

void init() {
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_set_fullscreen(window, true);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);
    
    const uint32_t inbound_size = app_message_inbox_size_maximum();
    const uint32_t outbound_size = app_message_outbox_size_maximum();
    app_message_open(inbound_size, outbound_size);
	
	prefs = preferences_load();
	weather = weather_load_cache();
	
    window_stack_push(window, true);
}

void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    futura_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_18));
    futura_25 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_25));
    futura_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_28));
    futura_35 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_35));
    futura_40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_40));
    futura_53 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_CONDENSED_53));
	
	
	
	statusbar_layer = layer_create(default_statusbar_frame = GRect(0, 0, 144, 15));
	
	statusbar_battery_layer = bitmap_layer_create(GRect(116, 3, 25, 11));
	layer_add_child(statusbar_layer, bitmap_layer_get_layer(statusbar_battery_layer));
	
	statusbar_connection_layer = bitmap_layer_create(GRect(3, 3, 24, 11));
	layer_add_child(statusbar_layer, bitmap_layer_get_layer(statusbar_connection_layer));
	
	layer_add_child(window_layer, statusbar_layer);
	
	
    
    time_layer = text_layer_create(default_time_frame = GRect(0, 2, 144, 162));
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_font(time_layer, futura_53);
    layer_add_child(window_layer, text_layer_get_layer(time_layer));
    
    date_layer = text_layer_create(default_date_frame = GRect(1, 74, 144, 106));
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_font(date_layer, futura_18);
    layer_add_child(window_layer, text_layer_get_layer(date_layer));
    
    
	
    weather_layer = layer_create(default_weather_frame = GRect(0, 90, 144, 80));
    
    weather_icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
    layer_add_child(weather_layer, bitmap_layer_get_layer(weather_icon_layer));
    
    weather_temperature_layer = text_layer_create(GRect(70, 19, 72, 80));
    text_layer_set_text_color(weather_temperature_layer, GColorWhite);
    text_layer_set_background_color(weather_temperature_layer, GColorClear);
    text_layer_set_font(weather_temperature_layer, futura_40);
    text_layer_set_text_alignment(weather_temperature_layer, GTextAlignmentRight);
    layer_add_child(weather_layer, text_layer_get_layer(weather_temperature_layer));
    
    layer_add_child(window_layer, weather_layer);
	
	
	
	change_preferences(NULL, prefs);
	
	
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	battery_state_service_subscribe(handle_battery);
	bluetooth_connection_service_subscribe(handle_bluetooth);
    
	// "Force" an update for each subscribed service (draws everything, since we're only drawing what we need)
    force_tick(ALL_UNITS);
	handle_battery(battery_state_service_peek());
	handle_bluetooth(bluetooth_connection_service_peek());
}

void window_unload(Window *window) {
    text_layer_destroy(time_layer);
    text_layer_destroy(date_layer);
	
    if(weather_icon_bitmap)
        gbitmap_destroy(weather_icon_bitmap);
    text_layer_destroy(weather_temperature_layer);
    bitmap_layer_destroy(weather_icon_layer);
    layer_destroy(weather_layer);
	
	if(statusbar_battery_bitmap)
		gbitmap_destroy(statusbar_battery_bitmap);
	if(statusbar_connection_bitmap)
		gbitmap_destroy(statusbar_connection_bitmap);
	bitmap_layer_destroy(statusbar_battery_layer);
	bitmap_layer_destroy(statusbar_connection_layer);
	layer_destroy(statusbar_layer);
    
    fonts_unload_custom_font(futura_18);
    fonts_unload_custom_font(futura_25);
    fonts_unload_custom_font(futura_28);
    fonts_unload_custom_font(futura_35);
    fonts_unload_custom_font(futura_40);
    fonts_unload_custom_font(futura_53);
}

void deinit() {
    window_destroy(window);
}



void handle_tick(struct tm *now, TimeUnits units_changed) {
    if(units_changed & MINUTE_UNIT) {
        static char time_text[6];
		format_time(time_text, sizeof(time_text), now, clock_is_24h_style());
		
        text_layer_set_text(time_layer, time_text);
    }
    
    if(units_changed & DAY_UNIT) {
        static char date_text[18]; // 18 characters should be enougth to fit the most common language formats
		format_date(date_text, sizeof(date_text), now, prefs);
		
        text_layer_set_text(date_layer, date_text);
    }
    
	bool outdated = weather_needs_update(weather, prefs->weather_outdated_time);
    if(outdated || weather_needs_update(weather, prefs->weather_update_freq)) {
        weather_request_update();
	}
	if(outdated) {
		set_weather_visible(false, true);
		update_connection_info(bluetooth_connection_service_peek(), has_internet_connection(weather));
	}
}

void handle_battery(BatteryChargeState battery) {
	uint32_t new_battery_resource = get_resource_for_battery_state(battery);
	if(!statusbar_battery_bitmap || new_battery_resource != statusbar_battery_resource) {
		statusbar_battery_resource = new_battery_resource;
		
		if(statusbar_battery_bitmap)
			gbitmap_destroy(statusbar_battery_bitmap);
		statusbar_battery_bitmap = gbitmap_create_with_resource(statusbar_battery_resource);
		bitmap_layer_set_bitmap(statusbar_battery_layer, statusbar_battery_bitmap);
		
		layer_mark_dirty(bitmap_layer_get_layer(statusbar_battery_layer));
		layer_mark_dirty(statusbar_layer);
	}
}

void handle_bluetooth(bool connected) {
	update_connection_info(connected, has_internet_connection(weather));
	
	if(!connected) {
		vibes_long_pulse();
	}
}





void force_tick(TimeUnits units_changed) {
    time_t then = time(NULL);
    struct tm *now = localtime(&then);
	
    handle_tick(now, units_changed);
}

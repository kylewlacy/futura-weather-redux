#include "pebble.h"
#include "config.h"
#include "utils.h"
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
static uint32_t statusbar_battery_resource;
static GBitmap *statusbar_battery_bitmap = NULL;
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



void change_preferences(Preferences *old_prefs, Preferences *new_prefs) {
	// old_prefs will be NULL for initialization (app first loaded)
	if(old_prefs == NULL || old_prefs->temp_format != new_prefs->temp_format) {
		if(!weather_needs_update(weather, new_prefs->weather_update_freq))
			update_weather_info(weather);
	}
	if(old_prefs == NULL || old_prefs->statusbar != new_prefs->statusbar) {
		GRect statusbar_frame = default_statusbar_frame;
		GRect time_frame = default_time_frame;
		GRect date_frame = default_date_frame;
		
		if(new_prefs->statusbar) {
			time_frame.origin.y += 8;
			date_frame.origin.y += 4;
		}
		else {
			statusbar_frame.origin.y -= statusbar_frame.size.h;
		}
		
		if(old_prefs == NULL) {
			layer_set_frame(statusbar_layer, statusbar_frame);
			layer_set_frame(text_layer_get_layer(time_layer), time_frame);
			layer_set_frame(text_layer_get_layer(date_layer), date_frame);
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
			
			animation_schedule(&statusbar_animation->animation);
			animation_schedule(&time_animation->animation);
			animation_schedule(&date_animation->animation);
		}
	}
}

void update_weather_info(Weather *weather) {
    if(weather->conditions % 1000) {
        static char temperature_text[8];
		int temperature = weather_convert_temperature(weather->temperature, prefs->temp_format);
		
        snprintf(temperature_text, 8, "%d\u00B0", temperature);
        text_layer_set_text(weather_temperature_layer, temperature_text);
        
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
    }
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
		update_weather_info(weather);
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
    
    const uint32_t inbound_size = 256;
    const uint32_t outbound_size = 256;
    app_message_open(inbound_size, outbound_size);
	
	prefs = preferences_load();
	weather = weather_load_cache();
	
	preferences_send(prefs);
    
    const bool animated = true;
    window_stack_push(window, animated);
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
	
	statusbar_battery_layer = bitmap_layer_create(GRect(125, 3, 16, 10));
	layer_add_child(statusbar_layer, bitmap_layer_get_layer(statusbar_battery_layer));
	
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
    
	// "Force" a tick with all units (draws everything, since we're only drawing what we need)
    force_tick(ALL_UNITS);
	
	// "Force" a battery update with the current battery status
	handle_battery(battery_state_service_peek());
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
	bitmap_layer_destroy(statusbar_battery_layer);
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
		strftime(time_text, 6, clock_is_24h_style() ? "%H:%M" : "%I:%M", now);
		
        text_layer_set_text(time_layer, time_text);
    }
    
    if(units_changed & DAY_UNIT) {
        static char date_text[11];
        strftime(date_text, 11, "%a %b %d",  now);
        text_layer_set_text(date_layer, date_text);
    }
    
	// TOOD: Tell don't ask
    if(weather_needs_update(weather, prefs->weather_update_freq))
        weather_request_update();
}

void handle_battery(BatteryChargeState battery) {
	uint32_t new_battery_resource = get_resource_for_battery_state(battery);
	if(!statusbar_battery_bitmap || new_battery_resource != statusbar_battery_resource) {
		statusbar_battery_resource = new_battery_resource;
		
		if(statusbar_battery_bitmap)
			gbitmap_destroy(statusbar_battery_bitmap);
		statusbar_battery_bitmap = gbitmap_create_with_resource(statusbar_battery_resource);
		bitmap_layer_set_bitmap(statusbar_battery_layer, statusbar_battery_bitmap);
		
		// TODO: Why is this needed to redraw the bitmap?
		force_tick(ALL_UNITS);
	}
}



void force_tick(TimeUnits units_changed) {
    time_t then = time(NULL);
    struct tm *now = localtime(&then);
	
    handle_tick(now, units_changed);
}
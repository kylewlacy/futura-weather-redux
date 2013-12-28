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

static TextLayer *time_layer;
static TextLayer *date_layer;

static Layer *weather_layer;

static TextLayer *temperature_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static Preferences *prefs;
static Weather *weather;

void update_weather_info(Weather *weather) {
    if(weather->conditions % 1000) {
        static char temperature_text[8];
		int temperature = weather_convert_temperature(weather->temperature, prefs->temp_format);
		
        snprintf(temperature_text, 8, "%d\u00B0", temperature);
        text_layer_set_text(temperature_layer, temperature_text);
        
        if(10 <= temperature && temperature <= 99) {
            layer_set_frame(text_layer_get_layer(temperature_layer), GRect(70, 19+3, 72, 80));
            text_layer_set_font(temperature_layer, futura_35);
        }
        else if((0 <= temperature && temperature <= 9) || (-9 <= temperature && temperature <= -1)) {
            layer_set_frame(text_layer_get_layer(temperature_layer), GRect(70, 19, 72, 80));
            text_layer_set_font(temperature_layer, futura_40);
        }
        else if((100 <= temperature) || (-99 <= temperature && temperature <= -10)) {
            layer_set_frame(text_layer_get_layer(temperature_layer), GRect(70, 19+3, 72, 80));
            text_layer_set_font(temperature_layer, futura_28);
        }
        else {
            layer_set_frame(text_layer_get_layer(temperature_layer), GRect(70, 19+6, 72, 80));
            text_layer_set_font(temperature_layer, futura_25);
        }
        
        if(icon_bitmap)
            gbitmap_destroy(icon_bitmap);
        icon_bitmap = gbitmap_create_with_resource(get_resource_for_weather_conditions(weather->conditions));
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
    }
}

uint32_t get_resource_for_weather_conditions(uint32_t conditions) {
	bool is_day = conditions >= 1000;
    switch((conditions - (conditions % 100)) % 1000) {
        case 0:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Error getting data (conditions returned %d)", (int)conditions);
            return RESOURCE_ID_ICON_CLOUD_ERROR;
        case 200:
            return RESOURCE_ID_ICON_THUNDER;
        case 300:
            return RESOURCE_ID_ICON_DRIZZLE;
        case 500:
            return RESOURCE_ID_ICON_RAIN;
        case 600:
            switch(conditions % 100) {
                case 611:
                    return RESOURCE_ID_ICON_SLEET;
                case 612:
                    return RESOURCE_ID_ICON_RAIN_SLEET;
                case 615:
                case 616:
                case 620:
                case 621:
                case 622:
                    return RESOURCE_ID_ICON_RAIN_SNOW;
            }
            return RESOURCE_ID_ICON_SNOW;
        case 700:
            switch(conditions % 100) {
                case 731:
                case 781:
                    return RESOURCE_ID_ICON_WIND;
            }
            return RESOURCE_ID_ICON_FOG;
        case 800:
            switch(conditions % 100) {
                case 0:
					if(is_day)
						return RESOURCE_ID_ICON_CLEAR_DAY;
					return RESOURCE_ID_ICON_CLEAR_NIGHT;
                case 1:
                case 2:
					if(is_day)
						return RESOURCE_ID_ICON_PARTLY_CLOUDY_DAY;
					return RESOURCE_ID_ICON_PARTLY_CLOUDY_NIGHT;
                case 3:
                case 4:
                    return RESOURCE_ID_ICON_CLOUDY;
            }
        case 900:
            switch(conditions % 100) {
                case 0:
                case 1:
                case 2:
                    return RESOURCE_ID_ICON_WIND;
                case 3:
                    return RESOURCE_ID_ICON_HOT;
                case 4:
                    return RESOURCE_ID_ICON_COLD;
                case 5:
                    return RESOURCE_ID_ICON_WIND;
                case 950:
                case 951:
                case 952:
                case 953:
					if(is_day)
						return RESOURCE_ID_ICON_CLEAR_DAY;
					return RESOURCE_ID_ICON_CLEAR_NIGHT;
                case 954:
                case 955:
                case 956:
                case 957:
                case 959:
                case 960:
                case 961:
                case 962:
                    return RESOURCE_ID_ICON_WIND;
            }
    }
    
    APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown wearther conditions: %d", (int)conditions);
    return RESOURCE_ID_ICON_CLOUD_ERROR;
}



void out_sent_handler(DictionaryIterator *sent, void *context) {
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Sending message failed (reason: %d)", (int)reason);
}

void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *set_weather = dict_find(received, SET_WEATHER_KEY);
	Tuple *set_preferences = dict_find(received, SET_PREFERENCES_KEY);
	
	if(set_weather) {
		if(weather_set(weather, received))
			update_weather_info(weather);
		else
			APP_LOG(APP_LOG_LEVEL_WARNING, "Received weather message without weather");
	}
	
	if(set_preferences) {
		if(preferences_set(prefs, received))
			update_weather_info(weather);
		else
			APP_LOG(APP_LOG_LEVEL_WARNING, "Received preference message without preferences");
		
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
    
    const uint32_t inbound_size = 64;
    const uint32_t outbound_size = 64;
    app_message_open(inbound_size, outbound_size);
	
	prefs = preferences_load();
	weather = weather_load_cache();
	
	preferences_send(prefs);
    
    const bool animated = true;
    window_stack_push(window, animated);
}

void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    futura_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
    futura_25 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_25));
    futura_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_28));
    futura_35 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35));
    futura_40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40));
    futura_53 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));
    
    time_layer = text_layer_create(GRect(0, 2, 144, 168-6));
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_font(time_layer, futura_53);
    layer_add_child(window_layer, text_layer_get_layer(time_layer));
    
    date_layer = text_layer_create(GRect(1, 74, 144, 168-62));
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_font(date_layer, futura_18);
    layer_add_child(window_layer, text_layer_get_layer(date_layer));
    
    
    weather_layer = layer_create(GRect(0, 90, 144, 80));
    
    icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
    layer_add_child(weather_layer, bitmap_layer_get_layer(icon_layer));
    
    temperature_layer = text_layer_create(GRect(70, 19, 72, 80));
    text_layer_set_text_color(temperature_layer, GColorWhite);
    text_layer_set_background_color(temperature_layer, GColorClear);
    text_layer_set_font(temperature_layer, futura_40);
    text_layer_set_text_alignment(temperature_layer, GTextAlignmentRight);
    layer_add_child(weather_layer, text_layer_get_layer(temperature_layer));
    
    layer_add_child(window_layer, weather_layer);
	
	// Draw weather info if the cache is recent enough
	if(!weather_needs_update(weather, prefs->weather_update_frequency))
		update_weather_info(weather);
    
	
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
    
    time_t then = time(NULL);
    struct tm *now = localtime(&then);
    handle_tick(now, SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT);
}

void window_unload(Window *window) {
    if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
    }
    
    text_layer_destroy(time_layer);
    text_layer_destroy(date_layer);
    
    text_layer_destroy(temperature_layer);
    bitmap_layer_destroy(icon_layer);
    layer_destroy(weather_layer);
    
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
    if(weather_needs_update(weather, prefs->weather_update_frequency))
        weather_request_update();
}

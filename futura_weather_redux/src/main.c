#include "pebble.h"

static Window *window;

#define TIME_FRAME      (GRect(0, 0, 144, 90))
#define DATE_FRAME      (GRect(0, 60, 144, 30))

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

static time_t last_weather_update = 0;
static time_t weather_update_frequency = 10*60;

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
static TempFormat temp_format = TEMP_FORMAT_CELCIUS;


bool need_weather_update() {
    time_t now = time(NULL);
	
    return last_weather_update && (now - last_weather_update >= weather_update_frequency);
}

static uint32_t get_resource_for_conditions(uint32_t conditions) {
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

static void request_weather_update() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    Tuplet request = TupletInteger(REQUEST_WEATHER_KEY, 1);
    dict_write_tuplet(iter, &request);
    
    app_message_outbox_send();
}



static void handle_tick(struct tm *now, TimeUnits units_changed) {
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
    
    if(need_weather_update()) {
		APP_LOG(APP_LOG_LEVEL_INFO, "[%01d:%01d:%01d] Updating weather", now->tm_hour, now->tm_min, now->tm_sec);
        request_weather_update();
	}
}



static void load_preferences() {
	if(persist_exists(TEMP_PREFERENCE_KEY))
		temp_format = persist_read_int(TEMP_PREFERENCE_KEY);
	if(persist_exists(WEATHER_UPDATE_PREFERENCE_KEY))
		weather_update_frequency = persist_read_int(WEATHER_UPDATE_PREFERENCE_KEY);
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Weather update frequency is %d", (int)weather_update_frequency);
}

static void save_preferences() {
	status_t save_temp = persist_write_int(TEMP_PREFERENCE_KEY, temp_format);
	status_t save_weather_update = persist_write_int(WEATHER_UPDATE_PREFERENCE_KEY, (int)weather_update_frequency);
	
	
	// TODO: Retry saving if failed
	if(save_temp < 0)
		APP_LOG(APP_LOG_LEVEL_ERROR, "Saving temperature preference returned %d", (int)save_temp);
	if(save_weather_update < 0 )
		APP_LOG(APP_LOG_LEVEL_ERROR, "Saving weather update preference returned %d", (int)save_weather_update);
	
}

static void send_preferences() {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	Tuplet request = TupletInteger(SET_PREFERENCES_KEY, 1);
	dict_write_tuplet(iter, &request);
	
	Tuplet temp = TupletInteger(TEMP_PREFERENCE_KEY, temp_format);
	dict_write_tuplet(iter, &temp);
	
	app_message_outbox_send();
}

static void update_weather_info(int conditions, int temperature) {
    if(conditions % 1000) {
        static char temperature_text[8];
        snprintf(temperature_text, 8, "%d\u00B0", temperature);
        text_layer_set_text(temperature_layer, temperature_text);
        
        if(10 <= temperature && temperature <= 99) {
            layer_set_frame(text_layer_get_layer(temperature_layer), GRect(70, 19+3, 72, 80));
            text_layer_set_font(temperature_layer, futura_35);
        }
        else if((0 <= temperature && temperature <= 9) || (-9 <= temperature && temperature <= -1)){
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
        icon_bitmap = gbitmap_create_with_resource(get_resource_for_conditions(conditions));
        bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
        
        last_weather_update = time(NULL);
    }
}



void out_sent_handler(DictionaryIterator *sent, void *context) {
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Sending message failed (reason: %d)", (int)reason);
}

void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *set_weather = dict_find(received, SET_WEATHER_KEY);
	Tuple *request_preferences = dict_find(received, REQUEST_PREFERENCES_KEY);
	Tuple *set_preferences = dict_find(received, SET_PREFERENCES_KEY);
	
	if(set_weather) {
		Tuple *conditions = dict_find(received, WEATHER_CONDITIONS_KEY);
		Tuple *temperature = dict_find(received, WEATHER_TEMPERATURE_KEY);
		
		if(conditions && temperature)
			update_weather_info((int)conditions->value->int32, (int)temperature->value->int32);
		else
			APP_LOG(APP_LOG_LEVEL_WARNING, "No temperature or weather conditions found in weather response");
	}
	
	if(request_preferences) {
		send_preferences();
	}
	
	if(set_preferences) {
		Tuple *temp_preference = dict_find(received, TEMP_PREFERENCE_KEY);
		Tuple *weather_update_preference = dict_find(received, WEATHER_UPDATE_PREFERENCE_KEY);
		
		if(temp_preference && (temp_preference->value->int32 != temp_format)) {
			temp_format = temp_preference->value->int32;
			request_weather_update();
		}
		if(weather_update_preference && (weather_update_preference->value->int32 != (int)weather_update_frequency)) {
			weather_update_frequency = weather_update_preference->value->int32;
		}
		
		save_preferences();
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Received message dropped (reason: %d)", (int)reason);
}




static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    futura_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
    futura_25 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_25));
    futura_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_28));
    futura_35 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35));
    futura_40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40));
    futura_53 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));
    
    time_layer = text_layer_create(TIME_FRAME);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_background_color(time_layer, GColorWhite);
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_font(time_layer, futura_53);
    layer_add_child(window_layer, text_layer_get_layer(time_layer));
    
    date_layer = text_layer_create(DATE_FRAME);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_background_color(date_layer, GColorWhite);
    text_layer_set_text_color(date_layer, GColorBlack);
    text_layer_set_font(date_layer, futura_18);
    layer_add_child(window_layer, text_layer_get_layer(date_layer));
    
    
    weather_layer = layer_create(GRect(0, 90, 144, 80));
    
    icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
    layer_add_child(weather_layer, bitmap_layer_get_layer(icon_layer));
    
    temperature_layer = text_layer_create(GRect(70, 19, 72, 80));
    text_layer_set_text_color(temperature_layer, GColorWhite);
    text_layer_set_background_color(temperature_layer, GColorBlack);
    text_layer_set_font(temperature_layer, futura_40);
    text_layer_set_text_alignment(temperature_layer, GTextAlignmentRight);
    layer_add_child(weather_layer, text_layer_get_layer(temperature_layer));
    
    layer_add_child(window_layer, weather_layer);
    
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
    
    time_t then = time(NULL);
    struct tm *now = localtime(&then);
    handle_tick(now, SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT);
}

static void window_unload(Window *window) {
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

static void init(void) {
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
	
	load_preferences();
    
    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

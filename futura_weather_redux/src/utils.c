#include "utils.h"

uint32_t get_resource_for_weather_conditions(uint32_t conditions) {
	bool is_day = conditions >= 1000;
    switch((conditions - (conditions % 100)) % 1000) {
        case 0:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Error getting data (conditions returned %d)", (int)conditions);
            return RESOURCE_ID_ICON_CLOUD_ERROR;
        case 200:
            return RESOURCE_ID_WEATHER_THUNDER;
        case 300:
            return RESOURCE_ID_WEATHER_DRIZZLE;
        case 500:
            return RESOURCE_ID_WEATHER_RAIN;
        case 600:
            switch(conditions % 100) {
                case 611:
                    return RESOURCE_ID_WEATHER_SLEET;
                case 612:
                    return RESOURCE_ID_WEATHER_RAIN_SLEET;
                case 615:
                case 616:
                case 620:
                case 621:
                case 622:
                    return RESOURCE_ID_WEATHER_RAIN_SNOW;
            }
            return RESOURCE_ID_WEATHER_SNOW;
        case 700:
            switch(conditions % 100) {
                case 731:
                case 781:
                    return RESOURCE_ID_WEATHER_WIND;
            }
            return RESOURCE_ID_WEATHER_FOG;
        case 800:
            switch(conditions % 100) {
                case 0:
					if(is_day)
						return RESOURCE_ID_WEATHER_CLEAR_DAY;
					return RESOURCE_ID_WEATHER_CLEAR_NIGHT;
                case 1:
                case 2:
					if(is_day)
						return RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY;
					return RESOURCE_ID_WEATHER_PARTLY_CLOUDY_NIGHT;
                case 3:
                case 4:
                    return RESOURCE_ID_WEATHER_CLOUDY;
            }
        case 900:
            switch(conditions % 100) {
                case 0:
                case 1:
                case 2:
                    return RESOURCE_ID_WEATHER_WIND;
                case 3:
                    return RESOURCE_ID_WEATHER_HOT;
                case 4:
                    return RESOURCE_ID_WEATHER_COLD;
                case 5:
                    return RESOURCE_ID_WEATHER_WIND;
                case 950:
                case 951:
                case 952:
                case 953:
					if(is_day)
						return RESOURCE_ID_WEATHER_CLEAR_DAY;
					return RESOURCE_ID_WEATHER_CLEAR_NIGHT;
                case 954:
                case 955:
                case 956:
                case 957:
                case 959:
                case 960:
                case 961:
                case 962:
                    return RESOURCE_ID_WEATHER_WIND;
            }
    }
    
    APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown wearther conditions: %d", (int)conditions);
    return RESOURCE_ID_ICON_CLOUD_ERROR;
}

uint32_t get_resource_for_battery_state(BatteryChargeState battery) {
	if((battery.is_charging || battery.is_plugged) && battery.charge_percent > 99)
		return RESOURCE_ID_CHARGED;
	
	if(battery.charge_percent >= 99)
		return battery.is_charging ? RESOURCE_ID_CHARGING_100 : RESOURCE_ID_BATTERY_100;
	if(battery.charge_percent >= 91)
		return battery.is_charging ? RESOURCE_ID_CHARGING_91 : RESOURCE_ID_BATTERY_91;
	if(battery.charge_percent >= 82)
		return battery.is_charging ? RESOURCE_ID_CHARGING_82 : RESOURCE_ID_BATTERY_82;
	if(battery.charge_percent >= 73)
		return battery.is_charging ? RESOURCE_ID_CHARGING_73 : RESOURCE_ID_BATTERY_73;
	if(battery.charge_percent >= 64)
		return battery.is_charging ? RESOURCE_ID_CHARGING_64 : RESOURCE_ID_BATTERY_64;
	if(battery.charge_percent >= 55)
		return battery.is_charging ? RESOURCE_ID_CHARGING_55 : RESOURCE_ID_BATTERY_55;
	if(battery.charge_percent >= 46)
		return battery.is_charging ? RESOURCE_ID_CHARGING_46 : RESOURCE_ID_BATTERY_46;
	if(battery.charge_percent >= 37)
		return battery.is_charging ? RESOURCE_ID_CHARGING_37 : RESOURCE_ID_BATTERY_37;
	if(battery.charge_percent >= 28)
		return battery.is_charging ? RESOURCE_ID_CHARGING_28 : RESOURCE_ID_BATTERY_28;
	if(battery.charge_percent >= 19)
		return battery.is_charging ? RESOURCE_ID_CHARGING_19 : RESOURCE_ID_BATTERY_19;
	if(battery.charge_percent >= 9)
		return battery.is_charging ? RESOURCE_ID_CHARGING_9 : RESOURCE_ID_BATTERY_9;
	
	return battery.is_charging ? RESOURCE_ID_CHARGING_0 : RESOURCE_ID_BATTERY_0;
}

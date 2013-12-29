var config_url = "http://kylewlacy.github.io/futura-weather-redux/v1/preferences.html";

var prefs = {
	"temp_format": 1,
	"weather_update_frequency": 10 * 60
};

var weather = {
	"temperature": -461,
	"conditions": 0,
	"last_update": 0
};

var max_weather_update_frequency = 10 * 60;

function fetchWeather(latitude, longitude) {
	if(Date.now() - weather.last_update > max_weather_update_frequency) {
		var response;
		var req = new XMLHttpRequest();
		req.open('GET', "http://api.openweathermap.org/data/2.5/find?" +
				 "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if(req.status == 200) {
					response = JSON.parse(req.responseText);
					if (response && response.list && response.list.length > 0) {
						var weatherResult = response.list[0];
						weather.temperature = weatherResult.main.temp;
						weather.conditions = weatherResult.weather[0].id;
						weather.last_update = Date.now();
						
						sendWeather(weather = {
							"temperature": weatherResult.main.temp,
							"conditions": weatherResult.weather[0].id,
							"last_update": Date.now()
						});
					}
				} else {
					console.log("Error getting weather info");
				}
			}
		}
		req.send(null);
	}
	else {
		console.warn("Weather update requested too soon; loading from cache");
		sendWeather(weather);
	}
}

function sendWeather(weather) {
	Pebble.sendAppMessage({
		"setWeather": 1,
		"temperature": Math.round(weather.temperature * 100),		// Pebble SDK only lets us send ints (easily), so we multiply the temperature by 100 to maintain significant digits
		"conditions": weather.conditions
	});
}

function sendPreferences(prefs) {
	Pebble.sendAppMessage({
		"setPreferences": 1,
		"tempPreference": prefs.temp_format,
		"weatherUpdatePreference": prefs.weather_update_frequency
	});
}


Pebble.addEventListener("ready", function(e) {
});

Pebble.addEventListener("appmessage", function(e) {
	if(e.payload["setPreferences"] == 1) {
		prefs = {
			"temp_format": e.payload["tempPreference"],
			"weather_update_freqeuncy": e.payload["weatherUpdatePreference"]
		};
	}
	else if(e.payload["requestWeather"] == 1) {
		window.navigator.geolocation.getCurrentPosition(function(pos) {
			fetchWeather(pos.coords.latitude, pos.coords.longitude);
		}, function(error) {
			console.warn("location error (" + err.code + "): " + err.message);
		}, { "timeout": 15000, "maximumAge": 60000 });
	}
	else {
		console.warn("Received unknown app message");
	}
});

Pebble.addEventListener("showConfiguration", function() {
	Pebble.openURL(config_url + "?temp=" + prefs.temp_format + "&weather-update=" + prefs.weather_update_frequency);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if(e && e.response) {
		var new_prefs = JSON.parse(e.response);
		
		if(new_prefs["temp"])
			prefs.temp_format = parseInt(new_prefs.temp);
		if(prefs["weather-update"])
			prefs.weather_update_frequency = Math.max(max_weather_update_frequency, parseInt(new_prefs.weather-update));
		sendPreferences();
	}
});

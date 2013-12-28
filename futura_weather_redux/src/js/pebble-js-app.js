var config_url = "http://kylewlacy.github.io/futura-weather-redux/v1/preferences.html";
var temp_format = 1;
var weather_update_frequency = 10 * 60;

var temperature = -1;
var conditions = 0;
var is_day = false;

var last_weather_update = 0;
var max_weather_update_frequency = 10 * 60;

function fetchWeather(latitude, longitude) {
	if(Date.now() - last_weather_update > max_weather_update_frequency) {
		var response;
		var req = new XMLHttpRequest();
		req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
				 "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if(req.status == 200) {
					response = JSON.parse(req.responseText);
					if (response && response.list && response.list.length > 0) {
						var weatherResult = response.list[0];
						
						// The weather icon will end with either 'd' or 'n',
						// depending on the time
						// TODO: Actually calculate the time of the sunset or something
						is_day = weatherResult.weather[0].icon[3] != 'n';
						
						temperature = weatherResult.main.temp;
						conditions = weatherResult.weather[0].id;
						
						sendWeather(is_day, temperature, conditions);
						
						last_weather_update = Date.now();
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
		sendWeather(is_day, temperature, conditions);
	}
}

function sendWeather() {
	Pebble.sendAppMessage({
		"setWeather": 1,
		"temperature": Math.round(temperature * 100),		// Pebble SDK only lets us send ints (easily), so we multiply the temperature by 100 to maintain significant digits
		"conditions": conditions + (is_day ? 1000 : 0)
	});
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    console.warn("location error (" + err.code + "): " + err.message);
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function setPreferences() {
	Pebble.sendAppMessage({
		"setPreferences": 1,
		"tempPreference": temp_format,
		"weatherUpdatePreference": weather_update_frequency
	});
}


Pebble.addEventListener("ready", function(e) {
});

Pebble.addEventListener("appmessage", function(e) {
	if(e.payload["setPreferences"] == 1) {
		temp_format = e.payload["tempPreference"];
		weather_update_frequency = e.payload["weatherUpdatePreference"];
	}
	else if(e.payload["requestWeather"] == 1) {
		window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
	}
	else {
		console.warn("Received unknown app message");
	}
});

Pebble.addEventListener("showConfiguration", function() {
	Pebble.openURL(config_url + "?temp=" + temp_format + "&weather-update=" + weather_update_frequency);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if(e && e.response) {
		var prefs = JSON.parse(e.response);
		
		if(prefs["temp"])
			temp_format = parseInt(prefs["temp"]);
		if(prefs["weather-update"])
			weather_update_frequency = Math.max(max_weather_update_frequency, parseInt(prefs["weather-update"]));
		setPreferences();
	}
});



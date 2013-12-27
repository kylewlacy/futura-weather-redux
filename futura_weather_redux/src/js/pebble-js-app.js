function fetchWeather(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
             "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                response = JSON.parse(req.responseText);
                var temperature, icon, is_day;
                if (response && response.list && response.list.length > 0) {
                    var weatherResult = response.list[0];
                    
					// The weather icon will end with either 'd' or 'n',
					// depending on the time
					is_day = weatherResult.weather[0].icon[3] != 'n';
					
					temperature = Math.round(weatherResult.main.temp - 273.15);
                    conditions = weatherResult.weather[0].id + (is_day ? 1000 : 0);
                    
                    Pebble.sendAppMessage({
						"conditions": conditions,
						"temperature": temperature,
					});
                }
                
            } else {
                console.log("Error getting weather info");
            }
        }
    }
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    console.warn("location error (" + err.code + "): " + err.message);
    Pebble.sendAppMessage({
		"temperature": -274,
		"conditions": 0
	});
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };


Pebble.addEventListener("ready", function(e) {
	locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
});

Pebble.addEventListener("appmessage", function(e) {
	window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
});

//Pebble.addEventListener("webviewclosed", function(e) {
//	console.log("webview closed");
//	console.log(e.type);
//	console.log(e.response);
//});



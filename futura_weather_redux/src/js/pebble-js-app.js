function Preferences() {
	this.tempFormat = 1;
	this.weatherUpdateFreq = 10 * 60;
	this.statusbar = 0;
	this.weatherProvider = 1;
	this.weatherOutdatedTime = 60 * 60;
	this.languageCode = 0;
	this.translation = "Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,Mon,Tue,Wed,Thu,Fri,Sat,Sun";
	
	this.setFromJsonObject = function(newPrefs) {
		for(var key in getProperties(this)) {
			if(typeof(newPrefs[key]) !== "undefined") {
				if(key === "translation") {
					this[key] = newPrefs[key].join(",");
					localStorage.setItem(key, this[key]);
				}
				else {
					localStorage.setItem(key, newPrefs[key]);
					this[key] = parseInt(newPrefs[key]);
				}
			}
		}
	}
	
	this.loadFromStorage = function() {
		for(var key in getProperties(this)) {
			var value = localStorage.getItem(key);
			if (value != null) {
				if(key === "translation") {
					this[key] = localStorage.getItem(key);
				}
				else {
					this[key] = parseInt(localStorage.getItem(key));
				}
			}
		}
	}

	this.asMessage = function() {
		var prefs = copyObject(getProperties(this));
		prefs.translation = prepareString(prefs.translation);
		
		return mergeObjects(prefs, {"setPrefs": 1});
	}
	
	this.asQueryString = function() {
		// Send everything but the acutual translation
		var queryPrefs = copyObject(getProperties(this));
		delete queryPrefs.translation;
		
		return queryify(queryPrefs);
	}
}



function LocationHandler() {
	this.coords = {};
	
	var getCurrentLocationCallback = function(pos, callback) {
		this.coords = pos.coords;
		if(typeof(callback) === "function") { callback(this); }
	}
	
	var getCurrentLocationErrorCallback = function(err) {
		console.warn("location error (" + err.code + "): " + err.message);
	}
	
	this.getCurrentLocation = function(callback) {
		window.navigator.geolocation.getCurrentPosition(
			function(pos) {
				getCurrentLocationCallback(pos, callback);
			},
			getCurrentLocationErrorCallback,
			{ "timeout": 15000, "maximumAge": 60000 }
		);
	}
}



function Weather(temperature, conditions, lastUpdate) {
	this.temperature = valueOrDefault(temperature, -461);
	this.conditions = valueOrDefault(conditions, 0);
	this.lastUpdate = valueOrDefault(lastUpdate, 0);
	
	this.setFromJsonObject = function(newWeather, updatedAt) {
		updatedAt = valueOrDefault(updatedAt, new Date());
		
		if(typeof(newWeather) !== "undefined") {
			for(var key in getProperties(this)) {
				if(typeof(newWeather[key]) !== "undefined") {
					this[key] = newWeather[key];
				}
			}
		}
		
		this.lastUpdate = Math.round(updatedAt.getTime() / 1000);
	}
	
	this.asMessage = function() {
		var now = new Date();
		var sunCalc = SunCalc.getTimes(now, coords.latitude, coords.longitude);
		var isDay = sunCalc.sunset > now && now > sunCalc.sunrise;
		
		return {
			"temperature": Math.round(weather.temperature * 100),
			"weatherConditionCode": weather.conditions,
			"weatherConditionFlags": isDay ? 1 : 0, // TODO: Better flag handling
			"setWeather": 1
		};
	}
}

function YrNoWeatherProvider() {
	var convertConditionCode = function(code) {
		switch(parseInt(code)) {
			case 6:         //LIGHTRAINTHUNDERSUN
			case 11:        //RAINTHUNDER
            case 14:        //SNOWTHUNDER
            case 20:        //SLEETSUNTHUNDER
            case 21:        //SNOWSUNTHUNDER
			case 22:        //LIGHTRAINTHUNDER
            case 23:        //SLEETTHUNDER
				return 12;  // -> Thunder icon
			case 5:         //LIGHTRAINSUN
			case 9:         //LIGHTRAIN
				return 5;   // -> Drizzle icon
			case 10:        //RAIN
            case 18:        //LIGHTRAINSUN
				return 6;   // -> Rain icon
			case 8:         //SNOWSUN
            case 13:        //SNOW
			case 19:        //SNOWSUN (used for winter darkness)
				return 11;  // -> Snow icon
			case 7:         //SLEETSUN
			case 12:        //SLEET
				return 9;   // -> Sleet icon
			case 15:        //FOG
				return 4;   // -> Fog
			case 1:         //SUN
			case 16:        //SUN (used for winter darkness)
				return 1;   // -> Clear icon
			case 2:         //LIGHTCLOUD
			case 3:         //PARTLYCLOUD
			case 17:        //LIGHTCLOUD (winter darkness)
				return 2;   // -> Partly cloudy icon
			case 4:         //CLOUD
				return 3;   // -> Cloudy icon
		}
		
		console.warn("Unknown yr.no weather code: " + code);
		return 0;
	}
	
	var updateWeatherCallback = function(req, coords) {
		if(req.status == 200) {
            xmlStr = req.responseText;
            if (xmlStr) {
                var newStr=xmlStr.replace(/(\r\n|\n|\r)/gm,"");
                var t_arr = /<temperature.*?value\="(.*?)"/.exec(newStr);
                var s_arr = /<symbol.*?number\="(.*?)"/.exec(newStr);
                if (t_arr != null && s_arr != null) {
                    var weatherCode = s_arr[1];
                    var temperature = t_arr[1];
                    var tempKelvin = 273.15 + Number(temperature);
                    var code = convertConditionCode(weatherCode);

                    return {
                        "temperature": tempKelvin,
                            "conditions": code
                    };
                }
			}
		}
		else {
			console.warn("Error getting weather with OpenWeatherMap (status " + req.status + ")");
		}
	}
	
	this.updateWeather = function(weather, coords, callback) {
        var url = "http://api.yr.no/weatherapi/locationforecastlts/1.1/?lat=" + coords.latitude + ";lon=" + coords.longitude;
		makeRequest(
			"GET", url,
			function(req) {
				weather.setFromJsonObject(updateWeatherCallback(req, coords));
				if(typeof(callback) === "function") { callback(weather); }
			}
		);	
	}
}


function OpenWeatherMapProvider() {
	var convertConditionCode = function(code) {
		switch(parseInt(code)) {
			case 200:      // Thunderstorm with light rain
			case 201:      // Thunderstorm with rain
			case 202:      // Thunderstorm with heavy rain
			case 210:      // Light thunderstorm
			case 211:      // Thunderstorm
			case 212:      // Heavy thunderstorm
			case 221:      // Ragged thunderstorm
			case 230:      // Thunderstorm with light drizzle
			case 231:      // Thunderstorm with drizzle
			case 232:      // Thunderstorm with heavy drizzle
			case 900:      // Tornado
			case 901:      // Tropical storm
			case 902:      // Hurricane
			case 960:      // Storm
			case 961:      // Violent storm
			case 962:      // Hurricane (again)
				return 12; // -> Thunder icon
			case 300:      // Light intensity drizzle
			case 301:      // Drizzle
			case 310:      // Light intensity drizzle rain
			case 311:      // Drizzle rain
			case 312:      // Heavy intensity drizzle rain
			case 313:      // Shower rain and drizzle
			case 314:      // Heavy shower rain and drizzle
			case 321:      // Shower drizzle
				return 5;  // -> Drizzle icon
			case 500:      // Light rain
			case 501:      // Moderate rain
			case 502:      // Heavy intensity rain
			case 503:      // Very heavy rain
			case 504:      // Extreme rain
			case 511:      // Freezing rain
			case 520:      // Light intensity shower rain
			case 521:      // Shower rain
			case 522:      // Heavy intensity shower rain
			case 531:      // Ragged shower rain
				return 6;  // -> Rain icon
			case 600:      // Light snow
			case 601:      // Snow
			case 602:      // Heavy snow
				return 11; // -> Snow icon
			case 611:      // Sleet
			case 906:      // Hail
				return 9;  // -> Sleet icon
			case 612:      // Shower sleet
				return 7;  // -> Rain and sleet icon
			case 615:      // Light rain and snow
			case 616:      // Rain and snow
			case 620:      // Light shower snow
			case 621:      // Shower snow
			case 622:      // Heavy shower snow
				return 8;  // -> Rain and snow icon
			case 701:      // Mist
			case 711:      // Smoke
			case 721:      // Haze
			case 731:      // Sand/dust whirls
			case 741:      // Fog
			case 751:      // Sand
			case 761:      // Dust
			case 762:      // Volcanic ash
				return 4;  // -> Fog
			case 771:      // Squalls
			case 905:      // Windy
			case 954:      // Moderate breeze
			case 955:      // Fresh breeze
			case 956:      // Strong breeze
			case 957:      // High wind, near gale
			case 958:      // Gale
			case 959:      // Severe gale
				return 13; // -> Wind icon
			case 800:      // Sky is clear
			case 950:      // Setting
			case 951:      // Calm
			case 952:      // Light breeze
			case 953:      // Gentle breeze
				return 1;  // -> Clear icon
			case 801:      // Few clouds
			case 802:      // Scattered clouds
			case 803:      // Broken clouds
				return 2;  // -> Partly cloudy icon
			case 804:      // Overcast clouds
				return 3;  // -> Cloudy icon
			case 903:      // Cold
				return 15; // -> Cold icon
			case 904:      // Hot
				return 14; // -> Hot icon
		}
		
		console.warn("Unknown OpenWeatherMap weather code: " + code);
		return 0;
	}
	
	var updateWeatherCallback = function(req, coords) {
		if(req.status == 200) {
			var response = JSON.parse(req.responseText);
			
			if (response && response.list && response.list.length > 0) {
				var result = response.list[0];
				var temperature = result.main.temp;
				var code = convertConditionCode(result.weather[0].id);
				
				return {
					"temperature": temperature,
					"conditions": code
				};
			}
		}
		else {
			console.warn("Error getting weather with OpenWeatherMap (status " + req.status + ")");
		}
	}
	
	this.updateWeather = function(weather, coords, callback) {
		makeRequest(
			"GET",
			"http://api.openweathermap.org/data/2.5/find?lat=" +
			coords.latitude + "&lon=" + coords.longitude + "&cnt=1",
			function(req) {
				weather.setFromJsonObject(updateWeatherCallback(req, coords));
				if(typeof(callback) === "function") { callback(weather); }
			}
		);
	}
}

function YahooWeatherProvider() {
	var convertConditionCode = function(code) {
		switch(parseInt(code)) {
			case 0:        // Tornado
			case 1:        // Tropical storm
			case 2:        // Hurricane
			case 3:        // Severe thunderstomrs
			case 4:        // Thunderstorms
			case 37:       // Isolated thunderstorms
			case 38:       // Scattered thunderstorms
			case 39:       // Scattered thunderstorms (again)
			case 45:       // Thundershowers
			case 47:       // Isolated thundershowers
				return 12; // -> Thunder icon
			case 5:        // Mixed rain and snow
				return 8;  // -> Rain and snow icon
			case 6:        // Mixed rain and sleet
			case 35:       // Mixed rain and hail
				return 7;  // -> Rain and sleet icon
			case 7:        // Mixed snow and sleet
				return 10; // -> Snow and sleet icon
			case 8:        // Freezing drizzle
			case 9:        // Drizzle
				return 5;  // -> Drizzle icon
			case 10:       // Freezing rain
			case 11:       // Showers
			case 12:       // Showers (again)
			case 40:       // Scattered showers
				return 6;  // -> Rain icon
			case 13:       // Snow flurries
			case 14:       // Light snow showers
			case 15:       // Blowing snow
			case 16:       // Snow
			case 41:       // Heavy snow
			case 42:       // Scattered snow showers
			case 43:       // Heavy snow (again)
			case 46:       // Snow showers
				return 11; // -> Snow icon
			case 17:       // Hail
			case 18:       // Sleet
				return 9;  // -> Sleet icon
			case 19:       // Dust
			case 20:       // Foggy
			case 21:       // Haze
			case 22:       // Smoky
				return 4;  // -> Fog icon
			case 23:       // Blustery
			case 24:       // Windy
				return 13; // -> Wind icon
			case 25:       // Cold
				return 15; // -> Cold icon
			case 26:       // Cloudy
			case 27:       // Mostly cloudy (night)
			case 28:       // Mostly cloudy (day)
				return 3;  // -> Cloudy icon
			case 29:       // Partly cloudy (night)
			case 30:       // Partly cloudy (day)
			case 44:       // Partly cloudy
				return 2;  // -> Partly cloudy icon
			case 31:       // Clear (night)
			case 32:       // Sunny
			case 33:       // Fair (night)
			case 34:       // Fair (day)
				return 1;  // -> Clear icon
			case 36:       // Hot
				return 14; // -> Hot icon
		}
		
		console.warn("Unknown Yahoo Weather weather code: " + code);
		return 0;
	}
	
	var updateWeatherWithWoeidCallback = function(req, coords) {
		if (req.status == 200) {
			var response = JSON.parse(req.responseText);
			var conditions = response.query.results.channel.item.condition;
			
			//Convert to Kelvin
			var temperature = Number(conditions.temp) + 273.15;
			var code = convertConditionCode(conditions.code);
			
			return {
				"temperature": temperature,
				"conditions": code
			};
		}
		else {
		  console.warn("Error getting weather with Yahoo Weather (status " + req.status + ")");
		}
	}
	
	var updateWeatherWithWoeid = function(req, weather, coords, callback) {
		var response = JSON.parse(req.responseText);
		var woeid = response.query.results.Result.woeid;
		
		var query = encodeURI(
			"select item.condition from weather.forecast where woeid = "
			+ woeid + ' and u = "c"'
		);
		var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
		
		makeRequest("GET", url, function(req) {
			weather.setFromJsonObject(updateWeatherWithWoeidCallback(req, coords));
			if(typeof(callback) === "function") { callback(weather); }
		});
	}
	
	var woeidCallback = function(req, weather, coords, callback) {
		if(req.status == 200) {
			return updateWeatherWithWoeid(req, weather, coords, callback);
		}
		else {
			console.warn("Error getting Yahoo Weather WOEID (status " + req.status + ")");
		}
	}
	
	this.updateWeather = function(weather, coords, callback) {
		var query = encodeURI(
			'select woeid from geo.placefinder where text=\"'
			+ coords.latitude + "," + coords.longitude
			+ '" and gflags="R"'
		);
		var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
		
		makeRequest("GET", url, function(req) {
			woeidCallback(req, weather, coords, callback)
		});
	}
}



function getProperties(obj) {
	var properties = {};
	for(var property in obj) {
		if(obj.hasOwnProperty(property) && typeof(obj[property]) !== "function") {
			properties[property] = obj[property];
		}
	}

	return properties;
}

function makeRequest(method, url, callback) {
	var req = new XMLHttpRequest();
	req.open(method, url, true);
	req.onload = function(e) {
		if(req.readyState == 4) { callback(req); }
	}
	req.send(null);
}

function valueOrDefault(value, defaultValue) {
	return typeof(value) !== "undefined" ? value : defaultValue;
}

// Encodes string as byte array. Somehow,
// this actually makes UTF-8 work properly
// (even though JS uses UTF-16 internally).
function prepareString(string) {
	var bytes = [];
	for(var i = 0; i < string.length; i++) {
		bytes.push(string[i].charCodeAt(0));
	}

	bytes.push(0); // Null-terminate
	return bytes;
}

function mergeObjects(a, b) {
	for(var key in b) { a[key] = b[key]; }
	return a;
}

function copyObject(obj) {
	var new_obj = {};
	for(var key in obj) { new_obj[key] = obj[key]; }
	return new_obj;
}

function queryify(obj) {
	var queries = [];
	for(var key in obj) { queries.push(key + "=" + obj[key]) };
	return "?" + queries.join("&");
}



var configURL = "http://kylewlacy.github.io/futura-weather-redux/v3-yrno/preferences.html";

var prefs = new Preferences();
prefs.loadFromStorage();
var weather = new Weather();
var loc = new LocationHandler();

var providers = {
	"1": new OpenWeatherMapProvider(),
	"2": new YahooWeatherProvider(),
    "3": new YrNoWeatherProvider(),
	"default": new OpenWeatherMapProvider()
};

var maxWeatherUpdateFreq = 10 * 60;



function fetchWeather() {
    if(Math.round(Date.now()/1000) - weather.lastUpdate >= maxWeatherUpdateFreq) {
	    loc.getCurrentLocation(fetchWeatherFromLocation);
    }
    else {
        console.warn("Weather update requested too soon; loading from cache (" + (new Date()).toString() + ")");
        sendWeather(weather);
    }
}

function fetchWeatherFromLocation(loc) {
	var provider = valueOrDefault(
		providers[prefs.weatherProvider.toString()],
		providers["default"]
	);
	provider.updateWeather(weather, loc.coords, sendWeather);
}



function sendWeather(weather) {
	Pebble.sendAppMessage(weather.asMessage());
}

function sendPreferences(prefs) {
	Pebble.sendAppMessage(prefs.asMessage());
}



Pebble.addEventListener("ready", function(e) {
    fetchWeather();
});

Pebble.addEventListener("appmessage", function(e) {
	if(e.payload["requestWeather"] == 1) {
        fetchWeather();
	}
	else {
		console.warn("Received unknown app message:");
		for(var key in e.payload) { console.log("  " + key + ": " + e.payload[key]); }
	}
});

Pebble.addEventListener("showConfiguration", function() {	
	Pebble.openURL(configURL + prefs.asQueryString());
});

Pebble.addEventListener("webviewclosed", function(e) {
	if(e && e.response) {
		// Clear lastUpdate when updating preferences
		// so any provider updates will take effect instantly.
		// TODO: Only clear when the provider has actually changed
		weather.lastUpdate = 0;
		
		var newPrefs = JSON.parse(e.response);
		prefs.setFromJsonObject(newPrefs);
		
		sendPreferences(prefs);
	}
});






/*
 (c) 2011-2013, Vladimir Agafonkin
 SunCalc is a JavaScript library for calculating sun/mooon position and light phases.
 https://github.com/mourner/suncalc
*/

(function () { "use strict";
 
 // shortcuts for easier to read formulas
 
 var PI		= Math.PI,
 sin	= Math.sin,
 cos	= Math.cos,
 tan	= Math.tan,
 asin = Math.asin,
 atan = Math.atan2,
 acos = Math.acos,
 rad	= PI / 180;
 
 // sun calculations are based on http://aa.quae.nl/en/reken/zonpositie.html formulas
 
 
 // date/time constants and conversions
 
 var dayMs = 1000 * 60 * 60 * 24,
 J1970 = 2440588,
 J2000 = 2451545;
 
 function toJulian(date) {
 return date.valueOf() / dayMs - 0.5 + J1970;
 }
 function fromJulian(j) {
 return new Date((j + 0.5 - J1970) * dayMs);
 }
 function toDays(date) {
 return toJulian(date) - J2000;
 }
 
 
 // general calculations for position
 
 var e = rad * 23.4397; // obliquity of the Earth
 
 function getRightAscension(l, b) {
 return atan(sin(l) * cos(e) - tan(b) * sin(e), cos(l));
 }
 function getDeclination(l, b) {
 return asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
 }
 function getAzimuth(H, phi, dec) {
 return atan(sin(H), cos(H) * sin(phi) - tan(dec) * cos(phi));
 }
 function getAltitude(H, phi, dec) {
 return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
 }
 function getSiderealTime(d, lw) {
 return rad * (280.16 + 360.9856235 * d) - lw;
 }
 
 
 // general sun calculations
 
 function getSolarMeanAnomaly(d) {
 return rad * (357.5291 + 0.98560028 * d);
 }
 function getEquationOfCenter(M) {
 return rad * (1.9148 * sin(M) + 0.02 * sin(2 * M) + 0.0003 * sin(3 * M));
 }
 function getEclipticLongitude(M, C) {
 var P = rad * 102.9372; // perihelion of the Earth
 return M + C + P + PI;
 }
 function getSunCoords(d) {
 
 var M = getSolarMeanAnomaly(d),
 C = getEquationOfCenter(M),
 L = getEclipticLongitude(M, C);
 
 return {
 dec: getDeclination(L, 0),
 ra: getRightAscension(L, 0)
 };
 }
 
 
 var SunCalc = {};
 
 
 // calculates sun position for a given date and latitude/longitude
 
 SunCalc.getPosition = function (date, lat, lng) {
 
 var lw	 = rad * -lng,
 phi = rad * lat,
 d	 = toDays(date),
 
 c	= getSunCoords(d),
 H	= getSiderealTime(d, lw) - c.ra;
 
 return {
 azimuth: getAzimuth(H, phi, c.dec),
 altitude: getAltitude(H, phi, c.dec)
 };
 };
 
 
 // sun times configuration (angle, morning name, evening name)
 
 var times = [
				[-0.83, 'sunrise',			 'sunset'			 ],
				[ -0.3, 'sunriseEnd',		 'sunsetStart' ],
				[		-6, 'dawn',					 'dusk'				 ],
				[	 -12, 'nauticalDawn',	 'nauticalDusk'],
				[	 -18, 'nightEnd',			 'night'			 ],
				[		 6, 'goldenHourEnd', 'goldenHour'	 ]
				];
 
 // adds a custom time to the times config
 
 SunCalc.addTime = function (angle, riseName, setName) {
 times.push([angle, riseName, setName]);
 };
 
 
 // calculations for sun times
 
 var J0 = 0.0009;
 
 function getJulianCycle(d, lw) {
 return Math.round(d - J0 - lw / (2 * PI));
 }
 function getApproxTransit(Ht, lw, n) {
 return J0 + (Ht + lw) / (2 * PI) + n;
 }
 function getSolarTransitJ(ds, M, L) {
 return J2000 + ds + 0.0053 * sin(M) - 0.0069 * sin(2 * L);
 }
 function getHourAngle(h, phi, d) {
 return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d)));
 }
 
 
 // calculates sun times for a given date and latitude/longitude
 
 SunCalc.getTimes = function (date, lat, lng) {
 
 var lw	 = rad * -lng,
 phi = rad * lat,
 d	 = toDays(date),
 
 n	= getJulianCycle(d, lw),
 ds = getApproxTransit(0, lw, n),
 
 M = getSolarMeanAnomaly(ds),
 C = getEquationOfCenter(M),
 L = getEclipticLongitude(M, C),
 
 dec = getDeclination(L, 0),
 
 Jnoon = getSolarTransitJ(ds, M, L);
 
 
 // returns set time for the given sun altitude
 function getSetJ(h) {
 var w = getHourAngle(h, phi, dec),
 a = getApproxTransit(w, lw, n);
 
 return getSolarTransitJ(a, M, L);
 }
 
 
 var result = {
 solarNoon: fromJulian(Jnoon),
 nadir: fromJulian(Jnoon - 0.5)
 };
 
 var i, len, time, angle, morningName, eveningName, Jset, Jrise;
 
 for (i = 0, len = times.length; i < len; i += 1) {
 time = times[i];
 
 Jset = getSetJ(time[0] * rad);
 Jrise = Jnoon - (Jset - Jnoon);
 
 result[time[1]] = fromJulian(Jrise);
 result[time[2]] = fromJulian(Jset);
 }
 
 return result;
 };
 
 
 // moon calculations, based on http://aa.quae.nl/en/reken/hemelpositie.html formulas
 
 function getMoonCoords(d) { // geocentric ecliptic coordinates of the moon
 
 var L = rad * (218.316 + 13.176396 * d), // ecliptic longitude
 M = rad * (134.963 + 13.064993 * d), // mean anomaly
 F = rad * (93.272 + 13.229350 * d),	// mean distance
 
 l	= L + rad * 6.289 * sin(M), // longitude
 b	= rad * 5.128 * sin(F),			// latitude
 dt = 385001 - 20905 * cos(M);	// distance to the moon in km
 
 return {
 ra: getRightAscension(l, b),
 dec: getDeclination(l, b),
 dist: dt
 };
 }
 
 SunCalc.getMoonPosition = function (date, lat, lng) {
 
 var lw	 = rad * -lng,
 phi = rad * lat,
 d	 = toDays(date),
 
 c = getMoonCoords(d),
 H = getSiderealTime(d, lw) - c.ra,
 h = getAltitude(H, phi, c.dec);
 
 // altitude correction for refraction
 h = h + rad * 0.017 / tan(h + rad * 10.26 / (h + rad * 5.10));
 
 return {
 azimuth: getAzimuth(H, phi, c.dec),
 altitude: h,
 distance: c.dist
 };
 };
 
 
 // calculations for illuminated fraction of the moon,
 // based on http://idlastro.gsfc.nasa.gov/ftp/pro/astro/mphase.pro formulas
 
 SunCalc.getMoonFraction = function (date) {
 
 var d = toDays(date),
 s = getSunCoords(d),
 m = getMoonCoords(d),
 
 sdist = 149598000, // distance from Earth to Sun in km
 
 phi = acos(sin(s.dec) * sin(m.dec) + cos(s.dec) * cos(m.dec) * cos(s.ra - m.ra)),
 inc = atan(sdist * sin(phi), m.dist - sdist * cos(phi));
 
 return (1 + cos(inc)) / 2;
 };
 
 
 // export as AMD module / Node module / browser variable
 
 if (typeof define === 'function' && define.amd) {
 define(SunCalc);
 } else if (typeof module !== 'undefined') {
 module.exports = SunCalc;
 } else {
 window.SunCalc = SunCalc;
 }
 
 }());

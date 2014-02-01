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
				}
				else {
					this[key] = parseInt(newPrefs[key]);
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
	
	this.asMessage = function(isDay) {
		return {
			"temperature": Math.round(weather.temperature * 100),
			"conditions": weather.conditions + (weather.isDay ? 1000 : 0),
			"setWeather": 1
		};
	}
}

function OpenWeatherMapProvider() {
	var updateWeatherCallback = function(req, coords) {
		if(req.status == 200) {
			var response = JSON.parse(req.responseText);
			
			if (response && response.list && response.list.length > 0) {
				var weatherResult = response.list[0];
				
				return {
					"temperature": weatherResult.main.temp,
					"conditions": weatherResult.weather[0].id
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
		var conditionMap = {
		  0  : 900, // Tornado
		  1  : 901, // Tropical storm
		  2  : 902, // Hurricane
		  3  : 212, // Severe thunderstorms
		  4  : 211, // Thunderstorms
		  5  : 511, // Mixed rain and snow
		  6  : 611, // Mixed rain and sleet
		  7  : 611, // Mixed snow and sleet
		  8  : 511, // Freezing drizzle
		  9  : 301, // Drizzle
		  10 : 511, // Freezing rain
		  11 : 521, // Showers
		  12 : 521, // Showers
		  13 : 601, // Snow flurries
		  14 : 621, // Light snow showers
		  15 : 602, // Blowing snow
		  16 : 602, // Snow
		  17 : 906, // Hail
		  18 : 611, // Sleet
		  19 : 731, // Dust
		  20 : 741, // Foggy
		  21 : 721, // Haze
		  22 : 711, // Smoky
		  23 : 905, // Blustery
		  24 : 905, // Windy
		  25 : 903, // Cold
		  26 : 802, // Cloudy
		  27 : 804, // Mostly cloudy (night)
		  28 : 804, // Mostly cloudy (day)
		  29 : 801, // Partly cloudy (night)
		  30 : 801, // Partly cloudy (day)
		  31 : 800, // Clear (night)
		  32 : 800, // Sunny
		  33 : 800, // Fair (night)
		  34 : 800, // Fair (day)
		  35 : 906, // Mixed rain and hail
		  36 : 904, // Hot
		  37 : 210, // Isolated thunderstorms
		  38 : 211, // Scattered thunderstorms
		  39 : 211, // Scattered thunderstorms
		  40 : 521, // Scattered showers
		  41 : 602, // Heavy snow
		  42 : 621, // Scattered snow showers
		  43 : 602, // Heavy snow
		  44 : 801, // Partly cloudy
		  45 : 201, // Thundershowers
		  46 : 621, // Snow showers
		  47 : 210, // Isolated thundershowers
		  3200 : 0, // Not available
		};
		return conditionMap[code];
	}
	
	var updateWeatherWithWoeidCallback = function(req, coords) {
		if (req.status == 200) {
			var response = JSON.parse(req.responseText);
			var conditions = response.query.results.channel.item.condition.code;
			
			//Convert to Kelvin
			var temp = Number(conditions.temp) + 273.15;
			var now = new Date();
			var sunCalc = SunCalc.getTimes(now, coords.latitude, coords.longitude);
			
			return {
				"temperature": temp,
				"conditions": convertConditionCode(conditions)
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



var configURL = "http://kylewlacy.github.io/futura-weather-redux/v3/preferences.html";

var prefs = new Preferences();
var weather = new Weather();
var location = new LocationHandler();

var providers = {
	"1": new OpenWeatherMapProvider(),
	"2": new YahooWeatherProvider(),
	"default": new OpenWeatherMapProvider()
};

var maxWeatherUpdateFreq = 10 * 60;



function fetchWeather() {
    if(Math.round(Date.now()/1000) - weather.lastUpdate >= maxWeatherUpdateFreq) {
	    location.getCurrentLocation(fetchWeatherFromLocation);
    }
    else {
        console.warn("Weather update requested too soon; loading from cache (" + (new Date()).toString() + ")");
        sendWeather(weather);
    }
}

function fetchWeatherFromLocation(location) {
	var provider = valueOrDefault(
		providers[prefs.weatherProvider.toString()],
		providers["default"]
	);
	provider.updateWeather(weather, location.coords, sendWeather);
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
	if(e.payload["setPrefs"] == 1) {
		prefs.setFromJsonObject(e.payload)
	}
	else if(e.payload["requestWeather"] == 1) {
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

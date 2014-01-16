var configURL = "http://kylewlacy.github.io/futura-weather-redux/v3/preferences.html";

var prefs = {
	"tempFormat": 1,
	"weatherUpdateFreq": 10 * 60,
	"statusbar": 0
};

var weather = {
	"temperature": -461,
	"conditions": 0,
	"lastUpdate": 0
};
var prevMessages = {};
var coords;

var maxWeatherUpdateFreq = 10 * 60;

var yahooConditionToOpenWeatherMapCondition = {
  0 : 900, //tornado
  1 : 901, //tropical storm
  2 : 902, //hurricane
  3 : 212, //severe thunderstorms
  4 : 211, //thunderstorms
  5 : 511, //mixed rain and snow
  6 : 611, //mixed rain and sleet
  7 : 611, //mixed snow and sleet
  8 : 511, //freezing drizzle
  9 : 301, //drizzle
  10 : 511, //freezing rain
  11 : 521, //showers
  12 : 521, //showers
  13 : 601, //snow flurries
  14 : 621, //light snow showers
  15 : 602, //blowing snow
  16 : 602, //snow
  17 : 906, //hail
  18 : 611, //sleet
  19 : 731, //dust
  20 : 741, //foggy
  21 : 721, //haze
  22 : 711, //smoky
  23 : 905, //blustery
  24 : 905, //windy
  25 : 903, //cold
  26 : 802, //cloudy
  27 : 804, //mostly cloudy (night)
  28 : 804, //mostly cloudy (day)
  29 : 801, //partly cloudy (night)
  30 : 801, //partly cloudy (day)
  31 : 800, //clear (night)
  32 : 800, //sunny
  33 : 800, //fair (night)
  34 : 800, //fair (day)
  35 : 906, //mixed rain and hail
  36 : 904, //hot
  37 : 210, //isolated thunderstorms
  38 : 211, //scattered thunderstorms
  39 : 211, //scattered thunderstorms
  40 : 521, //scattered showers
  41 : 602, //heavy snow
  42 : 621, //scattered snow showers
  43 : 602, //heavy snow
  44 : 801, //partly cloudy
  45 : 201, //thundershowers
  46 : 621, //snow showers
  47 : 210, //isolated thundershowers
  3200 : 0, //not available
};

function fetchWeatherYahoo() {
    window.navigator.geolocation.getCurrentPosition(
            function(pos) { coords = pos.coords; },
            function(err) { console.warn("location error (" + err.code + "): " + err.message); },
            { "timeout": 15000, "maximumAge": 60000 }
            );
		
    var woeid = -1;
    var query = encodeURI("select woeid from geo.placefinder where text=\""+coords.latitude+","+coords.longitude + "\" and gflags=\"R\"");
    var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";

    
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
                response = JSON.parse(req.responseText);
                if (response) {
                    woeid = response.woeid;
                    woeid = response.query.results.Result.woeid;
                    getWeatherYahooByWoeid(woeid);
                }
            } else {
                console.log("Error");
            }
        }
    }
    req.send(null);
}

function getWeatherYahooByWoeid(woeid) {
  //console.log("getWeatherYahooByWoeid " + woeid);
  var celsius = 1;
  var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
                        " and u = \"c\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";

  var response;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          var condition = response.query.results.channel.item.condition;
          //Convert to kelvin
          var temp = Number(condition.temp) + 273.15;
          //console.log("temp " + condition.temp);
          //console.log("adapted temp " + temp);
          //console.log("condition " + condition.text);
          var now = new Date();
          var sunCalc = SunCalc.getTimes(now, coords.latitude, coords.longitude);
          sendWeather(weather = {
              "temperature": temp,
              "conditions": yahooConditionToOpenWeatherMapCondition[condition.code],
              "isDay": sunCalc.sunset > now && now > sunCalc.sunrise,
              "lastUpdate": Math.round(now.getTime() / 1000)
          });
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}



function fetchWeatherOpenWeatherMap() {
		window.navigator.geolocation.getCurrentPosition(
			function(pos) { coords = pos.coords; },
			function(err) { console.warn("location error (" + err.code + "): " + err.message); },
			{ "timeout": 15000, "maximumAge": 60000 }
		);
		
		
		var response;
		var req = new XMLHttpRequest();
		req.open('GET', "http://api.openweathermap.org/data/2.5/find?" +
				 "lat=" + coords.latitude + "&lon=" + coords.longitude + "&cnt=1", true);
		req.onload = function(e) {
			if (req.readyState == 4) {
				if(req.status == 200) {
					response = JSON.parse(req.responseText);
					if (response && response.list && response.list.length > 0) {
						var weatherResult = response.list[0];
						
						var now = new Date();
						var sunCalc = SunCalc.getTimes(now, coords.latitude, coords.longitude);
						
						sendWeather(weather = {
							"temperature": weatherResult.main.temp,
							"conditions": weatherResult.weather[0].id,
							"isDay": sunCalc.sunset > now && now > sunCalc.sunrise,
							"lastUpdate": Math.round(now.getTime() / 1000)
						});
					}
				}
				else {
					console.log("Error getting weather info (status " + req.status + ")");
				}
			}
		}
		req.send(null);
}

function sendWeather(weather) {
	Pebble.sendAppMessage(mergeObjects({
		"temperature": Math.round(weather.temperature * 100),
		"conditions": weather.conditions + (weather.isDay ? 1000 : 0)
	}, {"setWeather": 1}));
}

function sendPreferences(prefs) {
	Pebble.sendAppMessage(mergeObjects(prefs, {"setPrefs": 1}));
}


function mergeObjects(a, b) {
	for(var key in b)
		a[key] = b[key];
	return a;
}

function queryify(obj) {
	var queries = [];
	for(var key in obj) { queries.push(key + "=" + obj[key]) };
	return "?" + queries.join("&");
}


Pebble.addEventListener("ready", function(e) { prevMessages = {}; });

Pebble.addEventListener("appmessage", function(e) {
	if(e.payload["setPrefs"] == 1) {
		for(var key in prefs)
			if(e.payload[key] !== "undefined") { prefs[key] = e.payload[key]; }
	}
	else if(e.payload["requestWeather"] == 1) {
        if(Math.round(Date.now()/1000) - weather.lastUpdate >= maxWeatherUpdateFreq) {
            //TODO: Check pref and call the correct weather service
            //fetchWeatherOpenWeatherMap();
            fetchWeatherYahoo();
        }
        else {
            console.warn("Weather update requested too soon; loading from cache (" + (new Date()).toString() + ")");
            sendWeather(weather);
        }
	}
	else {
		console.warn("Received unknown app message:");
		for(var key in e.payload)
			console.log("	 " + key + ": " + e.payload[key]);
	}
});

Pebble.addEventListener("showConfiguration", function() {
	Pebble.openURL(configURL + queryify(prefs));
});

Pebble.addEventListener("webviewclosed", function(e) {
	if(e && e.response) {
		var newPrefs = JSON.parse(e.response);
		for(var key in prefs) {
			if(newPrefs[key] !== "undefined")
				prefs[key] = parseInt(newPrefs[key]);
		}
		
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

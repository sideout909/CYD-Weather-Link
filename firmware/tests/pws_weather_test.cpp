#include "pws_weather.h"
#include <cassert>
#include <limits>
#include <iostream>
struct Observation {
 uint64_t epoch_ms=1792281600000ULL;bool live=true,fresh=true;
 bool temperature_valid=true,humidity_valid=true,wind_valid=true,direction_valid=true,gust_valid=false,rain_hour_valid=false,rain_today_valid=false,uv_valid=false;
 double temperature_f=32,humidity=0,wind_mph=0,direction_deg=0,gust_mph=0,rain_hour_in=0,rain_today_in=0,uv=0;
};
int main(){
 Observation o;auto q=pws_weather::query(o,"TEST_STATION","fixture&key",o.epoch_ms);
 assert(q.find("PASSWORD=fixture%26key")!=std::string::npos);
 assert(q.find("dateutc=2026-10-18%2000%3A00%3A00")!=std::string::npos);
 assert(q.find("tempf=32.0")!=std::string::npos&&q.find("windspeedmph=0.0")!=std::string::npos);
 assert(q.find("humidity=0")!=std::string::npos&&q.find("rainin=")==std::string::npos);
 o.rain_hour_valid=true;o.rain_hour_in=.12;
 assert(pws_weather::query(o,"TEST","fixture",o.epoch_ms).find("rainin=0.12")!=std::string::npos);
 o.live=false;assert(pws_weather::query(o,"TEST","fixture",o.epoch_ms).empty());o.live=true;
 o.fresh=false;assert(pws_weather::query(o,"TEST","fixture",o.epoch_ms).empty());o.fresh=true;
 assert(pws_weather::query(o,"TEST","fixture",o.epoch_ms+120000).empty());
 assert(pws_weather::query(o,"TEST","fixture",o.epoch_ms-1).empty());
 assert(pws_weather::query(o,"bad&ID","fixture",o.epoch_ms).empty());
 o.temperature_f=std::numeric_limits<double>::quiet_NaN();q=pws_weather::query(o,"TEST","fixture",o.epoch_ms);
 assert(q.find("tempf=")==std::string::npos);
 assert(pws_weather::accepted(" success\n"));assert(pws_weather::accepted("Data Logged and posted in METAR mirror."));
 assert(!pws_weather::accepted("ERROR invalid password"));assert(!pws_weather::accepted("<html>success</html>"));
 std::cout<<"PWSWeather checks passed: raw imperial fields, missing/zero, escaping, freshness/replay and exact acknowledgments.\n";
}

#include <cassert>
#include <iostream>
#include <limits>
#include "windy_weather.h"
struct Observation {
  uint64_t epoch_ms=1700000000000ULL;
  bool live=true,fresh=true;
  bool temperature_valid=false,humidity_valid=false,wind_valid=false;
  bool gust_valid=false,direction_valid=false,rain_hour_valid=false,uv_valid=false;
  double temperature_f=0,humidity=0,wind_mph=0,gust_mph=0,direction_deg=0,rain_hour_in=0,uv=0;
};
int main(){
  Observation o;auto query=[&](){return windy_weather::query(o,"test-001",1700000000000ULL);};
  assert(query().empty());
  o.temperature_valid=o.humidity_valid=o.wind_valid=o.gust_valid=true;
  o.direction_valid=o.rain_hour_valid=o.uv_valid=true;
  o.temperature_f=68;o.humidity=100;o.wind_mph=10;o.gust_mph=20;
  o.direction_deg=360;o.rain_hour_in=1;o.uv=0;
  auto q=query();
  for(const char* field:{"id=test-001","&ts=1700000000","&temp=20.0","&humidity=100",
      "&wind=4.47","&gust=8.94","&winddir=360","&precip=25.40","&uv=0.0"})
    assert(q.find(field)!=std::string::npos);
  assert(q.find("PASSWORD")==std::string::npos&&q.find("dailyrainin")==std::string::npos);
  assert(q.find("pressure")==std::string::npos&&q.find("solarradiation")==std::string::npos);
  o.temperature_f=std::numeric_limits<double>::quiet_NaN();o.uv=71;o.rain_hour_valid=false;
  q=query();assert(q.find("&temp=")==std::string::npos&&q.find("&uv=")==std::string::npos);
  assert(q.find("&precip=")==std::string::npos);
  o.live=false;assert(query().empty());o.live=true;o.fresh=false;assert(query().empty());o.fresh=true;
  assert(windy_weather::query(o,"test",o.epoch_ms+120000).empty());
  assert(!windy_weather::query(o,"test",o.epoch_ms+119999).empty());
  assert(windy_weather::query(o,"test",o.epoch_ms-1).empty());
  assert(windy_weather::query(o,"bad&id",o.epoch_ms).empty());
  assert(windy_weather::query(o,std::string(31,'a'),o.epoch_ms).empty());
  assert(windy_weather::accepted(200));
  for(int status:{201,204,301,400,401,409,429,500,-1})assert(!windy_weather::accepted(status));
  assert(windy_weather::interval_ms==300000);
  assert(windy_weather::validPassword("test-token"));
  for(const char* bad:{"","bad key","bad\r\nInjected:yes"})assert(!windy_weather::validPassword(bad));
  windy_weather::Schedule schedule;assert(schedule.due(100));
  schedule.result(100,true,1700000000);assert(!schedule.due(300099));assert(schedule.due(300100));
  schedule.result(300100,false,1700000000);assert(schedule.backoff==600000);
  for(int i=0;i<10;++i)schedule.result(0,false,1700000000);
  assert(schedule.backoff==3600000);
  schedule.result(0,true,1700000000,"2099-01-01T00:00:00Z");assert(schedule.next==300000);
  schedule.result(0,true,1700000000,"2023-11-14T23:13:20Z");assert(schedule.next==3600000);
  schedule.result(UINT32_MAX-100,true,1700000000);assert(!schedule.due(0));assert(schedule.due(300000));
  assert(std::string(windy_weather::resultMessage(200))=="Accepted for processing");
  std::cout<<"Windy v2 adapter checks passed: metric fields, omissions, freshness, identity and HTTP outcomes.\n";
}

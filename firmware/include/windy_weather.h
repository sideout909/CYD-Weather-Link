#pragma once
#include "pws_weather.h"
#include "nws_logic.h"
#include <algorithm>
namespace windy_weather {
inline constexpr const char* endpoint="https://stations.windy.com/api/v2/observation/update";
inline constexpr uint32_t interval_ms=300000;
// Password travels in Authorization: Bearer, never in this query string.
// Windy's v2 schema accepts these metric fields; no inferred solar/pressure.
template<class Observation>
std::string query(const Observation& o,const std::string& id,uint64_t now_ms){
  if(id.empty()||id.size()>30||!o.live||!o.fresh||!o.epoch_ms||
      now_ms<o.epoch_ms||now_ms-o.epoch_ms>=120000)return {};
  for(unsigned char c:id)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
      (c>='0'&&c<='9')||c=='-'||c=='_'))return {};
  std::string q="id="+pws_weather::encoded(id);
  pws_weather::add(q,"ts",std::to_string(o.epoch_ms/1000));
  pws_weather::add(q,"softwaretype","WX-Link");
  bool fields=false;
  auto field=[&](const char* name,bool valid,double value,unsigned digits,double low,double high){
    if(valid&&std::isfinite(value)&&value>=low&&value<=high){
      pws_weather::add(q,name,pws_weather::number(value,digits));fields=true;
    }
  };
  field("temp",o.temperature_valid,(o.temperature_f-32.0)/1.8,1,-100,100);
  field("humidity",o.humidity_valid,o.humidity,0,0,100);
  field("wind",o.wind_valid,o.wind_mph*0.44704,2,0,200);
  field("gust",o.gust_valid,o.gust_mph*0.44704,2,0,300);
  field("winddir",o.direction_valid,o.direction_deg,0,0,360);
  field("precip",o.rain_hour_valid,o.rain_hour_in*25.4,2,0,2500);
  field("uv",o.uv_valid,o.uv,1,0,70);
  return fields?q:std::string{};
}
// Monotonic scheduling survives millis() wrap. Absolute retry timestamps are
// accepted only within 24 hours; corrupt provider values cannot disable forever.
struct Schedule {
  uint32_t next=0,backoff=interval_ms;
  bool scheduled=false;
  bool due(uint32_t now)const{return !scheduled||int32_t(now-next)>=0;}
  void result(uint32_t now,bool success,int64_t epoch,const char* retry_after=nullptr){
    backoff=success?interval_ms:std::min(uint32_t(3600000),backoff*2);
    uint32_t delay=backoff;
    const int64_t retry=nws::isoTime(retry_after);
    if(retry>epoch&&retry-epoch<=86400)delay=std::max(delay,uint32_t(retry-epoch)*1000);
    next=now+delay;scheduled=true;
  }
};
inline bool validPassword(const std::string& password){
  if(password.empty()||password.size()>100)return false;
  for(unsigned char c:password)if(c<=32||c>=127)return false;
  return true;
}
inline const char* resultMessage(int code){
  switch(code){
    case 200:return "Accepted for processing";
    case 400:return "Station credentials or fields rejected";
    case 401:return "Station password rejected";
    case 409:return "Duplicate observation rejected";
    case 429:return "Rate limited; retry scheduled";
    default:return code>0?"Upload rejected":"Connection unavailable";
  }
}
// Per v2 schema, 200 means accepted for processing, not verified map display.
inline bool accepted(int http_status){return http_status==200;}
}

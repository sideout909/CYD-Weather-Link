#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
namespace pws_weather {
inline std::string encoded(const std::string& value){
  static constexpr char hex[]="0123456789ABCDEF";std::string out;
  for(unsigned char c:value){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=char(c);
    else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}
  return out;
}
inline void add(std::string& query,const char* name,const std::string& value){query+='&';query+=name;query+='=';query+=encoded(value);}
inline std::string number(double value,unsigned digits){char buffer[40];std::snprintf(buffer,sizeof(buffer),"%.*f",int(digits),value);return buffer;}
template<class Observation>
std::string query(const Observation& o,const std::string& id,const std::string& key,uint64_t now_ms){
  if(id.empty()||id.size()>64||key.empty()||key.size()>100||!o.live||!o.fresh||
      !o.epoch_ms||now_ms<o.epoch_ms||now_ms-o.epoch_ms>=120000)return {};
  for(unsigned char c:id)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return {};
  time_t stamp=o.epoch_ms/1000;tm utc{};gmtime_r(&stamp,&utc);char date[24];std::strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",&utc);
  std::string q="ID="+encoded(id);add(q,"PASSWORD",key);add(q,"dateutc",date);
  add(q,"action","updateraw");add(q,"softwaretype","WX-Link");
  bool fields=false;
  auto field=[&](const char* name,bool valid,double value,unsigned digits,double low,double high){
    if(valid&&std::isfinite(value)&&value>=low&&value<=high){add(q,name,number(value,digits));fields=true;}};
  field("tempf",o.temperature_valid,o.temperature_f,1,-148,176);
  field("humidity",o.humidity_valid,o.humidity,0,0,100);
  field("windspeedmph",o.wind_valid,o.wind_mph,1,0,300);
  field("winddir",o.direction_valid,o.direction_deg,0,0,360);
  field("windgustmph",o.gust_valid,o.gust_mph,1,0,300);
  field("rainin",o.rain_hour_valid,o.rain_hour_in,2,0,100);
  field("dailyrainin",o.rain_today_valid,o.rain_today_in,2,0,100);
  field("UV",o.uv_valid,o.uv,1,0,100);
  return fields?q:std::string{};
}
inline bool accepted(std::string body){
  const auto first=body.find_first_not_of(" \r\n\t");if(first==std::string::npos)return false;
  body=body.substr(first,body.find_last_not_of(" \r\n\t")-first+1);
  return body=="success"||body=="Data Logged and posted in METAR mirror."||body=="Data Logged and posted in METAR mirror";
}
}

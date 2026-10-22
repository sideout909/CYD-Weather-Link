#include "services_internal.h"
#include "web_history.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <LittleFS.h>
#include "aprs_weather.h"
#include "nws_logic.h"
#include "sun_root.h"
#include "aprs_login.h"
#include "firmware_updates.h"
#include "startup_health.h"
#include "nws_pressure.h"
#include "pws_weather.h"
#include "windy_weather.h"
#include "windy_root.h"
#include <lwip/sockets.h>
#include <lwip/tcp.h>

extern const uint8_t root_bundle[] asm("_binary_certs_x509_crt_bundle_bin_start");
namespace services::detail {
namespace {
struct PsramAllocator {
  void* allocate(size_t n){return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
  void deallocate(void* p){heap_caps_free(p);}
  void* reallocate(void* p,size_t n){return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
};
using Document=BasicJsonDocument<PsramAllocator>;
String forecast_url,hourly_url,stations_url;
uint32_t next_pws=0,pws_delay=300000;
windy_weather::Schedule windy_schedule;
std::array<String,3> pressure_stations;unsigned pressure_station_index=0;uint32_t next_pressure=0;
time_t point_fetched=0;
uint32_t next_forecast=0,next_alerts=0,next_sun=0;
uint32_t server_delay=0,retry_delay=0;
uint32_t forecast_delay=60000,alert_delay=30000;
uint32_t next_wu=0,next_cloud=0,next_aprs=0,wu_delay=60000,cloud_delay=600000,aprs_delay=300000;
WiFiClient aprs_socket;
bool aprs_logged_in=false;
uint32_t next_aprs_connect=0,aprs_connect_delay=30000;
void stopAprs(State& s,const String& reason){
  aprs_socket.stop();aprs_logged_in=false;s.aprs_connected=false;s.aprs_verified=false;s.aprs_state=reason;
}
bool maintainAprs(State& s){
  if(!config.aprs_enabled){stopAprs(s,"Off");return false;}
  if(WiFi.status()!=WL_CONNECTED||!clockValid()){stopAprs(s,WiFi.status()!=WL_CONNECTED?"Wi-Fi unavailable":"Waiting for clock");return false;}
  if(aprs_logged_in&&aprs_socket.connected()){
    // Drain a bounded amount of server comments/feed; never display or log it.
    for(unsigned n=0;n<1024&&aprs_socket.available();++n)aprs_socket.read();
    s.aprs_connected=true;return true;
  }
  if(aprs_logged_in)stopAprs(s,"Disconnected; reconnecting");
  if(int32_t(millis()-next_aprs_connect)<0)return false;
  next_aprs_connect=millis()+aprs_connect_delay;aprs_connect_delay=std::min(uint32_t(900000),aprs_connect_delay*2);
  s.aprs_state="Connecting";s.aprs_connected=false;writeState(s);
  const bool cwop=config.aprs_profile=="cwop";
  aprs_socket.setTimeout(2);
  if(!aprs_socket.connect(cwop?"cwop.aprs.net":"rotate.aprs2.net",14580,4000)){stopAprs(s,"Connection unavailable");return false;}
  aprs_socket.setNoDelay(true);
  int enabled=1,idle=30,interval=10,count=3;
  aprs_socket.setSocketOption(SOL_SOCKET,SO_KEEPALIVE,&enabled,sizeof(enabled));
  aprs_socket.setSocketOption(IPPROTO_TCP,TCP_KEEPIDLE,&idle,sizeof(idle));
  aprs_socket.setSocketOption(IPPROTO_TCP,TCP_KEEPINTVL,&interval,sizeof(interval));
  aprs_socket.setSocketOption(IPPROTO_TCP,TCP_KEEPCNT,&count,sizeof(count));
  const uint32_t deadline=millis()+5000;String line;bool greeting=false,overflow=false;
  aprs_login::Reply reply=aprs_login::Reply::Other;
  while(aprs_socket.connected()&&int32_t(deadline-millis())>0){
    for(unsigned n=0;n<1024&&aprs_socket.available();++n){
      const char ch=aprs_socket.read();if(ch=='\r')continue;
      if(ch=='\n'){
        if(!overflow){
          if(!greeting&&line.startsWith("#")){
            greeting=true;aprs_socket.print("user "+config.callsign+" pass "+(cwop?String("-1"):config.aprs_passcode)+" vers WX-Link "+String(firmware_version::current)+"\r\n");
          }else reply=aprs_login::parse(line.c_str(),config.callsign.c_str());
        }
        line="";overflow=false;
        if(reply!=aprs_login::Reply::Other)break;
      }else if(line.length()<512)line+=ch;else overflow=true;
    }
    if(reply!=aprs_login::Reply::Other)break;vTaskDelay(pdMS_TO_TICKS(10));
  }
  if(reply==aprs_login::Reply::Other||(!cwop&&reply!=aprs_login::Reply::Verified)){stopAprs(s,"Login not accepted");return false;}
  aprs_logged_in=true;s.aprs_connected=true;s.aprs_verified=reply==aprs_login::Reply::Verified;
  s.aprs_state=cwop?"Connected to CWOP":"Connected; verified login";aprs_connect_delay=30000;writeState(s);return true;
}
time_t isoTime(const char* input){return time_t(nws::isoTime(input));}
bool getJson(const String& url,Document& out,const JsonDocument& filter,String& error){
  if(!url.startsWith("https://api.weather.gov/")&&!url.startsWith("https://aviationweather.gov/api/data/metar?")){error="Invalid NWS endpoint";return false;}
  WiFiClientSecure tls;tls.setCACertBundle(root_bundle);tls.setHandshakeTimeout(6);
  HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(4000);http.setTimeout(5000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);http.setRedirectLimit(3);
  const char* response_headers[]={"Cache-Control","Retry-After"};http.collectHeaders(response_headers,2);
  server_delay=retry_delay=0;
  if(!http.begin(tls,url)){error="NWS connection failed";return false;}
  http.addHeader("User-Agent",String("WX-Link/")+firmware_version::current+" (personal weather display)");http.addHeader("Accept","application/geo+json");
  int code=http.GET();
  String retry=http.header("Retry-After");if(retry.length()){long seconds=retry.toInt();retry_delay=seconds>0?std::min(uint32_t(86400000),uint32_t(seconds)*1000):300000;}
  String cache=http.header("Cache-Control");int max_age=cache.indexOf("max-age=");if(max_age>=0){long seconds=cache.substring(max_age+8).toInt();if(seconds>0)server_delay=std::min(uint32_t(86400000),uint32_t(seconds)*1000);}
  if(code!=200){error=code>0?"NWS HTTP "+String(code):"NWS connection unavailable";http.end();return false;}
  const auto parsed=deserializeJson(out,http.getStream(),DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(16));
  http.end();if(parsed){error="NWS response incomplete";return false;}error="Current";return true;
}
bool aviationPressure(State& s){
  String ids;for(const auto& id:pressure_stations){if(id.length()!=4)continue;if(ids.length())ids+=",";ids+=id;}
  if(ids.isEmpty())return false;
  Document doc(8192);StaticJsonDocument<384> filter;
  for(const char* key:{"icaoId","obsTime","altim","slp"})filter[0][key]=true;
  String error;
  if(!getJson("https://aviationweather.gov/api/data/metar?ids="+ids+"&format=json",doc,filter,error)){
    s.pressure_error=s.pressure_primary_error+"; AWC unavailable";return false;
  }
  // Prefer the nearest configured station, not arbitrary API result ordering.
  for(const auto& station:pressure_stations)for(JsonObject p:doc.as<JsonArray>()){
    if(p["icaoId"]!=station||!p["obsTime"].is<int64_t>())continue;
    float pa=0;bool sea=false,altimeter=false;const time_t stamp=p["obsTime"].as<int64_t>();
    const double slp=p["slp"].is<double>()?p["slp"].as<double>():NAN;
    const double alt=p["altim"].is<double>()?p["altim"].as<double>():NAN;
    if(!nws_pressure::metar(slp,alt,stamp,time(nullptr),pa,sea,altimeter))continue;
    s.pressure_pa=pa;s.pressure_observed=stamp;s.pressure_station=station;
    s.pressure_sea_level=sea;s.pressure_altimeter=altimeter;s.pressure_provider="NWS AWC";
    s.pressure_error="Current";next_pressure=millis()+std::max(uint32_t(600000),std::max(server_delay,retry_delay));return true;
  }
  s.pressure_error=s.pressure_primary_error+"; no fresh AWC pressure";return false;
}
void pressure(State& s){
  // Nearby station observation, never a replacement local RF measurement.
  String error;Document doc(16384);StaticJsonDocument<1024> filter;
  if(pressure_stations[0].isEmpty()){
    if(stations_url.isEmpty()){s.pressure_error="Waiting for NWS station lookup";return;}
    filter["features"][0]["properties"]["stationIdentifier"]=true;
    if(!getJson(stations_url,doc,filter,error)){s.pressure_error=error;return;}
    unsigned n=0;for(JsonObject f:doc["features"].as<JsonArray>()){
      String id=f["properties"]["stationIdentifier"]|String("");bool valid=id.length()>0&&id.length()<=8;
      for(char c:id)valid&=isalnum(static_cast<unsigned char>(c));
      if(valid){pressure_stations[n++]=id;if(n==pressure_stations.size())break;}
    }
    if(!n){s.pressure_error="No nearby NWS reporting stations";return;}
    pressure_station_index=0;doc.clear();filter.clear();
  }
  const String station=pressure_stations[pressure_station_index];
  s.pressure_attempt_station=station;s.pressure_attempted=time(nullptr);
  filter["properties"]["timestamp"]=true;
  for(const char* key:{"seaLevelPressure","barometricPressure"}){filter["properties"][key]["value"]=true;filter["properties"][key]["unitCode"]=true;filter["properties"][key]["qualityControl"]=true;}
  bool ok=getJson("https://api.weather.gov/stations/"+station+"/observations/latest",doc,filter,error);
  s.pressure_attempt_timestamp=doc["properties"]["timestamp"]|String("");
  const time_t stamp=isoTime(doc["properties"]["timestamp"]);s.pressure_attempt_observed=stamp;float pa=0;bool sea=false;
  if(ok&&!nws_pressure::fresh(stamp,time(nullptr))){ok=false;error="NWS pressure observation too old or invalid";}
  if(ok){
    ok=false;
    for(const char* key:{"seaLevelPressure","barometricPressure"}){
      auto p=doc["properties"][key];const char* qc=p["qualityControl"]|"";
      const bool level=!strcmp(key,"seaLevelPressure");
      if(p["value"].is<float>()&&strcmp(qc,"X")&&strcmp(qc,"B")&&
         nws_pressure::pascals(p["value"].as<double>(),p["unitCode"]|"",level,pa)){sea=level;ok=true;break;}
    }
    if(!ok)error="NWS pressure unavailable";
  }
  if(ok){s.pressure_pa=pa;s.pressure_observed=stamp;s.pressure_sea_level=sea;s.pressure_altimeter=false;s.pressure_provider="NWS";s.pressure_primary_error="";s.pressure_station=station;s.pressure_error="Current";next_pressure=millis()+std::max(uint32_t(600000),std::max(server_delay,retry_delay));}
  else{s.pressure_error=error;s.pressure_primary_error=error;if(aviationPressure(s))return;pressure_station_index=(pressure_station_index+1)%pressure_stations.size();if(pressure_stations[pressure_station_index].isEmpty())pressure_station_index=0;}
}

void forecast(State& s){
  String error;Document doc(100000);StaticJsonDocument<1536> filter;
  if(time(nullptr)-point_fetched>86400)forecast_url="";
  if(forecast_url.isEmpty()){
    filter["properties"]["forecast"]=true;filter["properties"]["forecastHourly"]=true;filter["properties"]["observationStations"]=true;
    String point="https://api.weather.gov/points/"+String(config.latitude,4)+","+String(config.longitude,4);
    if(!getJson(point,doc,filter,error)){s.forecast_error=error;return;}
    forecast_url=doc["properties"]["forecast"].as<String>();hourly_url=doc["properties"]["forecastHourly"].as<String>();
    if(!forecast_url.startsWith("https://api.weather.gov/")){forecast_url="";s.forecast_error="NWS unavailable at this location";return;}
    stations_url=doc["properties"]["observationStations"].as<String>();pressure_stations={};
    point_fetched=time(nullptr);doc.clear();filter.clear();
  }
  auto p=filter["properties"]["periods"][0];
  for(const char* field:{"name","startTime","endTime","temperature","temperatureUnit","shortForecast","detailedForecast","windSpeed","windDirection","isDaytime"})p[field]=true;
  p["probabilityOfPrecipitation"]["value"]=true;filter["properties"]["updateTime"]=true;
  if(!getJson(forecast_url,doc,filter,error)){s.forecast_error=error;return;}
  auto periods=doc["properties"]["periods"].as<JsonArray>();
  if(!periods.size()){s.forecast_error="NWS returned no forecast periods";return;}
  s.forecast_count=0;
  for(JsonObject p:periods){
    if(s.forecast_count>=s.forecast.size())break;
    if(isoTime(p["endTime"])<=time(nullptr))continue;
    auto& f=s.forecast[s.forecast_count++];f=ForecastPeriod{};
    f.is_daytime=p["isDaytime"]|false;f.daytime_known=p["isDaytime"].is<bool>();f.name=p["name"].as<String>();f.short_text=p["shortForecast"].as<String>();f.detail=p["detailedForecast"].as<String>();
    f.wind=p["windDirection"].as<String>()+" "+p["windSpeed"].as<String>();
    f.temperature_valid=p["temperature"].is<int>();f.temperature=p["temperature"]|0;
    if(p["temperatureUnit"]=="C")f.temperature=lroundf(f.temperature*1.8f+32);
    f.rain_probability=p["probabilityOfPrecipitation"]["value"]|-1;
    f.start=isoTime(p["startTime"]);f.end=isoTime(p["endTime"]);
  }
  s.forecast_issued=isoTime(doc["properties"]["updateTime"]);
  s.forecast_valid=s.forecast_count>0;s.forecast_fetched=time(nullptr);s.forecast_error=s.forecast_valid?"Current":"No future forecast periods";
  doc.clear();
  if(!getJson(hourly_url,doc,filter,error)){s.hourly_error=error;return;}
  if(!doc["properties"]["periods"].is<JsonArray>()){s.hourly_error="No hourly forecast";return;}
  s.hourly_count=0;
  for(JsonObject p:doc["properties"]["periods"].as<JsonArray>()){
    if(s.hourly_count>=s.hourly.size())break;time_t start=isoTime(p["startTime"]),end=isoTime(p["endTime"]);if(end<=time(nullptr))continue;
    auto& f=s.hourly[s.hourly_count++];f=ForecastPeriod{};struct tm local{};localtime_r(&start,&local);char name[32];strftime(name,sizeof(name),"%a %I:%M %p",&local);f.name=name;f.is_daytime=p["isDaytime"]|false;f.daytime_known=p["isDaytime"].is<bool>();
    f.short_text=p["shortForecast"].as<String>();f.detail=p["detailedForecast"].as<String>();f.wind=p["windDirection"].as<String>()+" "+p["windSpeed"].as<String>();
    f.temperature_valid=p["temperature"].is<int>();f.temperature=p["temperature"]|0;if(p["temperatureUnit"]=="C")f.temperature=lroundf(f.temperature*1.8f+32);
    f.rain_probability=p["probabilityOfPrecipitation"]["value"]|-1;f.start=start;f.end=end;
  }
  s.hourly_fetched=time(nullptr);s.hourly_issued=isoTime(doc["properties"]["updateTime"]);s.hourly_error=s.hourly_count?"Current":"No future hourly periods";

}
int alertRank(const String& event){return nws::alertRank(event.c_str());}
void alerts(State& s){
  Document doc(100000);StaticJsonDocument<1536>filter;
  auto p=filter["features"][0]["properties"];
  for(const char* field:{"id","event","headline","description","instruction","expires","ends","status","messageType"})p[field]=true;
  filter["pagination"]["next"]=true;String error;
  String url="https://api.weather.gov/alerts/active?point="+String(config.latitude,4)+","+String(config.longitude,4)+"&status=actual";
  if(!getJson(url,doc,filter,error)){s.alert_error=error;return;}
  if(!doc["features"].is<JsonArray>()){s.alert_error="Invalid NWS alert response";return;}
  std::array<Alert,8> list;uint8_t count=0;const char* next_page=doc["pagination"]["next"]|"";bool truncated=next_page[0];
  for(JsonObject feature:doc["features"].as<JsonArray>()){
    JsonObject p=feature["properties"];
    if(p["status"]!="Actual" || p["messageType"]=="Cancel")continue;
    Alert a;a.expires=isoTime(p["expires"]);if(!a.expires || a.expires<=time(nullptr))continue;
    a.id=p["id"].as<String>();a.event=p["event"].as<String>();a.headline=p["headline"].as<String>();
    a.description=p["description"].as<String>();a.instruction=p["instruction"].as<String>();
    a.rank=alertRank(a.event);a.category=a.event.endsWith("Warning")?3:a.event.endsWith("Watch")?2:1;
    if(count<list.size())list[count++]=a;
    else{truncated=true;if(a.rank>list[count-1].rank)list[count-1]=a;}
    std::sort(list.begin(),list.begin()+count,[](const Alert& a,const Alert& b){return a.rank>b.rank;});
  }
  const uint8_t next_theme=count?list[0].category:0;
  if(!truncated || next_theme>=s.alert_theme){s.alerts=list;s.alert_count=count;s.alert_theme=next_theme;}
  s.alerts_truncated=truncated;s.alerts_valid=!truncated;if(!truncated)s.alert_fetched=time(nullptr);
  s.alert_error=truncated?"Additional alerts may exist":"Current";
}
String encoded(const String& input){
  const char hex[]="0123456789ABCDEF";String out;out.reserve(input.length()*3);
  for(unsigned char c:input){if(isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')out+=char(c);else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;
}
void add(String& url,const char* key,const String& value){url+='&';url+=key;url+='=';url+=encoded(value);}
String timestamp(uint64_t ms,const char* format){time_t t=ms/1000;struct tm utc{};gmtime_r(&t,&utc);char b[40];strftime(b,sizeof(b),format,&utc);return b;}
bool upload(const String& url,bool weathercloud,String& result,bool pws=false){
  Observation current_observation{};if(xSemaphoreTake(state_mutex,pdMS_TO_TICKS(20))==pdTRUE){current_observation=latest;xSemaphoreGive(state_mutex);}
  if(!current_observation.live || !current_observation.fresh){result="Waiting for live observations";return false;}
  WiFiClientSecure tls;tls.setCACertBundle(root_bundle);tls.setHandshakeTimeout(6);
  HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(4000);http.setTimeout(5000);
  if(!http.begin(tls,url)){result="Connection failed";return false;}
  int code=http.GET();String body;
  if(code==200){char b[96];size_t n=http.getStream().readBytes(b,sizeof(b)-1);b[n]=0;body=b;body.trim();}
  http.end();
  if(code==200 && (pws?pws_weather::accepted(body.c_str()):weathercloud?body=="200":body=="success")){result="Accepted";return true;}
  if(code==200)result=weathercloud&&body=="401"?"Station ID / key rejected":weathercloud&&body=="429"?"Rate limited":"Upload rejected";
  else result=code>0?"HTTP "+String(code):"Connection unavailable";
  return false;
}
bool uploadWu(const Observation& o,String& result){
  String url="https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?ID="+encoded(config.wu_id);
  add(url,"PASSWORD",config.wu_key);add(url,"dateutc",timestamp(o.epoch_ms,"%Y-%m-%d %H:%M:%S"));add(url,"action","updateraw");add(url,"softwaretype","CYDWeatherLink-0.2");
  if(o.temperature_valid)add(url,"tempf",String(o.temperature_f,1));if(o.humidity_valid)add(url,"humidity",String(o.humidity));
  if(o.wind_valid)add(url,"windspeedmph",String(o.wind_mph,1));if(o.direction_valid)add(url,"winddir",String(int(lroundf(o.direction_deg))%360));
  if(o.gust_valid)add(url,"windgustmph",String(o.gust_mph,1));if(o.rain_today_valid)add(url,"dailyrainin",String(o.rain_today_in,2));
  if(o.rain_hour_valid)add(url,"rainin",String(o.rain_hour_in,2));if(o.uv_valid)add(url,"UV",String(o.uv,1));
  return upload(url,false,result);
}
bool uploadWindy(const Observation& o,State& s){
  const uint64_t now_ms=clockMs();
  const auto query=windy_weather::query(o,config.windy_id.c_str(),now_ms);
  Observation current{};
  if(xSemaphoreTake(state_mutex,pdMS_TO_TICKS(20))==pdTRUE){current=latest;xSemaphoreGive(state_mutex);}
  if(query.empty()||!current.live||!current.fresh||!current.epoch_ms||
      current.epoch_ms>now_ms||now_ms-current.epoch_ms>=120000){s.windy_state="Waiting for eligible live fields";return false;}
  if(!windy_weather::validPassword(config.windy_password.c_str())){s.windy_state="Station password invalid";return false;}
  s.windy_attempted=time(nullptr);s.windy_http_code=0;s.windy_tls_code=0;
  s.windy_dns_server=WiFi.dnsIP().toString();IPAddress resolved;
  if(!WiFi.hostByName("stations.windy.com",resolved)){
    s.windy_dns_ip="";s.windy_state="Windy DNS lookup failed";
    windy_schedule.result(millis(),false,time(nullptr));return false;
  }
  s.windy_dns_ip=resolved.toString();s.windy_state="Connecting to Windy";writeState(s);

  WiFiClientSecure tls;tls.setCACert(kWindyRoot);tls.setHandshakeTimeout(6);
  HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(4000);http.setTimeout(5000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if(!http.begin(tls,String(windy_weather::endpoint)+"?"+query.c_str())){
    s.windy_state="Connection failed";windy_schedule.result(millis(),false,time(nullptr));return false;
  }
  http.addHeader("Authorization","Bearer "+config.windy_password);
  const int code=http.GET();String retry;
  s.windy_http_code=code;char tls_error[96];const int tls_result=tls.lastError(tls_error,sizeof(tls_error));
  s.windy_tls_code=tls_result<0?tls_result:0; // SDK also retains positive handshake results.
  if(code==429){
    char body[257];const size_t n=http.getStream().readBytes(body,sizeof(body)-1);body[n]=0;
    StaticJsonDocument<384> d;if(!deserializeJson(d,body))retry=d["retry_after"]|String("");
  }
  http.end();const bool accepted=windy_weather::accepted(code);
  s.windy_state=code<0?"Connection failed: HTTP "+String(code)+" / TLS "+String(s.windy_tls_code):windy_weather::resultMessage(code);
  if(accepted)s.windy_success=time(nullptr);
  windy_schedule.result(millis(),accepted,time(nullptr),retry.c_str());return accepted;
}
bool uploadPws(const Observation& o,String& result){
  const std::string query=pws_weather::query(o,config.pws_id.c_str(),config.pws_key.c_str(),clockMs());
  if(query.empty()){result="Waiting for eligible live fields";return false;}
  const String url=String("https://www.pwsweather.com/pwsupdate/pwsupdate.php?")+query.c_str();
  return upload(url,false,result,true);
}
bool uploadCloud(const Observation& o,String& result){
  String url="https://api.weathercloud.net/v01/set?wid="+encoded(config.weathercloud_id);add(url,"key",config.weathercloud_key);
  add(url,"software","cyd_weather_link_0.2");add(url,"date",timestamp(o.epoch_ms,"%Y%m%d"));add(url,"time",timestamp(o.epoch_ms,"%H%M"));
  if(o.temperature_valid)add(url,"temp",String(lroundf((o.temperature_f-32)/1.8f*10)));if(o.humidity_valid)add(url,"hum",String(o.humidity));
  if(o.wind_valid && o.wind_mph<=134.2f)add(url,"wspd",String(lroundf(o.wind_mph*4.4704f)));
  if(o.direction_valid)add(url,"wdir",String(int(lroundf(o.direction_deg))%360));
  if(o.rain_today_valid && o.rain_today_in<=39.37f)add(url,"rain",String(lroundf(o.rain_today_in*254)));
  if(o.uv_valid)add(url,"uvi",String(lroundf(o.uv*10)));
  // Do not invent 10-minute peak/mean fields from a 5-minute display gust.
  return upload(url,true,result);
}

bool uploadAprs(const Observation& o,String& result){
  aprs::Weather w;w.epoch=o.epoch_ms/1000;w.latitude=config.latitude;w.longitude=config.longitude;
  w.temperature_valid=o.temperature_valid;w.temperature_f=o.temperature_f;w.humidity_valid=o.humidity_valid;w.humidity=o.humidity;
  w.gust_valid=o.gust_valid;w.gust_mph=o.gust_mph;w.direction_valid=o.direction_valid;w.direction_deg=o.direction_deg;
  w.rain_today_valid=o.rain_today_valid;w.rain_today=o.rain_today_in;w.rain_hour_valid=o.rain_hour_valid;w.rain_hour=o.rain_hour_in;w.rain_24h_valid=o.rain_24h_valid;w.rain_24h=o.rain_24h_in;
  std::string packet=aprs::packet(config.callsign.c_str(),w);
  if(packet.empty()){result="Invalid packet settings";return false;}
  if(!aprs_logged_in||!aprs_socket.connected()){result="APRS not connected";return false;}
  // A replay/reset may have begun while connecting. Recheck the latest source.
  Observation current_observation{};if(xSemaphoreTake(state_mutex,pdMS_TO_TICKS(20))==pdTRUE){current_observation=latest;xSemaphoreGive(state_mutex);}
  if(!current_observation.live || !current_observation.fresh){result="Live observation unavailable";return false;}
  bool sent=aprs_socket.write(reinterpret_cast<const uint8_t*>(packet.data()),packet.size())==packet.size();
  if(!sent){aprs_socket.stop();aprs_logged_in=false;}
  result=sent?"Sent; ingestion unconfirmed":"Send failed";return sent;
}

bool portalSelftest(String& failure){
  // Start only after the main task has actually serviced HTTP. Each probe gets
  // a fresh client, so credentials and request headers cannot cross test cases.
  // Test the actual HTTP server through SDK-enabled loopback; AP/STA routes
  // can change during Wi-Fi/mDNS initialization. This still exercises auth/CSRF.
  const String root="http://127.0.0.1";
  auto expect=[&](const char* stage,const char* path,int expected,const String& password,
                  const char* body=nullptr,const String& token=String(),String* response=nullptr){
    failure=stage;
    WiFiClient client;HTTPClient http;http.setConnectTimeout(1000);http.setTimeout(2000);
    if(!http.begin(client,root+path)){failure+=" (client setup)";return false;}
    if(password.length())http.setAuthorization("admin",password.c_str());
    if(token.length())http.addHeader("X-CSRF-Token",token);
    if(body)http.addHeader("Content-Type","application/json");
    const int code=body?http.POST(String(body)):http.GET();
    if(code!=expected){failure+=" (HTTP "+String(code)+")";http.end();return false;}
    if(response)*response=http.getString();
    http.end();return true;
  };
  const String none;
  if(!expect("Unauthenticated config","/api/config",401,none)||
     !expect("Unauthenticated updater","/api/update",401,none)||
     !expect("Unauthenticated archive","/api/archive",401,none)||
     !expect("Unauthenticated CSV","/api/archive.csv?end=1&offset=0",401,none))return false;
  for(const char* path:{"/api/history","/api/forecast","/api/alerts","/api/discovery","/assets/discovery.js"})if(!expect("Dashboard authentication",path,401,none))return false;
  String response;
  if(!expect("Authenticated config","/api/config",200,config.setup_key,nullptr,none,&response))return false;
  DynamicJsonDocument d(4096);
  if(deserializeJson(d,response)||d["station_id"].as<unsigned>()!=config.station_id||
     d.containsKey("setup_key")||d.containsKey("setup_wifi_key")||d.containsKey("wifi_password")||d.containsKey("wu_key")||d.containsKey("pws_key")||d.containsKey("windy_password")||d.containsKey("weathercloud_key")||d.containsKey("aprs_passcode")){
    failure="Config schema or secret masking";return false;
  }
  const String token=d["csrf"].as<String>();
  if(!token.length()){failure="Config CSRF token missing";return false;}
  if(d["update_channel"]!=config.update_channel||d["hostname"]!=config.hostname||d["units"]!=config.units||d["windy_enabled"]!=config.windy_enabled||d["pws_enabled"]!=config.pws_enabled){failure="Settings migration schema";return false;}
  // Rejected validation probes cannot save or restart the device.
  for(const char* invalid:{"{\"update_channel\":\"nightly\"}","{\"update_channel\":42}","{\"hostname\":\"bad.local\"}","{\"hostname\":42}","{\"windy_password\":\"bad\\r\\nheader\"}","{\"windy_enabled\":\"yes\"}","{\"pws_enabled\":\"yes\"}","{\"units\":\"unknown\"}","{\"lux_calibrate\":\"yes\"}"})
    if(!expect("Settings validation rejection","/api/config",400,config.setup_key,invalid,token))return false;
  // During OTA acceptance, a fully valid no-op request must still be locked.
  // This probe is never sent after confirmation, where it would save/restart.
  if(firmware_updates::state().boot_pending&&
      !expect("Pending boot save lock","/api/config",409,config.setup_key,"{}",token))return false;
  // Negative reset probes only: never send the destructive confirmation.
  if(!expect("Reset authentication","/api/factory-reset",401,none,"{}")||
     !expect("Reset CSRF rejection","/api/factory-reset",403,config.setup_key,"{}")||
     !expect("Reset confirmation rejection","/api/factory-reset",400,config.setup_key,"{}",token))return false;
  if(d["display_theme"]!=config.display_theme||d["date_format"]!=config.date_format||d["clock_24h"]!=config.clock_24h||d["display_sleep_mode"]!=config.display_sleep_mode||d["display_timeout_s"]!=config.display_timeout_s){failure="Display preferences schema";return false;}
  if(!expect("Local address status","/api/status",200,config.setup_key,nullptr,none,&response))return false;
  if(deserializeJson(d,response)||d["local_url"]!=portalUrl()||!d["mdns_ready"].is<bool>()||!d["ip"].is<const char*>()){failure="Local address schema";return false;}
  if(!expect("Discovery schema","/api/discovery",200,config.setup_key,nullptr,none,&response))return false;
  if(deserializeJson(d,response)||d["duration_s"]!=300||!d["candidates"].is<JsonArray>()){failure="Discovery schema";return false;}
  if(!expect("Discovery CSRF rejection","/api/discovery/start",403,config.setup_key,""))return false;
  if(!expect("Discovery cancel CSRF rejection","/api/discovery/cancel",403,config.setup_key,""))return false;
  if(!expect("Authenticated archive","/api/archive",200,config.setup_key,nullptr,none,&response))return false;
  if(deserializeJson(d,response)||d["schema"]!=1||d["retention_days"]!=7){failure="Archive schema";return false;}
  if(config.admin_custom){
    if(config.setup_wifi_key==config.setup_key){failure="Admin/AP key separation";return false;}
    if(!expect("AP key denied admin access","/api/config",401,config.setup_wifi_key))return false;
  }
  if(!expect("Local map asset","/assets/location.js",200,config.setup_key,nullptr,none,&response))return false;
  if(response.length()<=100||uint8_t(response[0])!=0x1f||uint8_t(response[1])!=0x8b){failure="Local map asset content";return false;}
  if(!expect("Dashboard asset","/assets/dashboard.js",200,config.setup_key,nullptr,none,&response))return false;
  if(response.length()<=100||uint8_t(response[0])!=0x1f||uint8_t(response[1])!=0x8b){failure="Dashboard asset content";return false;}
  if(!expect("Dashboard forecast","/api/forecast?type=daily&offset=0",200,config.setup_key,nullptr,none,&response))return false;
  if(deserializeJson(d,response)||!d["periods"].is<JsonArray>()){failure="Forecast dashboard schema";return false;}
  if(!expect("Dashboard alerts","/api/alerts?offset=0",200,config.setup_key,nullptr,none,&response))return false;
  if(deserializeJson(d,response)||!d["alerts"].is<JsonArray>()){failure="Alert dashboard schema";return false;}
  if(clockValid()){
    if(!expect("Dashboard history","/api/history?offset=1320&count=120",200,config.setup_key,nullptr,none,&response))return false;
    DynamicJsonDocument history_page(web_history::kDocumentBytes);
    if(deserializeJson(history_page,response)||history_page["total"]!=1440||history_page["next_offset"]!=1440||history_page["rows"].size()!=120){failure="History dashboard schema";return false;}
  }
  if(!expect("Dashboard invalid page","/api/forecast?offset=bad",400,config.setup_key))return false;
  if(!expect("Updater CSRF rejection","/api/update/install",403,config.setup_key,""))return false;
  if(!expect("Settings CSRF rejection","/api/config",403,config.setup_key,"{}"))return false;
  if(!expect("Invalid station rejection","/api/config",400,config.setup_key,"{\"station_id\":65536}",token))return false;
  if(!expect("Invalid display timeout rejection","/api/config",400,config.setup_key,"{\"display_timeout_s\":65566}",token))return false;
  if(!expect("Invalid clock setting rejection","/api/config",400,config.setup_key,"{\"clock_24h\":\"false\"}",token))return false;
  if(!expect("Invalid theme rejection","/api/config",400,config.setup_key,"{\"display_theme\":\"unknown\"}",token))return false;
  if(!expect("Invalid theme type rejection","/api/config",400,config.setup_key,"{\"display_theme\":false}",token))return false;
  failure="Passed";return true;
}
int32_t localDay(){time_t t=time(nullptr);struct tm tm{};localtime_r(&t,&tm);return (tm.tm_year+1900)*10000+(tm.tm_mon+1)*100+tm.tm_mday;}
void fetchSun(State& s){
  time_t now=time(nullptr);struct tm local{};localtime_r(&now,&local);char date[16];strftime(date,sizeof(date),"%Y-%m-%d",&local);
  String zone="America/Detroit";if(config.timezone.startsWith("CST"))zone="America/Chicago";else if(config.timezone=="MST7")zone="America/Phoenix";else if(config.timezone.startsWith("MST"))zone="America/Denver";else if(config.timezone.startsWith("PST"))zone="America/Los_Angeles";else if(config.timezone=="UTC0")zone="Etc/UTC";
  String url="https://api.sunrise-sunset.org/v2?lat="+String(config.latitude,5)+"&lng="+String(config.longitude,5)+"&date="+date+"&time_format=unix&tz="+encoded(zone);
  WiFiClientSecure tls;tls.setCACert(kSunRoot);tls.setHandshakeTimeout(6);HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(4000);http.setTimeout(5000);
  const char* headers[]={"Retry-After"};http.collectHeaders(headers,1);
  if(!http.begin(tls,url)){s.sun_error="Sun connection failed";next_sun=millis()+900000;return;}
  int code=http.GET();if(code!=200){long retry=http.header("Retry-After").toInt();next_sun=millis()+std::max(uint32_t(900000),uint32_t(std::min(86400L,std::max(0L,retry)))*1000);char reason[120];int tls_code=tls.lastError(reason,sizeof(reason));s.sun_error=code>0?"Sun HTTP "+String(code):"Sun connection "+String(code)+" / TLS "+String(tls_code);http.end();return;}
  Document d(4096);StaticJsonDocument<512>filter;for(const char* key:{"date","sunrise","sunset","sun_status","moon_phase","moon_illumination"})filter[key]=true;
  bool ok=!deserializeJson(d,http.getStream(),DeserializationOption::Filter(filter));http.end();
  String status=d["sun_status"]|String("unknown");float illumination=d["moon_illumination"]|-1.0f;
  if(!ok || d["date"]!=date || (status!="normal"&&status!="midnight_sun"&&status!="polar_night") || illumination<0 || illumination>100){s.sun_error="Invalid sun response";next_sun=millis()+900000;return;}
  s.sunrise=d["sunrise"]|time_t(0);s.sunset=d["sunset"]|time_t(0);s.sun_status=status;s.sun_day=localDay();s.sun_fetched=now;s.moon_phase=d["moon_phase"].as<String>();s.moon_illumination=illumination;s.sun_error="Current";next_sun=millis()+60000;
}
void saveCache(const State& s){
  Document d(100000);d["schema"]=1;d["sunrise"]=s.sunrise;d["sunset"]=s.sunset;d["sun_day"]=s.sun_day;d["sun_fetched"]=s.sun_fetched;d["sun_status"]=s.sun_status;d["moon_phase"]=s.moon_phase;d["moon_illumination"]=s.moon_illumination;d["lat"]=config.latitude;d["lon"]=config.longitude;
  d["forecast_fetched"]=s.forecast_fetched;d["forecast_issued"]=s.forecast_issued;d["hourly_fetched"]=s.hourly_fetched;d["hourly_issued"]=s.hourly_issued;d["alert_fetched"]=s.alert_fetched;d["alerts_truncated"]=s.alerts_truncated;
  auto forecasts=d.createNestedArray("forecast");
  for(unsigned i=0;i<s.forecast_count;++i){const auto& f=s.forecast[i];auto v=forecasts.createNestedObject();v["name"]=f.name;v["short"]=f.short_text;v["detail"]=f.detail;v["wind"]=f.wind;v["temp"]=f.temperature;v["temp_valid"]=f.temperature_valid;v["daytime"]=f.is_daytime;v["daytime_known"]=f.daytime_known;v["rain"]=f.rain_probability;v["start"]=f.start;v["end"]=f.end;}
  auto hours=d.createNestedArray("hourly");
  for(unsigned i=0;i<s.hourly_count;++i){const auto& f=s.hourly[i];auto v=hours.createNestedObject();v["name"]=f.name;v["short"]=f.short_text;v["detail"]=f.detail;v["wind"]=f.wind;v["temp"]=f.temperature;v["temp_valid"]=f.temperature_valid;v["daytime"]=f.is_daytime;v["daytime_known"]=f.daytime_known;v["rain"]=f.rain_probability;v["start"]=f.start;v["end"]=f.end;}
  auto alerts=d.createNestedArray("alerts");
  for(unsigned i=0;i<s.alert_count;++i){const auto& a=s.alerts[i];auto v=alerts.createNestedObject();v["id"]=a.id;v["event"]=a.event;v["headline"]=a.headline;v["description"]=a.description;v["instruction"]=a.instruction;v["category"]=a.category;v["rank"]=a.rank;v["expires"]=a.expires;}
  if(d.overflowed())return;File f=LittleFS.open("/nws.tmp","w");if(!f)return;
  size_t written=serializeJson(d,f);f.flush();f.close();if(written==measureJson(d))LittleFS.rename("/nws.tmp","/nws.json");
}
void loadCache(State& s){
  File f=LittleFS.open("/nws.json","r");if(!f||f.size()>120000)return;
  Document d(100000);if(deserializeJson(d,f)||d["schema"]!=1||fabs(d["lat"].as<double>()-config.latitude)>0.00001||fabs(d["lon"].as<double>()-config.longitude)>0.00001)return;
  s.sunrise=d["sunrise"]|time_t(0);s.sunset=d["sunset"]|time_t(0);s.sun_day=d["sun_day"]|0;s.sun_fetched=d["sun_fetched"]|time_t(0);s.sun_status=d["sun_status"]|String("unknown");s.moon_phase=d["moon_phase"].as<String>();s.moon_illumination=d["moon_illumination"]|-1.0f;
  if(s.sun_fetched)s.sun_error="Cached";
  s.forecast_count=0;s.alert_count=0;
  for(JsonObject p:d["forecast"].as<JsonArray>()){
    if(s.forecast_count>=s.forecast.size())break;if(p["end"].as<time_t>()<=time(nullptr))continue;
    auto& v=s.forecast[s.forecast_count++];v.name=p["name"].as<String>();v.short_text=p["short"].as<String>();v.detail=p["detail"].as<String>();v.wind=p["wind"].as<String>();
    v.temperature=p["temp"]|0;v.temperature_valid=p["temp_valid"]|false;v.is_daytime=p["daytime"]|false;v.daytime_known=p["daytime_known"]|false;v.rain_probability=p["rain"]|-1;v.start=p["start"]|time_t(0);v.end=p["end"]|time_t(0);
  }
  s.hourly_count=0;
  for(JsonObject p:d["hourly"].as<JsonArray>()){
    if(s.hourly_count>=s.hourly.size())break;if(p["end"].as<time_t>()<=time(nullptr))continue;
    auto& v=s.hourly[s.hourly_count++];v.name=p["name"].as<String>();v.short_text=p["short"].as<String>();v.detail=p["detail"].as<String>();v.wind=p["wind"].as<String>();v.temperature=p["temp"]|0;v.temperature_valid=p["temp_valid"]|false;v.is_daytime=p["daytime"]|false;v.daytime_known=p["daytime_known"]|false;v.rain_probability=p["rain"]|-1;v.start=p["start"]|time_t(0);v.end=p["end"]|time_t(0);
  }
  s.forecast_issued=d["forecast_issued"]|time_t(0);s.hourly_fetched=d["hourly_fetched"]|time_t(0);s.hourly_issued=d["hourly_issued"]|time_t(0);if(s.hourly_count)s.hourly_error="Cached / refresh pending";
  for(JsonObject p:d["alerts"].as<JsonArray>()){
    if(s.alert_count>=s.alerts.size())break; // Retain last-known hazards as unverified, even after expiry.
    auto& v=s.alerts[s.alert_count++];v.id=p["id"].as<String>();v.event=p["event"].as<String>();v.headline=p["headline"].as<String>();v.description=p["description"].as<String>();v.instruction=p["instruction"].as<String>();v.category=p["category"]|uint8_t(0);v.rank=p["rank"]|0;v.expires=p["expires"]|time_t(0);
  }
  s.forecast_fetched=d["forecast_fetched"]|time_t(0);s.alert_fetched=d["alert_fetched"]|time_t(0);
  s.forecast_valid=s.forecast_count>0;s.alerts_valid=false;s.alert_theme=s.alert_count?s.alerts[0].category:0;s.alerts_truncated=d["alerts_truncated"]|false;
  if(s.forecast_count)s.forecast_error="Cached / refresh pending";
  s.alert_error="Cached / unverified";
}
String alertSignature(const State& s){String out=String(s.alerts_truncated);for(unsigned i=0;i<s.alert_count;++i)out+=s.alerts[i].id+"|"+s.alerts[i].headline;return out;}

void worker(void*){
  Observation o{};bool cache_loaded=false;uint32_t cache_saved=0;bool cache_written=false;String alert_signature;
  startup_health::PortalProbe probe;
  while(!probe.finished()){
    if(probe.due(millis(),http_serving.load())){
      State tested=state();tested.portal_passed=portalSelftest(tested.portal_test_status);
      tested.portal_tested=true;probe.result(millis(),tested.portal_passed);
      tested.portal_attempts=probe.attempts();writeState(tested);
    }
    vTaskDelay(pdMS_TO_TICKS(25));
  }
  for(;;){
    xQueueReceive(observations,&o,pdMS_TO_TICKS(200));
    State s=state();
    if(!cache_loaded){if(config.location_set)loadCache(s);writeState(s);cache_loaded=true;alert_signature=alertSignature(s);}
    if(WiFi.status()!=WL_CONNECTED || !clockValid()){maintainAprs(s);writeState(s);vTaskDelay(pdMS_TO_TICKS(800));continue;}
    uint32_t now=millis();
    if(config.location_set){
      if(int32_t(now-next_alerts)>=0){alerts(s);alert_delay=s.alert_error=="Current"?30000:std::min(uint32_t(900000),alert_delay*2);s.alert_poll_ms=std::max(alert_delay,std::max(server_delay,retry_delay));next_alerts=millis()+s.alert_poll_ms;writeState(s);}
      if(int32_t(now-next_forecast)>=0){forecast(s);forecast_delay=s.forecast_error=="Current"?1800000:std::min(uint32_t(1800000),forecast_delay*2);next_forecast=millis()+std::max(forecast_delay,std::max(server_delay,retry_delay));writeState(s);}
    }
    if(config.location_set && s.sun_day!=localDay() && int32_t(millis()-next_sun)>=0){fetchSun(s);writeState(s);}
    if(config.location_set && (s.forecast_fetched || s.alert_fetched) && (!cache_written || millis()-cache_saved>900000 || alert_signature!=alertSignature(s))){saveCache(s);cache_written=true;cache_saved=millis();alert_signature=alertSignature(s);}
    if(config.location_set&&int32_t(millis()-next_pressure)>=0){next_pressure=millis()+60000;pressure(s);if(retry_delay>uint32_t(next_pressure-millis()))next_pressure=millis()+retry_delay;writeState(s);}
    maintainAprs(s);writeState(s); // Due NWS checks run before a reconnect/login.
    // Exporters remain opt-in. No replay or stale observations may leave the device.
    if(config.wu_enabled || config.weathercloud_enabled || config.aprs_enabled || config.pws_enabled || config.windy_enabled){
      const bool allowed=o.live&&o.fresh&&o.epoch_ms&&clockMs()>=o.epoch_ms&&clockMs()-o.epoch_ms<120000;
      if(!allowed){
        s.windy_state=config.windy_enabled?"Waiting for live radio data":"Off";
        s.pws_state=config.pws_enabled?"Waiting for live radio data":"Off";
        s.wu_state=config.wu_enabled?"Waiting for live radio data":"Off";
        s.weathercloud_state=config.weathercloud_enabled?"Waiting for live radio data":"Off";
        if(s.aprs_connected)s.aprs_state="Connected; waiting for fresh Atlas data";
      }else{
        // Refresh after slow NWS requests; never send the old queued observation.
        xQueueReceive(observations,&o,0);
        if(o.live && o.fresh && clockMs()-o.epoch_ms<120000 && config.wu_enabled && int32_t(millis()-next_wu)>=0){bool ok=uploadWu(o,s.wu_state);if(ok)s.wu_success=time(nullptr);wu_delay=ok?60000:std::min(uint32_t(1800000),wu_delay*2);next_wu=millis()+wu_delay;}
        xQueueReceive(observations,&o,0);
        if(o.live && o.fresh && clockMs()-o.epoch_ms<120000 && config.weathercloud_enabled && int32_t(millis()-next_cloud)>=0){bool ok=uploadCloud(o,s.weathercloud_state);if(ok)s.weathercloud_success=time(nullptr);cloud_delay=ok?600000:std::min(uint32_t(3600000),cloud_delay*2);next_cloud=millis()+cloud_delay;}
        xQueueReceive(observations,&o,0);
        if(o.live && o.fresh && config.windy_enabled && windy_schedule.due(millis()))uploadWindy(o,s);
        xQueueReceive(observations,&o,0);
        if(o.live && o.fresh && config.pws_enabled && int32_t(millis()-next_pws)>=0){bool ok=uploadPws(o,s.pws_state);if(ok)s.pws_success=time(nullptr);pws_delay=ok?300000:std::min(uint32_t(3600000),pws_delay*2);next_pws=millis()+pws_delay;}
        xQueueReceive(observations,&o,0);
        if(o.live && o.fresh && clockMs()-o.epoch_ms<120000 && config.aprs_enabled && int32_t(millis()-next_aprs)>=0){bool ok=uploadAprs(o,s.aprs_state);if(ok)s.aprs_success=time(nullptr);aprs_delay=ok?300000:std::min(uint32_t(3600000),aprs_delay*2);next_aprs=millis()+aprs_delay;}
      }
      if(s.aprs_connected&&!aprs_socket.connected())stopAprs(s,"Disconnected; reconnecting");
      writeState(s);
    }
  }
}
}
void startWorker(){xTaskCreatePinnedToCore(worker,"wx-network",24576,nullptr,1,nullptr,0);}
}

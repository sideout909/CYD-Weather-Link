#include "services.h"
#include "services_internal.h"
#include "setup_page.h"
#include "setup_assets.h"
#include "setup_credentials.h"
#include "sensor_health.h"
#include "firmware_updates.h"
#include "firmware_version.h"
#include "update_policy.h"
#include "archive_storage.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <cmath>
#include "aprs_weather.h"

namespace services {
namespace detail {
Settings config;
State current;
SemaphoreHandle_t state_mutex = nullptr;
QueueHandle_t observations = nullptr;
Observation latest;
std::atomic<bool> http_serving{false};
void writeState(const State& next) {
  if(xSemaphoreTake(state_mutex,pdMS_TO_TICKS(100))==pdTRUE){current=next;xSemaphoreGive(state_mutex);}
}
}
namespace {
WebServer server(80);
DNSServer dns;
Preferences prefs;
String csrf,ap_name;
uint32_t restart_at=0,last_link_check=0;
bool scan_started=false;
const char* headers[]={"X-CSRF-Token"};
String randomKey() {
  const char alphabet[]="23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
  String key;for(int i=0;i<12;++i)key+=alphabet[esp_random()&31];return key;
}
bool authorize() {
  server.sendHeader("Cache-Control","no-store");
  if(server.authenticate("admin",detail::config.setup_key.c_str()))return true;
  server.requestAuthentication(BASIC_AUTH,"Weather Link setup");return false;
}
void error(const char* message,int code=400) {
  StaticJsonDocument<192> d;d["error"]=message;String body;serializeJson(d,body);server.send(code,"application/json",body);
}
void encodeConfig(JsonDocument& d,bool secrets,const Settings& c=detail::config) {
  d["ssid"]=c.ssid;d["timezone"]=c.timezone;d["station_id"]=c.station_id;
  d["channel"]=String(c.channel);d["radio_enabled"]=c.radio_enabled;
  d["location_set"]=c.location_set;d["latitude"]=c.latitude;d["longitude"]=c.longitude;d["elevation_m"]=c.elevation_m;
  d["wu_enabled"]=c.wu_enabled;d["wu_id"]=c.wu_id;
  d["weathercloud_enabled"]=c.weathercloud_enabled;d["weathercloud_id"]=c.weathercloud_id;
  d["aprs_enabled"]=c.aprs_enabled;d["aprs_profile"]=c.aprs_profile;d["callsign"]=c.callsign;
  d["update_repo"]=c.update_repo;d["update_enabled"]=c.update_enabled;d["update_auto"]=c.update_auto;
  if(secrets){d["wifi_password"]=c.wifi_password;d["setup_key"]=c.setup_key;d["setup_wifi_key"]=c.setup_wifi_key;d["admin_custom"]=c.admin_custom;d["wu_key"]=c.wu_key;d["weathercloud_key"]=c.weathercloud_key;d["aprs_passcode"]=c.aprs_passcode;}
  else{d["wu_key_set"]=c.wu_key.length()>0;d["weathercloud_key_set"]=c.weathercloud_key.length()>0;d["aprs_passcode_set"]=c.aprs_passcode.length()>0;d["wifi_password_set"]=c.wifi_password.length()>0;d["csrf"]=csrf;}
}
void readConfig(JsonDocument& d,Settings& c,bool keep_empty) {
  c.ssid=d["ssid"]|c.ssid;c.timezone=d["timezone"]|c.timezone;
  c.station_id=d["station_id"]|c.station_id;
  String channel=d["channel"]|String(c.channel);c.channel=channel.length()?channel[0]:'A';
  c.radio_enabled=d["radio_enabled"]|c.radio_enabled;
  c.location_set=d["location_set"]|c.location_set;
  c.latitude=d["latitude"]|c.latitude;c.longitude=d["longitude"]|c.longitude;c.elevation_m=d["elevation_m"]|c.elevation_m;
  c.wu_enabled=d["wu_enabled"]|c.wu_enabled;c.wu_id=d["wu_id"]|c.wu_id;
  c.weathercloud_enabled=d["weathercloud_enabled"]|c.weathercloud_enabled;c.weathercloud_id=d["weathercloud_id"]|c.weathercloud_id;
  c.aprs_enabled=d["aprs_enabled"]|c.aprs_enabled;c.aprs_profile=d["aprs_profile"]|c.aprs_profile;c.callsign=d["callsign"]|c.callsign;c.callsign.toUpperCase();
  c.update_repo=d["update_repo"]|c.update_repo;c.update_repo.trim();c.update_enabled=d["update_enabled"]|c.update_enabled;c.update_auto=d["update_auto"]|c.update_auto;
  if(!c.update_repo.length())c.update_repo=firmware_version::repository;
  auto secret=[&](const char* name,String& value){if(d[name].is<const char*>()){String next=d[name].as<String>();if(!keep_empty || next.length())value=next;}};
  secret("wifi_password",c.wifi_password);secret("setup_key",c.setup_key);secret("wu_key",c.wu_key);secret("weathercloud_key",c.weathercloud_key);secret("aprs_passcode",c.aprs_passcode);
}
bool cleanId(const String& value) {for(char ch:value)if(!isalnum(ch)&&ch!='-'&&ch!='_')return false;return true;}
const char* validate(const Settings& c) {
  if(c.ssid.length()>32 || c.wifi_password.length()>63)return "Wi-Fi settings exceed supported lengths.";
  if(c.setup_key.length()<8 || c.setup_key.length()>63)return "Admin password must contain 8 to 63 characters.";
  if(c.station_id>1023 || (c.channel!='A'&&c.channel!='B'&&c.channel!='C'))return "Invalid Atlas sensor ID or channel.";
  const String zones[]={"EST5EDT,M3.2.0,M11.1.0","CST6CDT,M3.2.0,M11.1.0","MST7MDT,M3.2.0,M11.1.0","MST7","PST8PDT,M3.2.0,M11.1.0","UTC0"};
  bool zone=false;for(const auto& z:zones)zone|=c.timezone==z;if(!zone)return "Choose a supported timezone.";
  if(!std::isfinite(c.latitude)||!std::isfinite(c.longitude)||c.latitude < -90 || c.latitude>90 || c.longitude < -180 || c.longitude>180)return "Invalid coordinates.";
  if(!std::isfinite(c.elevation_m)||c.elevation_m < -500 || c.elevation_m>9000)return "Invalid ground elevation.";
  if(c.wu_id.length()>32 || !cleanId(c.wu_id)||c.weathercloud_id.length()>64 || !cleanId(c.weathercloud_id))return "Invalid sharing station ID.";
  if(c.wu_key.length()>100||c.weathercloud_key.length()>100)return "Upload key is too long.";
  if(c.wu_enabled && (!c.wu_id.length() || !c.wu_key.length()))return "Weather Underground needs a station ID and upload key.";
  if(c.weathercloud_enabled && (!c.weathercloud_id.length() || !c.weathercloud_key.length()))return "Weathercloud needs a WID and device key.";
  if(c.aprs_profile!="ham" && c.aprs_profile!="cwop")return "Invalid APRS connection profile.";
  if(c.callsign.length() && !aprs::identity(c.callsign.c_str()))return "Invalid APRS callsign or station ID.";
  if(c.aprs_enabled && (!c.location_set || !c.callsign.length()))return "APRS/CWOP needs a configured location and station identity.";
  if(c.aprs_enabled && c.aprs_profile=="ham" && !c.aprs_passcode.length())return "Ham APRS-IS needs its configured passcode.";
  if(c.aprs_passcode.length()>16)return "Invalid APRS passcode length.";
  if(c.update_repo.length()&&!update_policy::repository(c.update_repo.c_str()))return "GitHub repository must be owner/repository.";
  if(c.update_enabled&&!c.update_repo.length())return "Choose a public GitHub repository before enabling update checks.";
  if(c.update_auto&&!c.update_enabled)return "Enable update checks before automatic installation.";
  for(char ch:c.aprs_passcode)if(!isdigit(ch)&&ch!='-')return "Invalid APRS passcode.";
  return nullptr;
}
void configGet() {
  if(!authorize())return;DynamicJsonDocument d(4096);encodeConfig(d,false);
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void configPost() {
  if(!authorize())return;
  if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before saving.",403);return;}
  if(server.arg("plain").length()>4096){error("Settings request is too large.",413);return;}
  DynamicJsonDocument d(6144);
  if(deserializeJson(d,server.arg("plain")) || !d.is<JsonObject>()){error("Invalid settings JSON.");return;}
  if(d.containsKey("station_id") && (!d["station_id"].is<unsigned>() || d["station_id"].as<unsigned>()>1023)){error("Invalid Atlas sensor ID.");return;}
  const auto update=firmware_updates::state();
  if(update.busy||update.boot_pending){error(update.boot_pending?"Wait for updated firmware to pass startup checks before saving.":"Wait for the firmware update operation to finish.",409);return;}
  Settings next=detail::config;readConfig(d,next,true);
  if(const char* message=validate(next)){error(message);return;}
  if(d["setup_key"].is<const char*>())setup_credentials::chooseAdmin(next.setup_wifi_key,next.setup_key,next.admin_custom,String(d["setup_key"].as<const char*>()),randomKey);
  DynamicJsonDocument saved(4096);encodeConfig(saved,true,next);String body;serializeJson(saved,body);
  if(prefs.putString("settings",body)!=body.length()){error("Settings could not be saved.",500);return;}
  server.send(200,"application/json","{\"saved\":true,\"restarting\":true}");restart_at=millis()+1500;
}
void statusGet() {
  if(!authorize())return;
  State s=state();Observation o;
  if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(10))==pdTRUE){o=detail::latest;xSemaphoreGive(detail::state_mutex);}
  StaticJsonDocument<2048>d;
  d["wifi"]=WiFi.status()==WL_CONNECTED;d["ip"]=portalAddress();d["clock_valid"]=clockValid();
  d["source"]=o.live?"radio":(o.epoch_ms?"usb_replay":"none");d["radio_ready"]=o.radio_ready;
  d["sensor_status"]=sensor_health::label(sensor_health::State(o.sensor_health));
  if(o.sensor_packet_age_s>=0)d["sensor_packet_age_s"]=o.sensor_packet_age_s;
  d["aprs_connected"]=s.aprs_connected&&WiFi.status()==WL_CONNECTED;d["aprs_verified"]=s.aprs_verified&&s.aprs_connected&&WiFi.status()==WL_CONNECTED;
  d["packets"]=o.packets;d["dropped"]=o.dropped;d["free_heap"]=ESP.getFreeHeap();d["free_psram"]=ESP.getFreePsram();
  d["uptime_s"]=millis()/1000;d["portal_tested"]=s.portal_tested;d["portal_passed"]=s.portal_passed;
  d["portal_attempts"]=s.portal_attempts;d["portal_test_status"]=s.portal_test_status;
  if(o.epoch_ms && o.live && clockValid())d["packet_age_s"]=(uint64_t(time(nullptr))*1000>o.epoch_ms)?(uint64_t(time(nullptr))*1000-o.epoch_ms)/1000:0;
  if(o.temperature_valid)d["temperature_F"]=o.temperature_f;
  if(o.humidity_valid)d["humidity"]=o.humidity;
  if(o.gust_valid)d["gust_mph"]=o.gust_mph;
  if(o.rain_today_observed)d["rain_today_in"]=o.rain_today_in;
  d["rain_partial"]=o.rain_today_observed&&!o.rain_today_valid;
  if(o.rain_24h_observed)d["rain_24h_in"]=o.rain_24h_in;
  d["rain_24h_partial"]=o.rain_24h_observed&&!o.rain_24h_valid;
  d["nws_pressure_pa"]=s.pressure_pa;d["nws_pressure_observed"]=s.pressure_observed;d["nws_pressure_station"]=s.pressure_station;d["nws_pressure_sea_level"]=s.pressure_sea_level;d["nws_pressure_state"]=s.pressure_error;
  d["sunrise"]=s.sunrise;d["sunset"]=s.sunset;d["sun_day"]=s.sun_day;d["sun_state"]=s.sun_error;d["moon_phase"]=s.moon_phase;d["moon_illumination"]=s.moon_illumination;
  d["forecast_state"]=s.forecast_error;d["alert_state"]=s.alert_error;d["alert_last_check"]=s.alert_fetched;d["alert_poll_s"]=s.alert_poll_ms/1000;
  if(s.forecast_count)d["forecast"]=s.forecast[0].name+": "+s.forecast[0].short_text;
  if(s.alert_count)d["alert"]=s.alerts[0].headline;
  d["wu_state"]=s.wu_state;d["weathercloud_state"]=s.weathercloud_state;d["aprs_state"]=s.aprs_state;
  d["wu_last_success"]=s.wu_success;d["weathercloud_last_success"]=s.weathercloud_success;d["aprs_last_success"]=s.aprs_success;
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void archiveStatusGet(){
  if(!authorize())return;
  const uint64_t now=clockValid()?uint64_t(time(nullptr))*1000:0;const auto s=archive_storage::state(now);
  StaticJsonDocument<1536>d;d["schema"]=1;d["ready"]=s.ready;d["clock_valid"]=bool(now);d["message"]=s.message;
  d["records"]=s.records;d["capacity"]=history_archive::kRecords;d["interval_seconds"]=300;d["retention_days"]=7;
  d["end_interval"]=archive_storage::endInterval(now);d["oldest_ms"]=s.oldest_ms;d["latest_ms"]=s.latest_ms;
  d["bytes"]=s.bytes;d["free_bytes"]=s.free_bytes;d["write_failures"]=s.write_failures;d["invalid_records"]=s.invalid_records;
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void archiveCsvGet(){
  if(!authorize())return;
  const uint32_t now=clockValid()?uint64_t(time(nullptr))*1000/history_archive::kIntervalMs:0;
  const auto digits=[](const String& s){if(!s.length()||s.length()>10)return false;for(char c:s)if(c<'0'||c>'9')return false;return true;};
  if(!now||!digits(server.arg("end"))||!digits(server.arg("offset"))){error("Archive download needs valid time and pagination.",400);return;}
  const uint64_t end=strtoull(server.arg("end").c_str(),nullptr,10),offset=strtoull(server.arg("offset").c_str(),nullptr,10);
  if(!end||end>now||now-end>=history_archive::kRecords||offset>=history_archive::kRecords){error("Invalid archive download window.",400);return;}
  if(!archive_storage::state(uint64_t(now)*history_archive::kIntervalMs).ready){error("Archive storage unavailable.",503);return;}
  String csv=(offset==0?String(archive_storage::csvHeader()):String(""))+archive_storage::csvPage(end,offset);
  server.sendHeader("X-Content-Type-Options","nosniff");server.send(200,"text/csv; charset=utf-8",csv);
}
}
const Settings& settings(){return detail::config;}
State state(){State copy;if(detail::state_mutex && xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(10))==pdTRUE){copy=detail::current;xSemaphoreGive(detail::state_mutex);}return copy;}
bool clockValid(){return time(nullptr)>1700000000;}
String setupWifiKey(){return detail::config.setup_wifi_key;}
String setupSsid(){return ap_name;}
String portalAddress(){return WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString();}
void publish(const Observation& sample){
  if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(2))==pdTRUE){detail::latest=sample;xSemaphoreGive(detail::state_mutex);}
  if(detail::observations)xQueueOverwrite(detail::observations,&sample);
}
void begin(){
  detail::state_mutex=xSemaphoreCreateMutex();detail::observations=xQueueCreate(1,sizeof(Observation));
  prefs.begin("wxlink",false);String saved=prefs.getString("settings","");
  if(saved.length()){DynamicJsonDocument d(6144);if(!deserializeJson(d,saved)){readConfig(d,detail::config,false);detail::config.setup_wifi_key=d["setup_wifi_key"]|String("");detail::config.admin_custom=d["admin_custom"]|true;}}
  if(setup_credentials::ensure(detail::config.setup_wifi_key,detail::config.setup_key,detail::config.admin_custom,randomKey)){
    DynamicJsonDocument d(4096);encodeConfig(d,true);String body;serializeJson(d,body);prefs.putString("settings",body);
  }
  char token[33];snprintf(token,sizeof(token),"%08x%08x%08x%08x",esp_random(),esp_random(),esp_random(),esp_random());csrf=token;
  const uint64_t mac=ESP.getEfuseMac();char ssid[24];snprintf(ssid,sizeof(ssid),"WXLink-%06X",unsigned((mac>>24)&0xffffff));ap_name=ssid;
  WiFi.persistent(false);WiFi.mode(WIFI_AP_STA);WiFi.setAutoReconnect(true);
  WiFi.softAP(ap_name.c_str(),detail::config.setup_wifi_key.c_str());
  dns.start(53,"*",WiFi.softAPIP());
  if(detail::config.ssid.length())WiFi.begin(detail::config.ssid.c_str(),detail::config.wifi_password.c_str());
  configTzTime(detail::config.timezone.c_str(),"time.cloudflare.com","pool.ntp.org");
  server.collectHeaders(headers,1);
  server.on("/",HTTP_GET,[]{if(authorize()){server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/html; charset=utf-8",kSetupPage);}});
  server.on("/assets/location.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::location_js),sizeof(setup_assets::location_js));}});
  server.on("/assets/leaflet-1.9.4.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::leaflet_js),sizeof(setup_assets::leaflet_js));}});
  server.on("/assets/leaflet-1.9.4.css",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/css",reinterpret_cast<const char*>(setup_assets::leaflet_css),sizeof(setup_assets::leaflet_css));}});
  server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configPost);server.on("/api/status",HTTP_GET,statusGet);
  server.on("/api/archive",HTTP_GET,archiveStatusGet);server.on("/api/archive.csv",HTTP_GET,archiveCsvGet);
  server.on("/api/update",HTTP_GET,[]{if(!authorize())return;auto s=firmware_updates::state();StaticJsonDocument<1024>d;d["current_version"]=firmware_version::current;d["available_version"]=s.available_version;d["message"]=s.message;d["busy"]=s.busy;d["available"]=s.available;d["progress"]=s.progress;d["boot_pending"]=s.boot_pending;d["last_checked"]=s.last_checked;String body;serializeJson(d,body);server.send(200,"application/json",body);});
  server.on("/api/update/check",HTTP_POST,[]{if(!authorize())return;if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before checking.",403);return;}if(!firmware_updates::requestCheck()){error("An update operation is already pending.",409);return;}server.send(202,"application/json","{\"queued\":true}");});
  server.on("/api/update/install",HTTP_POST,[]{if(!authorize())return;if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before installing.",403);return;}if(!firmware_updates::requestInstall()){error("Check for a compatible update first, or wait for the current operation.",409);return;}server.send(202,"application/json","{\"queued\":true}");});
  server.on("/api/scan",HTTP_GET,[]{
    if(!authorize())return;int count=WiFi.scanComplete();
    if(count==WIFI_SCAN_FAILED && !scan_started){WiFi.scanNetworks(true);scan_started=true;count=WIFI_SCAN_RUNNING;}
    StaticJsonDocument<2048>d;d["scanning"]=count==WIFI_SCAN_RUNNING;auto list=d.createNestedArray("networks");
    if(count>=0){for(int i=0;i<count && i<20;++i)list.add(WiFi.SSID(i));}
    String body;serializeJson(d,body);server.send(200,"application/json",body);
  });
  server.onNotFound([]{server.sendHeader("Location","http://"+WiFi.softAPIP().toString()+"/",true);server.send(302,"text/plain","");});
  server.begin();detail::startWorker();firmware_updates::startWorker();
}
void loop(){
  dns.processNextRequest();server.handleClient();
  detail::http_serving.store(true);
  if(restart_at && int32_t(millis()-restart_at)>=0){archive_storage::flush();ESP.restart();}
  if(millis()-last_link_check>1000){last_link_check=millis();
    if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(2))==pdTRUE){auto& s=detail::current;s.wifi=WiFi.status()==WL_CONNECTED;s.clock_valid=clockValid();s.ap=true;s.ap_name=ap_name;s.ip=portalAddress();xSemaphoreGive(detail::state_mutex);}
  }
}
}

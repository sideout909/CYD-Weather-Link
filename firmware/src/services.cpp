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
#include "nws_pressure.h"
#include "theme_icons.h"
#include "display_themes.h"
#include "lunar_phase.h"
#include "web_history.h"
#include "radio_receiver.h"
#include "display_preferences.h"
#include "factory_reset.h"
#include "weather_units.h"
#include "local_hostname.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <sys/time.h>
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
lux_cloud::Tracker cloud_tracker;
uint32_t restart_at=0,last_link_check=0,factory_restart_at=0;
bool scan_started=false,mdns_started=false;
uint32_t last_mdns_attempt=0;
void startLocalName(){
  last_mdns_attempt=millis();
  if(!MDNS.begin(detail::config.hostname.c_str())){MDNS.end();return;}
  MDNS.setInstanceName("WX-Link");
  if(!MDNS.addService("http","tcp",80)){MDNS.end();return;}
  mdns_started=true;
}
station_discovery::Scan discovery;
const weather::Store* history_source=nullptr;
uint64_t (*history_now)()=nullptr;
const char* headers[]={"X-CSRF-Token","Origin"};
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
  d["lux_cloud_enabled"]=c.lux_cloud_enabled;d["lux_reference"]=c.lux_reference;
  d["hostname"]=c.hostname;d["units"]=c.units;
  d["ssid"]=c.ssid;d["timezone"]=c.timezone;d["station_id"]=c.station_id;
  d["display_theme"]=c.display_theme;d["date_format"]=c.date_format;d["clock_24h"]=c.clock_24h;
  d["display_sleep_mode"]=c.display_sleep_mode;d["display_timeout_s"]=c.display_timeout_s;
  d["channel"]=String(c.channel);d["radio_enabled"]=c.radio_enabled;
  d["station_confirmed"]=c.station_confirmed;
  d["location_set"]=c.location_set;d["latitude"]=c.latitude;d["longitude"]=c.longitude;d["elevation_m"]=c.elevation_m;
  d["windy_enabled"]=c.windy_enabled;d["windy_id"]=c.windy_id;
  d["pws_enabled"]=c.pws_enabled;d["pws_id"]=c.pws_id;
  d["wu_enabled"]=c.wu_enabled;d["wu_id"]=c.wu_id;
  d["weathercloud_enabled"]=c.weathercloud_enabled;d["weathercloud_id"]=c.weathercloud_id;
  d["aprs_enabled"]=c.aprs_enabled;d["aprs_profile"]=c.aprs_profile;d["callsign"]=c.callsign;
  d["update_channel"]=c.update_channel;d["update_repo"]=c.update_repo;d["update_enabled"]=c.update_enabled;d["update_auto"]=c.update_auto;
  if(secrets){d["windy_password"]=c.windy_password;d["pws_key"]=c.pws_key;d["wifi_password"]=c.wifi_password;d["setup_key"]=c.setup_key;d["setup_wifi_key"]=c.setup_wifi_key;d["admin_custom"]=c.admin_custom;d["wu_key"]=c.wu_key;d["weathercloud_key"]=c.weathercloud_key;d["aprs_passcode"]=c.aprs_passcode;}
  else{d["windy_password_set"]=c.windy_password.length()>0;d["pws_key_set"]=c.pws_key.length()>0;d["wu_key_set"]=c.wu_key.length()>0;d["weathercloud_key_set"]=c.weathercloud_key.length()>0;d["aprs_passcode_set"]=c.aprs_passcode.length()>0;d["wifi_password_set"]=c.wifi_password.length()>0;d["csrf"]=csrf;}
}
void readConfig(JsonDocument& d,Settings& c,bool keep_empty) {
  c.lux_cloud_enabled=d["lux_cloud_enabled"]|c.lux_cloud_enabled;c.lux_reference=d["lux_reference"]|c.lux_reference;
  c.hostname=d["hostname"]|c.hostname;
  c.units=d["units"]|c.units;
  c.ssid=d["ssid"]|c.ssid;c.timezone=d["timezone"]|c.timezone;
  c.display_theme=d["display_theme"]|c.display_theme;c.date_format=d["date_format"]|c.date_format;c.clock_24h=d["clock_24h"]|c.clock_24h;
  c.display_sleep_mode=d["display_sleep_mode"]|c.display_sleep_mode;c.display_timeout_s=d["display_timeout_s"]|c.display_timeout_s;
  c.station_id=d["station_id"]|c.station_id;
  String channel=d["channel"]|String(c.channel);c.channel=channel.length()?channel[0]:'A';
  c.radio_enabled=d["radio_enabled"]|c.radio_enabled;
  c.location_set=d["location_set"]|c.location_set;
  c.latitude=d["latitude"]|c.latitude;c.longitude=d["longitude"]|c.longitude;c.elevation_m=d["elevation_m"]|c.elevation_m;
  c.windy_enabled=d["windy_enabled"]|c.windy_enabled;c.windy_id=d["windy_id"]|c.windy_id;
  c.pws_enabled=d["pws_enabled"]|c.pws_enabled;c.pws_id=d["pws_id"]|c.pws_id;
  c.wu_enabled=d["wu_enabled"]|c.wu_enabled;c.wu_id=d["wu_id"]|c.wu_id;
  c.weathercloud_enabled=d["weathercloud_enabled"]|c.weathercloud_enabled;c.weathercloud_id=d["weathercloud_id"]|c.weathercloud_id;
  c.aprs_enabled=d["aprs_enabled"]|c.aprs_enabled;c.aprs_profile=d["aprs_profile"]|c.aprs_profile;c.callsign=d["callsign"]|c.callsign;c.callsign.toUpperCase();
  c.update_channel=d["update_channel"]|c.update_channel;
  c.update_repo=d["update_repo"]|c.update_repo;c.update_repo.trim();c.update_enabled=d["update_enabled"]|c.update_enabled;c.update_auto=d["update_auto"]|c.update_auto;
  c.update_repo=update_policy::migratedProjectRepository(c.update_repo.c_str(),firmware_version::repository).c_str();
  auto secret=[&](const char* name,String& value){if(d[name].is<const char*>()){String next=d[name].as<String>();if(!keep_empty || next.length())value=next;}};
  secret("windy_password",c.windy_password);secret("pws_key",c.pws_key);secret("wifi_password",c.wifi_password);secret("setup_key",c.setup_key);secret("wu_key",c.wu_key);secret("weathercloud_key",c.weathercloud_key);secret("aprs_passcode",c.aprs_passcode);
}
bool cleanId(const String& value) {for(char ch:value)if(!isalnum(ch)&&ch!='-'&&ch!='_')return false;return true;}
const char* validate(const Settings& c) {
  if(!update_policy::channel(c.update_channel.c_str()))return "Choose Stable or Dev updates.";
  if(!std::isfinite(c.lux_reference)||c.lux_reference<0||c.lux_reference>200000)return "Invalid clear-sky calibration.";
  if(!weather_units::valid(c.units.c_str()))return "Choose metric or imperial units.";
  if(!display_themes::find(c.display_theme.c_str()))return "Choose a supported color theme.";
  if(!display_preferences::validDate(c.date_format.c_str()))return "Choose a supported date format.";
  if(!display_preferences::validSleep(c.display_sleep_mode.c_str())||!display_preferences::validTimeout(c.display_timeout_s))return "Choose a supported display sleep mode and timeout.";
  if(c.ssid.length()>32 || c.wifi_password.length()>63)return "Wi-Fi settings exceed supported lengths.";
  if(c.setup_key.length()<8 || c.setup_key.length()>63)return "Admin password must contain 8 to 63 characters.";
  if(c.station_id>1023 || (c.channel!='A'&&c.channel!='B'&&c.channel!='C'))return "Invalid Atlas sensor ID or channel.";
  const String zones[]={"EST5EDT,M3.2.0,M11.1.0","CST6CDT,M3.2.0,M11.1.0","MST7MDT,M3.2.0,M11.1.0","MST7","PST8PDT,M3.2.0,M11.1.0","UTC0"};
  bool zone=false;for(const auto& z:zones)zone|=c.timezone==z;if(!zone)return "Choose a supported timezone.";
  if(!std::isfinite(c.latitude)||!std::isfinite(c.longitude)||c.latitude < -90 || c.latitude>90 || c.longitude < -180 || c.longitude>180)return "Invalid coordinates.";
  if(!std::isfinite(c.elevation_m)||c.elevation_m < -500 || c.elevation_m>9000)return "Invalid ground elevation.";
  if(!local_hostname::valid(c.hostname.c_str()))return "Hostname must be 1–32 lowercase letters, digits or interior hyphens; include a letter or hyphen.";
  if(c.windy_id.length()>30||!cleanId(c.windy_id)||c.windy_password.length()>100)return "Invalid Windy station ID or password length.";
  for(unsigned char ch:c.windy_password)if(ch<=32||ch>=127)return "Windy station password must contain printable characters without spaces.";
  if(c.windy_enabled&&(c.windy_id.isEmpty()||c.windy_password.isEmpty()))return "Windy needs a station ID and station password.";
  if(c.pws_id.length()>64||!cleanId(c.pws_id)||c.pws_key.length()>100)return "Invalid PWSWeather station ID or key length.";
  if(c.pws_enabled&&(c.pws_id.isEmpty()||c.pws_key.isEmpty()))return "PWSWeather needs a station ID and API key.";
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
  if(!authorize())return;DynamicJsonDocument d(6144);encodeConfig(d,false);
  if(d.overflowed()){error("Configuration response exceeds storage budget.",503);return;}
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void configPost() {
  if(!authorize())return;
  if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before saving.",403);return;}
  if(server.arg("plain").length()>4096){error("Settings request is too large.",413);return;}
  DynamicJsonDocument d(6144);
  if(deserializeJson(d,server.arg("plain")) || !d.is<JsonObject>()){error("Invalid settings JSON.");return;}
  if(d.containsKey("update_channel")&&!d["update_channel"].is<const char*>()){error("Invalid update channel.");return;}
  if(d.containsKey("station_id") && (!d["station_id"].is<unsigned>() || d["station_id"].as<unsigned>()>1023)){error("Invalid Atlas sensor ID.");return;}
  if(d.containsKey("display_timeout_s")&&(!d["display_timeout_s"].is<unsigned>()||!display_preferences::validTimeout(d["display_timeout_s"].as<unsigned>()))){error("Invalid display timeout.");return;}
  for(const char* key:{"lux_cloud_enabled","lux_calibrate"})if(d.containsKey(key)&&!d[key].is<bool>()){error("Invalid light setting.");return;}
  if(d.containsKey("lux_reference")&&!d["lux_reference"].is<float>()){error("Invalid light calibration.");return;}
  if(d.containsKey("hostname")&&!d["hostname"].is<const char*>()){error("Invalid hostname setting.");return;}
  if(d.containsKey("windy_enabled")&&!d["windy_enabled"].is<bool>()){error("Invalid Windy enable setting.");return;}
  for(const char* key:{"windy_id","windy_password"})if(d.containsKey(key)&&!d[key].is<const char*>()){error("Invalid Windy credentials.");return;}
  if(d.containsKey("pws_enabled")&&!d["pws_enabled"].is<bool>()){error("Invalid PWSWeather enable setting.");return;}
  for(const char* key:{"pws_id","pws_key"})if(d.containsKey(key)&&!d[key].is<const char*>()){error("Invalid PWSWeather credentials.");return;}
  if(d.containsKey("units")&&!d["units"].is<const char*>()){error("Invalid units setting.");return;}
  if(d.containsKey("clock_24h")&&!d["clock_24h"].is<bool>()){error("Invalid clock setting.");return;}
  if(d.containsKey("display_theme")&&(!d["display_theme"].is<const char*>()||!display_themes::find(d["display_theme"].as<const char*>()))){error("Invalid color theme.");return;}
  Settings next=detail::config;readConfig(d,next,true);
  if(d.containsKey("discovery_session")&&(!d["discovery_session"].is<uint32_t>())){error("Invalid discovery session.");return;}
  const uint32_t discovery_session=d["discovery_session"]|uint32_t(0);
  if(discovery_session){
    if(!d["discovery_candidate"].is<uint32_t>()){error("Choose a detected station.");return;}
    const auto* chosen=discovery.choose(discovery_session,d["discovery_candidate"].as<uint32_t>(),millis());
    if(!chosen){error("That detection expired or is unavailable. Find your station again.",409);return;}
    next.station_id=chosen->packet.id;next.channel=chosen->packet.channel;next.station_confirmed=true;
    if(!next.radio_enabled){error("Enable reception before using a detected station.");return;}
  }else if(d["manual_binding"]==true){next.station_confirmed=true;}
  else if(next.station_id!=detail::config.station_id||next.channel!=detail::config.channel){error("Find your station, or choose manual binding under Advanced.");return;}
  // Moving the station or changing the light sensor invalidates old calibration.
  if((next.latitude!=detail::config.latitude||next.longitude!=detail::config.longitude||
      next.station_id!=detail::config.station_id||next.channel!=detail::config.channel)&&!d.containsKey("lux_reference"))next.lux_reference=0;
  if(d["lux_calibrate"]==true){
    Observation live{};if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(20))==pdTRUE){live=detail::latest;xSemaphoreGive(detail::state_mutex);}
    const uint64_t now_ms=clockMs();
    const auto calibration=lux_cloud::calibratedReference(live.lux,lux_cloud::elevation(now_ms/1000,next.latitude,next.longitude));
    if(!next.location_set||!clockValid()||!live.live||!live.lux_valid||!live.lux_epoch_ms||
        now_ms<live.lux_epoch_ms||now_ms-live.lux_epoch_ms>=120000||!calibration||
        next.station_id!=detail::config.station_id||next.channel!=detail::config.channel||
        next.latitude!=detail::config.latitude||next.longitude!=detail::config.longitude){error("Calibration needs fresh light, an unchanged station/location and sun at least 15 degrees high. Confirm clear direct sunlight.");return;}
    next.lux_reference=calibration;
  }
  if(const char* message=validate(next)){error(message);return;}
  // Validate without side effects before the OTA save lock: startup probes must
  // reject malformed settings with 400 while a pending image remains protected.
  const auto update=firmware_updates::state();
  if(update.busy||update.boot_pending){error(update.boot_pending?"Wait for updated firmware to pass startup checks before saving.":"Wait for the firmware update operation to finish.",409);return;}
  if(d["setup_key"].is<const char*>())setup_credentials::chooseAdmin(next.setup_wifi_key,next.setup_key,next.admin_custom,String(d["setup_key"].as<const char*>()),randomKey);
  DynamicJsonDocument saved(6144);encodeConfig(saved,true,next);
  if(saved.overflowed()){error("Settings exceed storage budget.",413);return;}
  String body;serializeJson(saved,body);
  if(prefs.putString("settings",body)!=body.length()){error("Settings could not be saved.",500);return;}
  server.send(200,"application/json","{\"saved\":true,\"restarting\":true}");restart_at=millis()+1500;
}
void factoryResetPost(){
  if(!authorize())return;
  const String origin=server.header("Origin");
  if(origin.length()&&origin!="http://"+server.hostHeader()){
    error("Reset must be requested from this device's webpage.",403);return;
  }
  if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before resetting.",403);return;}
  if(server.arg("plain").length()>128){error("Reset request is too large.",413);return;}
  StaticJsonDocument<192> request;
  if(deserializeJson(request,server.arg("plain"))||!request.is<JsonObject>()||
      !request["confirmation"].is<const char*>()||
      !factory_reset::confirmed(request["confirmation"].as<const char*>())){
    error("Type RESET WX LINK to confirm removal of all settings and local history.");return;
  }
  if(!requestFactoryReset()){error("Reset unavailable: wait for updates/restart or check storage.",409);return;}
  server.send(202,"application/json","{\"resetting\":true,\"restarting\":true}");
}
void statusGet() {
  if(!authorize())return;
  State s=state();Observation o;
  if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(10))==pdTRUE){o=detail::latest;xSemaphoreGive(detail::state_mutex);}
  DynamicJsonDocument d(6144);
  d["wifi"]=WiFi.status()==WL_CONNECTED;d["ip"]=portalAddress();d["local_url"]=portalUrl();d["mdns_ready"]=mdns_started;d["clock_valid"]=clockValid();
  d["source"]=o.live?"radio":(o.epoch_ms?"usb_replay":"none");d["radio_ready"]=o.radio_ready;
  d["station_confirmed"]=detail::config.station_confirmed;
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
  d["callsign"]=detail::config.callsign;d["aprs_profile"]=detail::config.aprs_profile;
  auto display=d.createNestedObject("display");
  auto reading=[&](const char* name,uint8_t bit,float value){if(o.display_present&bit)display[name]=value;};
  reading("temperature_F",Temperature,o.temperature_f);reading("humidity",Humidity,o.humidity);
  reading("wind_mph",Wind,o.wind_mph);reading("direction_deg",Direction,o.direction_deg);
  if(o.display_present&Light)display["lux"]=o.lux;
  reading("uv",UV,o.uv);display["stale_fields"]=o.display_stale;
  if(o.gust_observed)display["gust_mph"]=o.gust_mph;
  display["gust_partial"]=o.gust_partial;
  display["rain_partial"]=o.display_rain_partial;
  if(o.rain_today_observed)d["rain_today_in"]=o.rain_today_in;
  d["rain_partial"]=o.rain_today_observed&&!o.rain_today_valid;
  if(o.rain_24h_observed)d["rain_24h_in"]=o.rain_24h_in;
  d["rain_24h_partial"]=o.rain_24h_observed&&!o.rain_24h_valid;
  const auto cloud=cloudEstimate();d["lux_cloud_valid"]=cloud.valid();d["lux_cloud_kind"]=uint8_t(cloud.kind);
  d["lux_cloud_state"]=cloud.reason;d["lux_cloud_label"]=lux_cloud::label(cloud.kind);d["lux_solar_elevation"]=cloud.altitude;
  d["lux_cloud_samples"]=cloud.samples;d["lux_cloud_span_s"]=cloud.candidate_age_s;d["lux_sampled_ms"]=cloud.sampled_ms;
  d["lux_clear_expected"]=cloud.expected;d["lux_clear_ratio"]=cloud.ratio;
  d["windy_http_code"]=s.windy_http_code;d["windy_tls_code"]=s.windy_tls_code;d["windy_last_attempt"]=s.windy_attempted;
  d["windy_dns_ip"]=s.windy_dns_ip;d["windy_dns_server"]=s.windy_dns_server;
  d["windy_state"]=s.windy_state;d["windy_last_success"]=s.windy_success;
  d["pws_state"]=s.pws_state;d["pws_last_success"]=s.pws_success;
  d["nws_pressure_attempt_station"]=s.pressure_attempt_station;d["nws_pressure_attempted"]=s.pressure_attempted;
  d["nws_pressure_attempt_observed"]=s.pressure_attempt_observed;d["nws_pressure_attempt_timestamp"]=s.pressure_attempt_timestamp;
  d["nws_pressure_pa"]=s.pressure_pa;d["nws_pressure_observed"]=s.pressure_observed;d["nws_pressure_station"]=s.pressure_station;d["nws_pressure_sea_level"]=s.pressure_sea_level;d["nws_pressure_state"]=s.pressure_error;d["nws_pressure_provider"]=s.pressure_provider;d["nws_pressure_altimeter"]=s.pressure_altimeter;d["nws_pressure_primary_state"]=s.pressure_primary_error;
  d["nws_pressure_valid"]=s.wifi&&clockValid()&&nws_pressure::fresh(s.pressure_observed,time(nullptr));
  d["sunrise"]=s.sunrise;d["sunset"]=s.sunset;d["sun_day"]=s.sun_day;d["sun_state"]=s.sun_error;d["moon_reference"]=lunar_phase::referenceNoon(s.sun_day);d["moon_phase"]=s.moon_phase;d["moon_illumination"]=s.moon_illumination;
  d["solar_mode"]=s.sun_status;
  d["forecast_state"]=s.forecast_error;d["forecast_fetched"]=s.forecast_fetched;d["hourly_state"]=s.hourly_error;d["hourly_fetched"]=s.hourly_fetched;d["alert_state"]=s.alert_error;d["alert_last_check"]=s.alert_fetched;d["alert_poll_s"]=s.alert_poll_ms/1000;
  if(s.forecast_count)d["forecast"]=s.forecast[0].name+": "+s.forecast[0].short_text;
  if(s.alert_count)d["alert"]=s.alerts[0].headline;
  d["wu_state"]=s.wu_state;d["weathercloud_state"]=s.weathercloud_state;d["aprs_state"]=s.aprs_state;
  d["wu_last_success"]=s.wu_success;d["weathercloud_last_success"]=s.weathercloud_success;d["aprs_last_success"]=s.aprs_success;
  if(d.overflowed()){error("Status response unavailable.",503);return;}
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void historyGet(){
  if(!authorize())return;
  if(!history_source||!history_now||!clockValid()){error("History waits for a valid station clock.",503);return;}
  const uint64_t now=history_now()/60000;
  auto number=[&](const char* name,uint64_t fallback,uint64_t& out){if(!server.hasArg(name)){out=fallback;return true;}String value=server.arg(name);if(!value.length()||value.length()>12)return false;out=0;for(char ch:value){if(ch<'0'||ch>'9')return false;out=out*10+ch-'0';}return true;};
  uint64_t end,offset,count;
  if(!number("end",now,end)||!number("offset",0,offset)||!number("count",120,count)||end>now||now-end>=1440||offset>=1440||count<1||count>120||end<1439){error("Invalid history page.");return;}
  DynamicJsonDocument d(web_history::kDocumentBytes);
  if(!web_history::encode(d,*history_source,end,unsigned(offset),unsigned(count))){error("History response unavailable.",503);return;}
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
bool pageOffset(unsigned limit,unsigned& value){
  value=0;if(!server.hasArg("offset"))return true;const String input=server.arg("offset");
  if(!input.length()||input.length()>3)return false;
  for(char ch:input){if(ch<'0'||ch>'9')return false;value=value*10+ch-'0';}return value<limit;
}
void forecastGet(){
  if(!authorize())return;const State s=state();const bool hourly=server.arg("type")=="hourly";
  if(server.hasArg("type")&&server.arg("type")!="daily"&&!hourly){error("Invalid forecast type.");return;}
  unsigned offset;
  const unsigned total=hourly?s.hourly_count:s.forecast_count;
  if(!pageOffset(hourly?24:14,offset)){error("Invalid forecast page.");return;}
  DynamicJsonDocument d(4096);d["total"]=total;d["next_offset"]=std::min(unsigned(offset+4),total);
  const auto fetched=hourly?s.hourly_fetched:s.forecast_fetched;const String& message=hourly?s.hourly_error:s.forecast_error;
  d["state"]=message.c_str();d["fetched"]=fetched;d["issued"]=hourly?s.hourly_issued:s.forecast_issued;
  d["fresh"]=s.wifi&&clockValid()&&fetched&&time(nullptr)>=fetched&&time(nullptr)-fetched<7200&&message=="Current";
  auto periods=d.createNestedArray("periods");
  for(unsigned i=unsigned(offset);i<std::min(unsigned(offset+4),total);++i){const auto& f=hourly?s.hourly[i]:s.forecast[i];auto p=periods.createNestedObject();p["name"]=f.name.c_str();p["short"]=f.short_text.c_str();p["detail"]=f.detail.c_str();p["wind"]=f.wind.c_str();if(f.temperature_valid)p["temperature_F"]=f.temperature;else p["temperature_F"]=nullptr;p["rain_probability"]=f.rain_probability;p["daytime"]=f.is_daytime;p["daytime_known"]=f.daytime_known;p["start"]=f.start;p["end"]=f.end;}
  if(d.overflowed()||measureJson(d)>24576){error("Forecast response exceeds supported size.",503);return;}
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
void alertsGet(){
  if(!authorize())return;const State s=state();unsigned offset;
  if(!pageOffset(8,offset)){error("Invalid alert page.");return;}
  uint32_t revision=2166136261u;for(unsigned i=0;i<s.alert_count;++i)for(const String* value:{&s.alerts[i].id,&s.alerts[i].headline})for(char ch:*value){revision^=uint8_t(ch);revision*=16777619u;}
  StaticJsonDocument<2048>d;d["total"]=s.alert_count;d["next_offset"]=std::min(unsigned(offset+1),unsigned(s.alert_count));d["state"]=s.alert_error.c_str();d["checked"]=s.alert_fetched;d["truncated"]=s.alerts_truncated;
  d["revision"]=revision;
  d["fresh"]=s.wifi&&clockValid()&&s.alerts_valid&&!s.alerts_truncated&&s.alert_fetched&&time(nullptr)>=s.alert_fetched&&time(nullptr)-s.alert_fetched<180;
  d["theme"]=s.alert_theme;auto alerts=d.createNestedArray("alerts");
  if(unsigned(offset)<s.alert_count){const auto& a=s.alerts[offset];auto v=alerts.createNestedObject();v["id"]=a.id.c_str();v["event"]=a.event.c_str();v["headline"]=a.headline.c_str();v["description"]=a.description.c_str();v["instruction"]=a.instruction.c_str();v["expires"]=a.expires;v["category"]=a.category;}
  if(d.overflowed()||measureJson(d)>24576){error("Alert response exceeds supported size.",503);return;}
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
bool discoveryMutation(){
  if(!authorize())return false;
  if(server.header("X-CSRF-Token")!=csrf){error("Refresh this page before finding your station.",403);return false;}
  const auto update=firmware_updates::state();
  if(update.busy||update.boot_pending||restart_at){error("Wait for the current save or update to finish.",409);return false;}
  return true;
}
void discoveryGet(){
  if(!authorize())return;
  const uint32_t now=millis();
  DynamicJsonDocument d(8192);
  d["session"]=discovery.session();d["active"]=discovery.active(now);d["expired"]=discovery.expired(now);
  d["elapsed_s"]=discovery.elapsed(now)/1000;d["duration_s"]=300;
  d["ready"]=detail::config.radio_enabled&&radio_receiver::ready();d["overflow"]=discovery.overflow();
  d["profile"]="433.92 MHz OOK";d["station_confirmed"]=detail::config.station_confirmed;
  auto candidates=d.createNestedArray("candidates");
  for(const auto& candidate:discovery.candidates()){
    if(!candidate.occupied)continue;const auto& p=candidate.packet;
    auto c=candidates.createNestedObject();c["token"]=candidate.token;c["name"]=station_discovery::name(p.model);
    c["id"]=p.id;char channel[]={p.channel,0};c["channel"]=channel;
    c["sightings"]=candidate.sightings;c["age_s"]=uint32_t(now-candidate.last_ms)/1000;
    c["suggested"]=candidate.suggested(now);c["selectable"]=candidate.selectable(now);
    c["supported"]=p.model==station_discovery::Model::Atlas;
    const auto fields=candidate.freshFields(now);c["fields"]=fields;
    if(fields&station_discovery::Temperature)c["temperature_F"]=p.temperature_f;
    if(fields&station_discovery::Humidity)c["humidity"]=p.humidity;
    if(fields&station_discovery::Wind)c["wind_mph"]=p.wind_mph;
  }
  if(d.overflowed()){error("Detection status unavailable.",503);return;}
  String body;serializeJson(d,body);server.send(200,"application/json",body);
}
}
const Settings& settings(){return detail::config;}
State state(){State copy;if(detail::state_mutex && xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(10))==pdTRUE){copy=detail::current;xSemaphoreGive(detail::state_mutex);}return copy;}
bool clockValid(){return time(nullptr)>1700000000;}
uint64_t clockMs(){timeval now{};gettimeofday(&now,nullptr);return uint64_t(now.tv_sec)*1000+now.tv_usec/1000;}
String setupWifiKey(){return detail::config.setup_wifi_key;}
String setupSsid(){return ap_name;}
String portalAddress(){return WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString();}
String portalUrl(){return local_hostname::url(detail::config.hostname.c_str()).c_str();}
bool localNameReady(){return mdns_started;}
lux_cloud::Estimate cloudEstimate(){return cloud_tracker.estimate;}
void publish(const Observation& sample){
  const auto& c=detail::config;
  // RF fields and snapshot share the anchored millisecond clock. Rounding wall
  // time down to whole seconds can falsely mark a new subsecond sample future.
  cloud_tracker.update(c.lux_cloud_enabled,c.lux_reference,c.location_set&&clockValid(),c.latitude,c.longitude,
    sample.live,sample.lux_valid,sample.lux_epoch_ms,sample.lux,sample.snapshot_ms);

  if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(2))==pdTRUE){detail::latest=sample;xSemaphoreGive(detail::state_mutex);}
  if(detail::observations)xQueueOverwrite(detail::observations,&sample);
}
void bindHistory(const weather::Store* store,uint64_t (*now)()){history_source=store;history_now=now;}
void observeDiscovery(const station_discovery::Packet& packet,uint32_t now){discovery.observe(packet,now);}
bool factoryResetPending(){return factory_restart_at!=0;}
bool requestFactoryReset(){
  const auto update=firmware_updates::state();
  if(factory_restart_at||restart_at||update.busy||update.boot_pending||update.restart_pending)return false;
  if(!factory_reset::arm())return false;
  factory_restart_at=millis()+1000;
  if(!factory_restart_at)factory_restart_at=1;
  return true;
}
void begin(){
  detail::state_mutex=xSemaphoreCreateMutex();detail::observations=xQueueCreate(1,sizeof(Observation));
  prefs.begin("wxlink",false);String saved=prefs.getString("settings","");
  if(saved.length()){DynamicJsonDocument d(6144);if(!deserializeJson(d,saved)){readConfig(d,detail::config,false);detail::config.station_confirmed=d["station_confirmed"]|d.containsKey("station_id");detail::config.setup_wifi_key=d["setup_wifi_key"]|String("");detail::config.admin_custom=d["admin_custom"]|true;}}
  if(!local_hostname::valid(detail::config.hostname.c_str()))detail::config.hostname="wxlink";
  if(setup_credentials::ensure(detail::config.setup_wifi_key,detail::config.setup_key,detail::config.admin_custom,randomKey)){
    DynamicJsonDocument d(6144);encodeConfig(d,true);String body;serializeJson(d,body);prefs.putString("settings",body);
  }
  char token[33];snprintf(token,sizeof(token),"%08x%08x%08x%08x",esp_random(),esp_random(),esp_random(),esp_random());csrf=token;
  const uint64_t mac=ESP.getEfuseMac();char ssid[24];snprintf(ssid,sizeof(ssid),"WXLink-%06X",unsigned((mac>>24)&0xffffff));ap_name=ssid;
  WiFi.persistent(false);WiFi.mode(WIFI_AP_STA);WiFi.setHostname(detail::config.hostname.c_str());WiFi.setAutoReconnect(true);
  WiFi.softAP(ap_name.c_str(),detail::config.setup_wifi_key.c_str());
  dns.start(53,"*",WiFi.softAPIP());
  if(detail::config.ssid.length())WiFi.begin(detail::config.ssid.c_str(),detail::config.wifi_password.c_str());
  configTzTime(detail::config.timezone.c_str(),"time.cloudflare.com","pool.ntp.org");
  server.collectHeaders(headers,2);
  server.on("/",HTTP_GET,[]{if(authorize()){server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/html; charset=utf-8",kSetupPage);}});
  server.on("/assets/location.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::location_js),sizeof(setup_assets::location_js));}});
  server.on("/assets/dashboard.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::dashboard_js),sizeof(setup_assets::dashboard_js));}});
  server.on("/assets/discovery.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::discovery_js),sizeof(setup_assets::discovery_js));}});
  server.on("/assets/weather.png",HTTP_GET,[]{
    if(!authorize())return;const String name=server.arg("name");
    const String theme=server.hasArg("theme")?server.arg("theme"):detail::config.display_theme;
    const auto* icon=theme_icons::find(theme.c_str(),name.c_str());
    if(icon){server.send_P(200,"image/png",reinterpret_cast<const char*>(icon->bytes),icon->size);return;}
    error("Unknown weather icon.",404);
  });
  server.on("/assets/leaflet-1.9.4.js",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/javascript",reinterpret_cast<const char*>(setup_assets::leaflet_js),sizeof(setup_assets::leaflet_js));}});
  server.on("/assets/leaflet-1.9.4.css",HTTP_GET,[]{if(authorize()){server.sendHeader("Content-Encoding","gzip");server.sendHeader("X-Content-Type-Options","nosniff");server.send_P(200,"text/css",reinterpret_cast<const char*>(setup_assets::leaflet_css),sizeof(setup_assets::leaflet_css));}});
  server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configPost);server.on("/api/status",HTTP_GET,statusGet);
  server.on("/api/archive",HTTP_GET,archiveStatusGet);server.on("/api/archive.csv",HTTP_GET,archiveCsvGet);
  server.on("/api/history",HTTP_GET,historyGet);server.on("/api/forecast",HTTP_GET,forecastGet);server.on("/api/alerts",HTTP_GET,alertsGet);
  server.on("/api/discovery",HTTP_GET,discoveryGet);
  server.on("/api/discovery/start",HTTP_POST,[]{
    if(!discoveryMutation())return;
    if(!detail::config.radio_enabled||!radio_receiver::ready()){error("Receiver unavailable. Enable reception, save, and check CC1101 wiring/power.",409);return;}
    if(discovery.active(millis())){error("A scan is already running. Cancel it before starting again.",409);return;}
    discovery.start(millis(),esp_random());server.send(202,"application/json","{\"started\":true}");
  });
  server.on("/api/discovery/cancel",HTTP_POST,[]{if(!discoveryMutation())return;discovery.cancel();server.send(200,"application/json","{\"cancelled\":true}");});
  server.on("/api/factory-reset",HTTP_POST,factoryResetPost);
  server.on("/api/update",HTTP_GET,[]{if(!authorize())return;auto s=firmware_updates::state();StaticJsonDocument<1024>d;d["current_version"]=firmware_version::current;d["channel"]=detail::config.update_channel;d["signature_required"]=true;d["available_version"]=s.available_version;d["message"]=s.message;d["busy"]=s.busy;d["available"]=s.available;d["progress"]=s.progress;d["boot_pending"]=s.boot_pending;d["last_checked"]=s.last_checked;String body;serializeJson(d,body);server.send(200,"application/json",body);});
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
  server.begin();startLocalName();detail::startWorker();firmware_updates::startWorker();
}
void loop(){
  if(factory_restart_at){
    if(int32_t(millis()-factory_restart_at)>=0)ESP.restart();
    return;
  }
  if(!mdns_started&&uint32_t(millis()-last_mdns_attempt)>=30000)startLocalName();
  dns.processNextRequest();server.handleClient();
  detail::http_serving.store(true);
  if(restart_at && int32_t(millis()-restart_at)>=0){archive_storage::flush();ESP.restart();}
  if(millis()-last_link_check>1000){last_link_check=millis();
    if(xSemaphoreTake(detail::state_mutex,pdMS_TO_TICKS(2))==pdTRUE){auto& s=detail::current;s.wifi=WiFi.status()==WL_CONNECTED;s.clock_valid=clockValid();s.ap=true;s.ap_name=ap_name;s.ip=portalAddress();xSemaphoreGive(detail::state_mutex);}
  }
}
}

#pragma once
#include <Arduino.h>
#include <array>
#include "firmware_version.h"
#include "station_discovery.h"
#include "lux_cloud.h"
namespace weather { class Store; }

namespace services {
struct Settings {
  String ssid, wifi_password, setup_key, setup_wifi_key;
  String hostname="wxlink";
  bool lux_cloud_enabled=false;
  float lux_reference=0;
  bool admin_custom=false;
  String timezone = "EST5EDT,M3.2.0,M11.1.0";
  String date_format="md",display_sleep_mode="never",display_theme="midnight",units="imperial";
  bool clock_24h=false;
  uint16_t display_timeout_s=300;
  uint16_t station_id = 679;
  char channel = 'A';
  bool station_confirmed=false;
  bool radio_enabled = true, location_set = false;
  double latitude = 0, longitude = 0;
  float elevation_m = 0;
  bool wu_enabled = false, weathercloud_enabled = false, aprs_enabled = false,pws_enabled=false,windy_enabled=false;
  String wu_id, wu_key, weathercloud_id, weathercloud_key,pws_id,pws_key,windy_id,windy_password;
  String aprs_profile = "ham", callsign, aprs_passcode;
  String update_repo=firmware_version::repository;
  bool update_enabled=false,update_auto=false;
};
struct Observation {
  uint64_t epoch_ms = 0, snapshot_ms=0;
  bool live = false, fresh = false;
  bool temperature_valid=false,humidity_valid=false,wind_valid=false,direction_valid=false,gust_valid=false;
  float temperature_f=0,wind_mph=0,direction_deg=0,gust_mph=0;
  uint16_t humidity=0;
  uint32_t lux=0;
  uint64_t lux_epoch_ms=0;
  bool rain_today_observed=false,rain_24h_observed=false;
  bool lux_valid=false,uv_valid=false,rain_today_valid=false,rain_hour_valid=false,rain_24h_valid=false;
  float uv=0,rain_today_in=0,rain_hour_in=0,rain_24h_in=0;
  bool radio_ready=false;
  uint32_t packets=0,dropped=0;
  uint8_t sensor_health=0;
  int32_t sensor_packet_age_s=-1;
  // Display presence/age is independent of strict exporter validity above.
  uint8_t display_present=0,display_stale=0;
  bool gust_observed=false,gust_partial=true;
  bool display_rain_partial=true;
};
enum DisplayField : uint8_t { Temperature=1,Humidity=2,Wind=4,Direction=8,Light=16,UV=32 };
struct ForecastPeriod {
  String name, short_text, detail, wind;
  int temperature=0, rain_probability=-1;
  bool temperature_valid=false,is_daytime=false,daytime_known=false;
  time_t start=0,end=0;
};
struct Alert {
  String id,event,headline,description,instruction;
  uint8_t category=0;
  int rank=0;
  time_t expires=0;
};
struct State {
  bool wifi=false,clock_valid=false,ap=false;
  bool portal_tested=false,portal_passed=false;
  unsigned portal_attempts=0;
  String portal_test_status="Waiting for HTTP service";
  String ip,ap_name;
  time_t sunrise=0,sunset=0,sun_fetched=0;
  int32_t sun_day=0;
  String sun_status="unknown",sun_error="Not configured",moon_phase;
  float moon_illumination=-1;
  float pressure_pa=0;
  time_t pressure_observed=0;
  bool pressure_sea_level=false,pressure_altimeter=false;
  String pressure_provider="NWS",pressure_primary_error;
  String pressure_station,pressure_error="Not configured",pressure_attempt_station,pressure_attempt_timestamp;
  time_t pressure_attempted=0,pressure_attempt_observed=0;
  String forecast_error="Not configured",hourly_error="Not configured",alert_error="Not configured";
  time_t forecast_fetched=0,hourly_fetched=0,forecast_issued=0,hourly_issued=0,alert_fetched=0;
  bool forecast_valid=false,alerts_valid=false,alerts_truncated=false;
  uint32_t alert_poll_ms=30000;
  std::array<ForecastPeriod,14> forecast;
  std::array<ForecastPeriod,24> hourly;
  uint8_t hourly_count=0;
  uint8_t forecast_count=0;
  std::array<Alert,8> alerts;
  uint8_t alert_count=0,alert_theme=0;
  String wu_state="Off",weathercloud_state="Off",aprs_state="Off",pws_state="Off",windy_state="Off";
  bool aprs_connected=false,aprs_verified=false;
  time_t wu_success=0,weathercloud_success=0,aprs_success=0,pws_success=0,windy_success=0;
};
void begin();
void loop();
const Settings& settings();
State state();
lux_cloud::Estimate cloudEstimate();
void publish(const Observation& sample);
// Called/read only on the application loop, never from the network worker.
void bindHistory(const weather::Store* store,uint64_t (*now)());
void observeDiscovery(const station_discovery::Packet& packet,uint32_t now);
String setupWifiKey();
String setupSsid();
String portalAddress();
String portalUrl();
bool localNameReady();
bool clockValid();
uint64_t clockMs();
bool requestFactoryReset();
bool factoryResetPending();
}  // namespace services

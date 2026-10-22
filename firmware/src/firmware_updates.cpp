#include "firmware_updates.h"
#include "firmware_version.h"
#include "update_policy.h"
#include "ota_signature.h"
#include "update_release_list.h"
#include "startup_health.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <mbedtls/sha256.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern const uint8_t root_bundle[] asm("_binary_certs_x509_crt_bundle_bin_start");
// Arduino otherwise confirms a pending image before our display/services run.
extern "C" bool verifyRollbackLater(){return true;}

namespace firmware_updates {
namespace {
SemaphoreHandle_t mutex=nullptr;
State current;
bool check_requested=false,install_requested=false;
bool worker_ready=false;
bool verify_boot=false;
std::atomic<bool> restart_needed{false};
uint32_t next_check=0,boot_started=0;
uint32_t next_boot_check=0;
String image_url,image_sha;
size_t image_size=0;
bool lock(){return mutex&&xSemaphoreTake(mutex,pdMS_TO_TICKS(30))==pdTRUE;}
void unlock(){xSemaphoreGive(mutex);}
void publish(const State& state){if(lock()){current=state;restart_needed.store(state.restart_pending);unlock();}}
int get(HTTPClient& http,WiFiClientSecure& tls,String url){
  tls.setCACertBundle(root_bundle);tls.setHandshakeTimeout(6);tls.setTimeout(8);
  http.useHTTP10(true);http.setConnectTimeout(4000);http.setTimeout(8000);
  const char* headers[]={"Location"};http.collectHeaders(headers,1);
  for(unsigned redirect=0;redirect<5;++redirect){
    if(!update_policy::downloadHost(url.c_str()))return -100;
    if(!http.begin(tls,url))return -101;
    http.addHeader("User-Agent",String("CYDWeatherLink/")+firmware_version::current);
    const int code=http.GET();
    if(code!=301&&code!=302&&code!=303&&code!=307&&code!=308)return code;
    const String location=http.header("Location");http.end();tls.stop();
    url=location.startsWith("/")?String("https://github.com")+location:location;
  }
  return -102;
}
bool developmentManifest(const services::Settings& config,State& state,String& manifest){
  WiFiClientSecure tls;HTTPClient http;
  const String endpoint=String("https://api.github.com/repos/")+config.update_repo+"/releases?per_page=10";
  const int code=get(http,tls,endpoint);
  if(code!=200){state.message=code==403||code==429?"GitHub rate limit; try later":"Dev release lookup failed (HTTP "+String(code)+")";http.end();return false;}
  if(http.getSize()>262144){state.message="Dev release list too large";http.end();return false;}
  std::string best;
  const bool parsed=update_release_list::newest(http.getStream(),best);http.end();
  if(!parsed){state.message="No supported release or invalid Dev release list";return false;}
  manifest=String("https://github.com/")+config.update_repo+"/releases/download/v"+best.c_str()+"/weather-link-panellan_sc05_x.json";
  return true;
}
bool check(const services::Settings& config,State& state){
  state.message="Checking signed release";state.available=false;state.progress=0;state.available_version="";publish(state);
  image_url="";image_sha="";image_size=0;state.last_checked=time(nullptr);
  if(!update_policy::channel(config.update_channel.c_str())){state.message="Invalid update channel";return false;}
  String manifest=String("https://github.com/")+config.update_repo+"/releases/latest/download/weather-link-panellan_sc05_x.json";
  if(config.update_channel=="dev"&&!developmentManifest(config,state,manifest))return false;
  WiFiClientSecure tls;HTTPClient http;
  const int code=get(http,tls,manifest);
  if(code!=200){state.message=code==404?"No compatible release published":"Release lookup failed (HTTP "+String(code)+")";http.end();return false;}
  if(http.getSize()>4096){state.message="Release manifest too large";http.end();return false;}
  DynamicJsonDocument doc(4096);const bool parsed=!deserializeJson(doc,http.getStream(),DeserializationOption::NestingLimit(4));http.end();
  const String version=doc["version"]|String("");
  const String url=doc["firmware_url"]|String(""),sha=doc["sha256"]|String("");
  const auto* target=esp_ota_get_next_update_partition(nullptr);
  const uint32_t size=doc["size"]|uint32_t(0);
  if(!parsed||!doc.is<JsonObject>()||doc["schema"]!=1||doc["signature_schema"]!=1||
     doc["board"]!=firmware_version::board||doc["partition_layout"]!=firmware_version::partition_layout||
     !doc["flash_bytes"].is<uint32_t>()||doc["flash_bytes"]!=ESP.getFlashChipSize()||
     !doc["size"].is<uint32_t>()||!target||size<32768||size>target->size||
     !update_policy::eligible(version.c_str(),config.update_channel.c_str())||
     !update_policy::firmwareUrl(url.c_str(),config.update_repo.c_str(),version.c_str())){
    state.message="Release is incompatible, unsigned or outside selected channel";return false;
  }
  ota_manifest::Manifest signed_manifest;
  signed_manifest.board=doc["board"].as<const char*>();signed_manifest.partition_layout=doc["partition_layout"].as<const char*>();
  signed_manifest.flash_bytes=doc["flash_bytes"].as<uint32_t>();signed_manifest.size=size;
  signed_manifest.version=version.c_str();signed_manifest.sha256=sha.c_str();signed_manifest.firmware_url=url.c_str();
  signed_manifest.channel=doc["channel"]|"";signed_manifest.key_id=doc["signing_key_id"]|"";
  if(doc["signature_algorithm"]!=ota_manifest::algorithm||
     !ota_signature::verify(signed_manifest,config.update_repo.c_str(),std::string(doc["signature"]|""))){
    state.message="Publisher signature missing, invalid or untrusted";return false;
  }
  state.available_version=version;
  if(!update_policy::newer(version.c_str(),firmware_version::current)){
    state.message=config.update_channel=="stable"&&update_policy::releaseCandidate(firmware_version::current)?
        "Stable selected; waiting for a newer stable release (no downgrade)":"Firmware is up to date";return true;
  }
  image_url=url;image_sha=sha;image_size=size;
  state.available=true;state.message="Trusted signed firmware available";return true;
}
bool install(State& state){
  if(!state.available||!image_size)return false;
  state.message="Downloading release";state.progress=0;publish(state);
  WiFiClientSecure tls;HTTPClient http;
  const int code=get(http,tls,image_url);
  if(code!=200||http.getSize()!=int(image_size)){state.message="Firmware download size/status mismatch";http.end();return false;}
  if(!Update.begin(image_size,U_FLASH)){state.message="Inactive firmware slot unavailable";http.end();return false;}
  mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);mbedtls_sha256_starts_ret(&hash,0);
  uint8_t buffer[1024];size_t received=0;bool ok=true;
  uint32_t last_data=millis(),started=millis();auto* stream=http.getStreamPtr();
  while(received<image_size){
    if(millis()-started>180000){ok=false;break;}
    const size_t available=stream->available();
    if(available){
      constexpr size_t probe=sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(uint32_t);
      const size_t count=std::max(received?size_t(1):probe,std::min(sizeof(buffer),std::min(available,image_size-received)));
      const int bytes=stream->readBytes(buffer,count);
      if(bytes<=0){ok=false;break;}
      uint32_t app_magic=0;if(!received&&bytes>=int(probe))memcpy(&app_magic,buffer+probe-sizeof(app_magic),sizeof(app_magic));
      if(!received&&(bytes<int(probe)||buffer[0]!=0xe9||reinterpret_cast<const esp_image_header_t*>(buffer)->chip_id!=ESP_CHIP_ID_ESP32S3||app_magic!=ESP_APP_DESC_MAGIC_WORD)){ok=false;break;}
      if(mbedtls_sha256_update_ret(&hash,buffer,bytes)||Update.write(buffer,bytes)!=size_t(bytes)){ok=false;break;}
      received+=bytes;last_data=millis();const uint8_t progress=received*100/image_size;
      if(progress!=state.progress){state.progress=progress;publish(state);}
    }else if(!http.connected()||millis()-last_data>10000||millis()-started>180000){ok=false;break;}
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  uint8_t digest[32];char actual[65];mbedtls_sha256_finish_ret(&hash,digest);mbedtls_sha256_free(&hash);http.end();
  for(unsigned i=0;i<32;++i)snprintf(actual+i*2,3,"%02x",digest[i]);
  if(!ok||received!=image_size||image_sha!=actual){Update.abort();state.message="Firmware incomplete or SHA-256 mismatch";return false;}
  if(!Update.end()){Update.abort();state.message="Firmware image validation failed";return false;}
  state.available=false;state.restart_pending=true;state.message="Verified; saving state and restarting";return true;
}
}
void begin(){
  mutex=xSemaphoreCreateMutex();boot_started=millis();
  esp_ota_img_states_t status;
  current.boot_pending=esp_ota_get_state_partition(esp_ota_get_running_partition(),&status)==ESP_OK&&status==ESP_OTA_IMG_PENDING_VERIFY;
  verify_boot=current.boot_pending;
  if(current.boot_pending)current.message="Verifying updated firmware";
}
State state(){State result;if(lock()){result=current;unlock();}return result;}
bool requestCheck(){if(!lock())return false;if(!worker_ready||current.busy||current.boot_pending){unlock();return false;}check_requested=true;current.busy=true;current.message="Check queued";unlock();return true;}
bool requestInstall(){if(!lock())return false;if(!worker_ready||current.busy||!current.available||current.boot_pending){unlock();return false;}install_requested=true;current.busy=true;current.message="Install queued";unlock();return true;}
void tick(const services::Settings& config){
  State s=state();if(s.boot_pending||s.restart_pending)return;
  bool check_now=false,install_now=false;
  if(lock()){check_now=check_requested;install_now=install_requested;check_requested=install_requested=false;unlock();}
  if(!update_policy::repository(config.update_repo.c_str())){if(check_now||install_now){s.busy=false;s.message="Set a public GitHub owner/repository first";publish(s);}return;}
  if(!check_now&&!install_now&&(!config.update_enabled||int32_t(millis()-next_check)<0))return;
  if(WiFi.status()!=WL_CONNECTED||!services::clockValid()){s.busy=false;s.message="Updates need Wi-Fi and valid time";next_check=millis()+60000;publish(s);return;}
  s.busy=true;publish(s);
  if(install_now)install(s);
  else{
    const bool checked=check(config,s);next_check=millis()+(checked?86400000:900000);
    if(checked&&s.available&&config.update_enabled&&config.update_auto){
      const auto weather=services::state();
      if(config.location_set&&(!weather.alerts_valid||weather.alert_theme>=2||time(nullptr)-weather.alert_fetched>=180)){s.message="Update available; auto install waits for verified clear NWS status";next_check=millis()+900000;}
      else install(s);
    }
  }
  s.busy=s.restart_pending;publish(s);
}
void confirmBoot(bool healthy,const char* reason){
  if(!verify_boot||millis()-boot_started<30000)return;
  if(int32_t(millis()-next_boot_check)<0)return;next_boot_check=millis()+1000;
  auto s=state();
  const auto action=startup_health::bootAction(millis()-boot_started,healthy);
  if(action==startup_health::BootAction::Confirm){
    if(esp_ota_mark_app_valid_cancel_rollback()==ESP_OK){verify_boot=false;s.boot_pending=false;s.message="Updated firmware passed startup checks";publish(s);}
    else{s.message="Startup confirmation write failed; rollback remains armed";publish(s);}
  }
  else{s.message=String("Startup checks: ")+reason;publish(s);}
  if(verify_boot&&millis()-boot_started>90000)esp_ota_mark_app_invalid_rollback_and_reboot();
}
bool restartPending(){return restart_needed.load();}
void startWorker(){
  worker_ready=xTaskCreatePinnedToCore([](void*){for(;;){tick(services::settings());vTaskDelay(pdMS_TO_TICKS(250));}},"wx-updates",16384,nullptr,1,nullptr,0)==pdPASS;
  if(!worker_ready){auto s=state();s.message="Update worker unavailable";publish(s);}
}
}

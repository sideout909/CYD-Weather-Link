#include <Arduino.h>
#include <ArduinoJson.h>
#include <PanelLan.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <new>
#include <time.h>
#include "weather_store.h"
#include "atlas_packet.h"
#include "forecast_icon.h"
#include "moon_icon.h"
#include "weather_icons.h"
#include "home_fonts.h"
#include "daily_rain.h"
#include "checkpoint.h"
#include "archive_storage.h"
#include "services.h"
#include "radio_receiver.h"
#include "sensor_health.h"
#include "firmware_updates.h"
#include "firmware_version.h"

// JSON commands, HTTP handlers and display formatting share the main task.
// Keep explicit stack headroom; the Arduino default is only 8 KiB.
SET_LOOP_TASK_STACK_SIZE(24 * 1024);

namespace {
constexpr const char* kVersion = firmware_version::current;
constexpr uint16_t rgb565(unsigned r,unsigned g,unsigned b){return ((r&248)<<8)|((g&252)<<3)|(b>>3);}
constexpr uint16_t kWhite=rgb565(240,245,251),kAmber=0xfd20;
uint16_t kBackground,kCard,kMuted,kCyan,kDivider,kGreen,kAlertCard,kAlertBorder;
int8_t theme_preview=-1;
void palette(unsigned theme){
  if(theme==3){kBackground=rgb565(152,31,50);kCard=rgb565(125,25,43);kMuted=rgb565(255,225,229);kCyan=rgb565(184,241,255);kDivider=rgb565(195,111,123);kGreen=rgb565(196,255,214);kAlertCard=rgb565(98,18,35);kAlertBorder=rgb565(209,131,144);}
  else if(theme==2){kBackground=rgb565(148,64,0);kCard=rgb565(121,52,0);kMuted=rgb565(255,229,207);kCyan=rgb565(184,241,255);kDivider=rgb565(188,121,80);kGreen=rgb565(196,255,214);kAlertCard=rgb565(98,42,0);kAlertBorder=rgb565(207,139,83);}
  else{kBackground=rgb565(8,15,27);kCard=rgb565(17,30,45);kMuted=rgb565(162,180,201);kCyan=rgb565(97,216,237);kDivider=rgb565(35,50,70);kGreen=rgb565(131,226,177);kAlertCard=rgb565(16,45,41);kAlertBorder=rgb565(36,74,64);}
}
PanelLan display(BOARD_SC05_X);
LGFX_Sprite canvas(&display);
weather::Bucket* history = nullptr;
weather::RainInterval* rain_history=nullptr;
weather::Store* store = nullptr;
bool display_ok = false, replay = false, dirty = true;
uint32_t rf_stream_until=0;
uint8_t page = 0, metric = 0, forecast_index=0, alert_index=0;
int8_t measurement_detail=-1;
bool forecast_hourly=false;
bool setup_open=false,alerts_open=false,clock_anchored=false,checkpoint_restored=false;
int detail_scroll=0;
uint64_t clock_epoch=0,clock_uptime=0,last_publish=0,last_checkpoint=0;
weather::DailyRain daily_rain;
sensor_health::Monitor sensor_monitor;
services::State network;
uint64_t source_time = 0, last_receive = 0, last_draw = 0, last_status = 0;
char line[1024]; size_t line_size = 0; bool discard_line = false;
uint32_t touch_events = 0, serial_errors = 0, request_id = 0;
uint32_t last_sample_request_id = 0;
unsigned last_sample_result = 0;
bool have_sample_reply = false;
uint64_t uptimeMs() { return uint64_t(esp_timer_get_time()) / 1000; }
uint64_t liveNow(){return clock_anchored ? clock_epoch+uptimeMs()-clock_uptime : uptimeMs()+1;}
uint64_t dataNow() { return replay && source_time ? source_time + uptimeMs() - last_receive : liveNow(); }
int32_t localDay(uint64_t now){time_t seconds=now/1000;struct tm t{};localtime_r(&seconds,&t);return (t.tm_year+1900)*10000+(t.tm_mon+1)*100+t.tm_mday;}
weather::DailyRain::Total todayRain(uint64_t now){return replay||clock_anchored?daily_rain.today(localDay(now),now):weather::DailyRain::Total{};}
sensor_health::State sensorState(){return sensor_monitor.state(replay||services::settings().radio_enabled,replay||radio_receiver::ready(),dataNow(),*store);}

void text(const String& value, int x, int y, uint16_t color = kWhite, const lgfx::IFont* font = &fonts::Font2) {
  canvas.setFont(font); canvas.setTextColor(color); canvas.drawString(value, x, y);
}
void card(int x, int y, int w, int h) { canvas.fillRoundRect(x,y,w,h,7,kCard); }
// The study uses DejaVu, baseline coordinates, and proportional spacing.
// Four-bit glyph coverage keeps those fonts readable at the native 320x240 size.
uint16_t nextCode(const char*& p){
  uint8_t c=uint8_t(*p++);if(c<128)return c;
  if((c&224)==192 && *p){uint16_t code=(c&31)<<6;return code|(uint8_t(*p++)&63);}
  if((c&240)==224 && p[0] && p[1]){uint16_t code=(c&15)<<12;code|=(uint8_t(*p++)&63)<<6;return code|(uint8_t(*p++)&63);}
  return '?';
}
const home_fonts::Glyph* homeGlyph(const home_fonts::Font& font,uint16_t code){
  for(unsigned i=0;i<font.count;++i)if(font.glyphs[i].code==code)return &font.glyphs[i];
  return nullptr;
}
int homeWidth(const String& value,const home_fonts::Font& font,int tracking=0){
  int pen=0;const char* p=value.c_str();while(*p){const auto* g=homeGlyph(font,nextCode(p));if(g)pen+=g->advance+tracking*64;}
  return (pen+32)/64;
}
void homeText(const String& value,int x,int baseline,uint16_t color,const home_fonts::Font& font,uint16_t background,int tracking=0){
  uint16_t shades[16];
  for(unsigned a=0;a<16;++a){
    const unsigned r=(((color>>11)&31)*a+((background>>11)&31)*(15-a)+7)/15;
    const unsigned g=(((color>>5)&63)*a+((background>>5)&63)*(15-a)+7)/15;
    const unsigned b=((color&31)*a+(background&31)*(15-a)+7)/15;
    shades[a]=(r<<11)|(g<<5)|b;
  }
  int pen=x*64;const char* p=value.c_str();
  while(*p){
    const auto* g=homeGlyph(font,nextCode(p));if(!g)continue;
    const int left=(pen+32)/64+g->left,top=baseline+g->top;
    for(unsigned y=0;y<g->height;++y)for(unsigned x=0;x<g->width;++x){
      unsigned i=y*g->width+x;uint8_t packed=font.pixels[g->offset+i/2],a=i%2?packed&15:packed>>4;
      if(a)canvas.drawPixel(left+x,top+y,shades[a]);
    }
    pen+=g->advance+tracking*64;
  }
}
void homeText(const String& value,int x,int baseline,uint16_t color,const home_fonts::Font& font,int tracking=0){homeText(value,x,baseline,color,font,kBackground,tracking);}
String homeFit(String value,int width,const home_fonts::Font& font){
  if(homeWidth(value,font)<=width)return value;
  while(value.length() && homeWidth(value+"...",font)>width){
    unsigned last=value.length()-1;while(last && (uint8_t(value[last])&192)==128)--last;value.remove(last);
  }
  return value+"...";
}
template <typename T> String value(const weather::Field<T>& f, unsigned digits = 0) {
  return f.valid ? String(f.value, digits) : String("--");
}
// Integer fields use a separate conversion; String(integer, n) means base n.
String integer(const weather::Field<uint16_t>& f) { return f.valid ? String(f.value) : String("--"); }
String luxText() { return store->lux.valid ? String(store->lux.value) : String("--"); }
String replayClock() {
  if (!replay && !clock_anchored) return "--:--";
  time_t seconds = time_t(dataNow()/1000);
  struct tm local{};localtime_r(&seconds,&local);
  char clock[16];strftime(clock,sizeof(clock),"%I:%M %p",&local);
  return String(clock[0]=='0' ? clock+1 : clock);
}
void sparkline() {
  if (!store->accepted) return;
  const uint64_t end=dataNow()/60000,begin=end>=59?end-59:0;
  float low=INFINITY,high=-INFINITY;unsigned count=0;
  for(uint64_t m=begin;m<=end;++m){
    const auto* b=store->bucket(m);
    if(b && b->temperature_count){const float v=b->temperature_sum/b->temperature_count;low=fminf(low,v);high=fmaxf(high,v);++count;}
  }
  if(count<2)return;
  int previous_x=0,previous_y=0;bool previous=false;
  for(uint64_t m=begin;m<=end;++m){
    const auto* b=store->bucket(m);
    if(!b || !b->temperature_count){previous=false;continue;}
    const float v=b->temperature_sum/b->temperature_count;
    const int x=162-int(end-m)*150/59,y=118-int((v-low)/(high>low?high-low:1)*10);
    if(previous)canvas.drawLine(previous_x,previous_y,x,y,kCyan);
    previous_x=x;previous_y=y;previous=true;
  }
}
bool forecastGraphic(){
  time_t now=time(nullptr);
  if(!network.wifi||!services::clockValid()||!network.hourly_fetched||now-network.hourly_fetched>=7200||network.hourly_error!="Current")return false;
  const services::ForecastPeriod* period=nullptr;
  for(unsigned i=0;i<network.hourly_count;++i){const auto& p=network.hourly[i];if(p.start<=now&&p.end>now){period=&p;break;}}
  if(!period)return false;
  bool daytime=period->is_daytime;
  const bool solar_current=network.sun_day==localDay(uint64_t(now)*1000);
  if(solar_current&&network.sun_status=="midnight_sun")daytime=true;
  else if(solar_current&&network.sun_status=="polar_night")daytime=false;
  else if(solar_current&&network.sunrise&&network.sunset)daytime=now>=network.sunrise&&now<network.sunset;
  else if(!period->daytime_known)return false;
  const auto kind=forecast_icon::classify(period->short_text.c_str());
  const uint8_t* png=weather_icons::unknown;size_t size=sizeof(weather_icons::unknown);
  auto moon=[&]{
    constexpr int cx=149,cy=77,r=11;
    float illumination=solar_current?network.moon_illumination:-1;
    const bool waning=network.moon_phase.startsWith("Waning")||network.moon_phase=="Last Quarter";
    if(illumination<0){canvas.drawCircle(cx,cy,r,kMuted);text("?",146,72,kMuted,&fonts::Font0);return;}
    moon_icon::draw(canvas,cx,cy,r,illumination,waning,services::settings().latitude<0);
  };
  using K=forecast_icon::Kind;
  switch(kind){
    case K::Clear:if(!daytime){moon();homeText("NWS",140,105,kMuted,home_fonts::small);return true;}png=weather_icons::sunny;size=sizeof(weather_icons::sunny);break;
    case K::PartlyCloudy:if(!daytime){moon();canvas.drawPng(weather_icons::cloudy,sizeof(weather_icons::cloudy),139,78,0,0,0,0,.5f,.5f);homeText("NWS",140,112,kMuted,home_fonts::small);return true;}png=weather_icons::partly_cloudy;size=sizeof(weather_icons::partly_cloudy);break;
    case K::Cloudy:png=weather_icons::cloudy;size=sizeof(weather_icons::cloudy);break;
    case K::Rain:png=weather_icons::rain;size=sizeof(weather_icons::rain);break;
    case K::Thunder:png=weather_icons::thunderstorm;size=sizeof(weather_icons::thunderstorm);break;
    case K::Snow:png=weather_icons::snow;size=sizeof(weather_icons::snow);break;
    case K::Fog:png=weather_icons::fog;size=sizeof(weather_icons::fog);break;
    case K::Wind:png=weather_icons::windy;size=sizeof(weather_icons::windy);break;
    default:break;
  }
  canvas.drawPng(png,size,132,61,0,0,0,0,.75f,.75f);homeText("NWS",140,108,kMuted,home_fonts::small);return true;
}
void renderHome() {
  using namespace home_fonts;
  const uint64_t now=dataNow();
  homeText("OUTDOOR °F",11,60,kMuted,label,1);
  const String degrees=value(store->temperature,1);
  const bool compact_temperature=homeWidth(degrees,temperature,-2)>123;
  homeText(degrees,8,104,store->temperature.fresh(now)?kWhite:kMuted,compact_temperature?temperature_compact:temperature,-2);
  if(store->temperature.valid && !store->temperature.fresh(now))homeText("STALE",122,107,kAmber,small);
  const bool nws_graphic=forecastGraphic();
  sparkline();
  const String light=store->lux.valid ? String(store->lux.value/1000.0f,1)+" klx" : "-- klx";
  homeText(homeFit(store->lux.valid&&!store->lux.fresh(now)?"STALE · "+light:light+(nws_graphic?" · NWS forecast":" · NWS --"),164,body),11,132,kMuted,body);
  canvas.drawFastVLine(180,55,77,kDivider);
  homeText("WIND",193,60,kMuted,label,1);
  constexpr int cx=209,cy=89,r=20;
  canvas.drawCircle(cx,cy,r,kDivider);homeText("N",206,72,kMuted,small);
  String bearing="--";
  if(store->direction.valid){
    constexpr float pi=3.14159265358979323846f;
    const float angle=store->direction.value*pi/180;
    const int dx=int(sinf(angle)*14),dy=int(-cosf(angle)*14);
    const uint16_t arrow=store->direction.fresh(now)?kCyan:kMuted;
    canvas.drawLine(cx-dx/2,cy-dy/2,cx+dx,cy+dy,arrow);
    canvas.drawLine(cx-dx/2+1,cy-dy/2,cx+dx+1,cy+dy,arrow);
    canvas.fillTriangle(cx+dx,cy+dy,cx+dx/2-dy/3,cy+dy/2+dx/3,cx+dx/2+dy/3,cy+dy/2-dx/3,arrow);
    const char* bearings[]={"N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"};
    bearing=store->direction.fresh(now)?bearings[int((store->direction.value+11.25f)/22.5f)%16]:"STALE";
  }
  const String speed=value(store->wind,1);
  homeText(speed,238,98,store->wind.fresh(now)?kWhite:kMuted,homeWidth(speed,wind)<=76?wind:reading);
  homeText(homeFit("mph · "+bearing,77,body),239,112,kMuted,body);
  const auto peak=store->gust(now);
  homeText(homeFit("Gust "+value(peak)+(store->accepted && !store->gustComplete(now)?"*":"")+" · 5 min",119,body),193,130,kMuted,body);
  if(store->wind.valid && !store->wind.fresh(now))homeText("STALE",273,121,kAmber,small);
  canvas.fillRoundRect(8,140,96,43,5,kCard);canvas.fillRoundRect(112,140,96,43,5,kCard);canvas.fillRoundRect(216,140,96,43,5,kCard);
  homeText(store->humidity.valid&&!store->humidity.fresh(now)?"HUMIDITY !":"HUMIDITY",15,153,kMuted,tile_label,kCard);
  const String humidity=integer(store->humidity);
  homeText(humidity,15,175,store->humidity.fresh(now)?kWhite:kMuted,reading,kCard);
  homeText("%",15+homeWidth(humidity,reading)+6,175,kMuted,unit,kCard);
  const auto rain=store->rainState();const bool rain_fresh=rain.baseline&&now>=rain.time&&now-rain.time<weather::kStaleMs;
  const auto rain_today=todayRain(now);
  homeText(!rain_today.valid?"RAIN TODAY":!rain_fresh?"RAIN TODAY !":rain_today.complete?"RAIN TODAY":"RAIN TODAY*",119,153,kMuted,tile_label,kCard);
  const String rain_amount=rain_today.valid?String(rain_today.tips/100.0f,2):"--";
  homeText(rain_amount,119,175,rain_fresh?kWhite:kMuted,reading,kCard);
  homeText("in",119+homeWidth(rain_amount,reading)+5,175,kMuted,unit,kCard);
  homeText("SEA LEVEL",223,153,kMuted,tile_label,kCard);
  homeText("--",223,175,kMuted,reading,kCard);homeText("Not fitted",251,175,kMuted,unit,kCard);
}

bool bucketValue(const weather::Bucket& b, float& out) {
  switch (metric) {
    case 0: if (b.temperature_count) { out=b.temperature_sum/b.temperature_count;return true;} break;
    case 1: if (b.wind_count) { out=b.wind_max;return true;} break;
    case 2: if (b.lux_count) { out=float(b.lux_sum)/b.lux_count;return true;} break;
    case 3: if (b.humidity_count) { out=float(b.humidity_sum)/b.humidity_count;return true;} break;
  }
  return false;
}
template<typename T> String fieldAge(const weather::Field<T>& field,uint64_t now,bool mark_stale=true){
  if(!field.valid)return "No reading";
  if(now<field.time_ms)return "Time unknown";
  const uint64_t seconds=(now-field.time_ms)/1000;
  return String(uint32_t(seconds<3600?seconds:seconds/60))+(seconds<3600?"s":" min")+(!mark_stale||field.fresh(now)?"":" · STALE");
}
void detailRow(const String& label,const String& reading,int baseline){
  homeText(label,12,baseline,kMuted,home_fonts::body);
  const String fitted=homeFit(reading,177,home_fonts::clock);
  homeText(fitted,308-homeWidth(fitted,home_fonts::clock),baseline,kWhite,home_fonts::clock);
}
void renderMeasurement(){
  using namespace home_fonts;
  const char* titles[]={"OUTDOOR","WIND","HUMIDITY","RAIN","PRESSURE","LINK STATUS"};
  homeText("‹ Back",12,26,kCyan,header);homeText(titles[measurement_detail],89,26,kWhite,header);
  if(replay)homeText("REPLAY",263,15,kAmber,home_fonts::status);
  canvas.drawFastHLine(10,44,300,kDivider);
  const uint64_t now=dataNow();
  if(measurement_detail==0){
    detailRow("Temperature",value(store->temperature,1)+" °F",66);
    detailRow("Updated",fieldAge(store->temperature,now),88);
    detailRow("Local light",store->lux.valid?String(store->lux.value)+" lux":"--",110);
    detailRow("Light updated",fieldAge(store->lux,now),132);
    detailRow("UV index",value(store->uv,1),154);
    detailRow("UV updated",fieldAge(store->uv,now),176);
  }else if(measurement_detail==1){
    const auto peak=store->gust(now);
    detailRow("Current",value(store->wind,1)+" mph",66);
    detailRow("Direction",!store->direction.valid?"--":value(store->direction)+"°"+(store->direction.fresh(now)?"":" · STALE"),88);
    detailRow("Gust · 5 min",value(peak,1)+" mph",110);
    detailRow("Peak age",fieldAge(peak,now,false),132);
    detailRow("Wind age",fieldAge(store->wind,now),154);
    detailRow("Coverage",!store->wind.valid?"No readings":store->gustComplete(now)?"Full 5 min":"Partial / gaps",176);
  }else if(measurement_detail==2){
    detailRow("Humidity",integer(store->humidity)+" %",66);
    detailRow("Updated",fieldAge(store->humidity,now),88);
    detailRow("Sensor",String(services::settings().station_id)+" / "+services::settings().channel,110);
    detailRow("Status",!store->humidity.valid?"No reading":store->humidity.fresh(now)?"Fresh":"STALE",132);
    detailRow("Source",replay?"Recorded USB replay":store->accepted?"Atlas radio":"Waiting for Atlas",154);
    homeText("Swipe to open humidity trends",12,176,kMuted,body);
  }else if(measurement_detail==3){
    const auto today=todayRain(now);
    const auto hour=store->rain_timeline.window(now,3600000),day=store->rain_timeline.window(now,weather::kRainDayMs);
    auto amount=[](uint32_t tips,bool valid,bool complete){return valid?String(tips/100.0f,2)+" in"+(complete?"":" *"):String("--");};
    detailRow("Today",amount(today.tips,today.valid,today.complete),66);
    detailRow("Last hour",amount(hour.tips,hour.valid,hour.complete),88);
    detailRow("Last 24 hours",amount(day.tips,day.valid,day.complete),110);
    const auto counter=store->rainState();
    const String age=!counter.baseline?"No reading":now<counter.time?"Time unknown":String(uint32_t((now-counter.time)/1000))+"s"+(now-counter.time>=weather::kStaleMs?" · STALE":"");
    detailRow("Counter age",age,132);
    detailRow("Today resets","00:00 local",154);
    homeText("* Partial or uncertain coverage",12,176,kMuted,body);
  }else if(measurement_detail==4){
    detailRow("Barometer","Not fitted",66);
    detailRow("Sea level","--",88);
    detailRow("Station pressure","--",110);
    homeText("The Atlas does not report pressure.",12,143,kMuted,body);
    homeText("An optional barometer is required.",12,163,kMuted,body);
  }else{
    const auto state=sensorState();
    detailRow("Atlas",sensor_health::label(state),66);
    detailRow("Last packet",!sensor_monitor.received?"None":now<sensor_monitor.last_rx?"Time unknown":String(uint32_t((now-sensor_monitor.last_rx)/1000))+"s ago",88);
    String missing;for(unsigned i=0;i<3;++i)if(!sensor_monitor.types[i]||now<sensor_monitor.types[i]||now-sensor_monitor.types[i]>=weather::kStaleMs){if(missing.length())missing+=" / ";missing+=String(37+i);}
    detailRow("Missing types",missing.length()?missing:"None",110);
    detailRow(services::settings().aprs_profile=="cwop"?"CWOP link":"APRS link",network.aprs_connected&&network.wifi?(network.aprs_verified?"Connected · verified":"Connected · CWOP"):!network.wifi&&services::settings().aprs_enabled?String("Wi-Fi unavailable"):network.aprs_state,132);
    detailRow("Last report",network.aprs_success&&services::clockValid()?String(uint32_t(std::max(time_t(0),time(nullptr)-network.aprs_success)))+"s ago":"None sent",154);
    homeText(state==sensor_health::State::DataStale?"Receiving old/incomplete data. Check Atlas power.":state==sensor_health::State::NoSignal?"No station packets. Check Atlas power / range.":"Speaker: muted. APRS send is not an ingestion ACK.",12,176,kMuted,home_fonts::status);
  }
}
void speakerIcon(int x,int y,bool enabled){
  const uint16_t color=enabled?kGreen:kMuted;
  canvas.fillRect(x,y+4,3,5,color);canvas.fillTriangle(x+2,y+4,x+6,y+1,x+6,y+11,color);
  if(enabled){canvas.drawLine(x+9,y+3,x+11,y+6,color);canvas.drawLine(x+11,y+6,x+9,y+9,color);}
  else{canvas.drawLine(x+8,y+3,x+13,y+9,color);canvas.drawLine(x+13,y+3,x+8,y+9,color);}
}
void renderTrends() {
  const char* titles[] = {"Temperature (F)","Wind peaks (mph)","Light (lux)","Humidity (%)"};
  text(titles[metric],12,50,kCyan);
  text("tap chart to change",170,53,kMuted,&fonts::Font0);
  if (!store->accepted) { text("Collecting history",60,104,kMuted,&fonts::Font4); return; }
  const uint64_t end = dataNow()/60000, begin = end>=1439 ? end-1439 : 0;
  float low=INFINITY,high=-INFINITY;unsigned count=0;
  for (uint64_t m=begin;m<=end;++m) {
    float v;const auto* b=store->bucket(m);
    if (b && bucketValue(*b,v)) { low=fminf(low,v);high=fmaxf(high,v);++count; }
  }
  if (!count) { text("No samples for this metric",30,105,kMuted);return; }
  text(String(low,1)+" to "+String(high,1),12,72,kMuted,&fonts::Font0);
  const float range = high>low ? high-low : 1;
  canvas.drawFastHLine(16,161,288,kMuted);
  // One vertical min/max mark per screen column; empty columns stay blank.
  for (int x=0;x<288;++x) {
    // Keep "now" at the right edge even before UTC sync, when the uptime
    // minute index is smaller than the displayed 24-hour window.
    const int64_t first=int64_t(end)+1-1440+int64_t(x)*1440/288;
    const int64_t stop=int64_t(end)+1-1440+int64_t(x+1)*1440/288;
    float minv=INFINITY,maxv=-INFINITY;
    for (int64_t m=std::max(int64_t(0),first);m<stop && uint64_t(m)<=end;++m) {
      float v;const auto* b=store->bucket(m);
      if (b && bucketValue(*b,v)) { minv=fminf(minv,v);maxv=fmaxf(maxv,v); }
    }
    if (std::isfinite(minv)) {
      const int top=156-int((maxv-low)/range*67), bottom=156-int((minv-low)/range*67);
      canvas.drawFastVLine(16+x,top,max(1,bottom-top+1),kCyan);
    }
  }
  text("24 hours ago",16,165,kMuted,&fonts::Font0);
  text("now",282,165,kMuted,&fonts::Font0);
}
void wrapped(const String& text_value,int x,int y,int width,int height,int scroll=0) {
  canvas.setClipRect(x,y,width,height);canvas.setFont(&fonts::Font2);canvas.setTextColor(kWhite);
  String line;int yy=y-scroll;
  for(unsigned i=0;i<text_value.length();++i){
    char ch=text_value[i];
    if(ch=='\n' || canvas.textWidth(line+ch)>width){canvas.drawString(line,x,yy);yy+=17;line="";if(ch=='\n')continue;}
    line+=ch;
  }
  if(line.length())canvas.drawString(line,x,yy);canvas.clearClipRect();
}
void renderForecast() {
  const unsigned count=forecast_hourly?network.hourly_count:network.forecast_count;
  const String forecast_error=forecast_hourly?network.hourly_error:network.forecast_error;
  text(forecast_hourly?"Hourly / tap for daily":"Daily / tap for hourly",12,47,kCyan,&fonts::Font0);
  if(!count){text("Local forecast",16,68,kWhite,&fonts::Font4);wrapped(forecast_error,16,100,288,50);text("Tap title for Wi-Fi / location setup",16,157,kMuted,&fonts::Font0);return;}
  if(forecast_index>=count)forecast_index=0;
  const auto& f=forecast_hourly?network.hourly[forecast_index]:network.forecast[forecast_index];
  String period_name=f.name;if(forecast_hourly){struct tm local{};localtime_r(&f.start,&local);char label[32];strftime(label,sizeof(label),"%a %I:%M %p",&local);period_name=label;}
  text(period_name.substring(0,23),12,62,kWhite);text(String(forecast_index+1)+"/"+String(count),276,62,kMuted,&fonts::Font0);
  text(f.temperature_valid?String(f.temperature)+" F":"-- F",12,82,kWhite,&fonts::Font4);
  text(f.rain_probability>=0?"Rain "+String(f.rain_probability)+"%":"Rain --",116,89,kMuted,&fonts::Font0);
  text(forecast_error=="Current" && network.wifi && services::clockValid() && time(nullptr)-(forecast_hourly?network.hourly_fetched:network.forecast_fetched)<7200?"NWS":"NWS / STALE",234,89,kAmber,&fonts::Font0);
  wrapped(f.short_text+". "+f.wind+". "+f.detail,12,109,296,58,detail_scroll);
  time_t issued=forecast_hourly?network.hourly_issued:network.forecast_issued;time_t checked=forecast_hourly?network.hourly_fetched:network.forecast_fetched;
  struct tm issue_local{};localtime_r(&issued,&issue_local);char issue_text[12];strftime(issue_text,sizeof(issue_text),"%a %H:%M",&issue_local);
  text(String("Issued ")+(issued?issue_text:"--")+" / checked "+String(max(0L,long(time(nullptr)-checked))/60)+"m ago",12,174,kMuted,&fonts::Font0);
}
void renderSetup(){
  canvas.fillScreen(kBackground);text("SETUP",12,9,kCyan,&fonts::Font4);
  text("Tap header to close",175,16,kMuted,&fonts::Font0);
  text("Join Wi-Fi:",12,47,kMuted);text(services::setupSsid(),12,68,kWhite,&fonts::Font4);
  text("Setup Wi-Fi key:",12,102,kMuted);text(services::setupWifiKey(),12,124,kWhite,&fonts::Font4);
  text("Open http://192.168.4.1",12,164,kCyan);text(services::settings().admin_custom?"Admin: custom password set":"Admin: use the Wi-Fi key",12,187,kMuted);
  text("LAN: http://"+services::portalAddress(),12,209,kMuted,&fonts::Font0);
  text("Sun data by Sunrise-Sunset.org",12,230,kMuted,&fonts::Font0);
}
void renderAlerts(){
  canvas.fillScreen(kBackground);text(network.alerts_valid && services::clockValid() && time(nullptr)-network.alert_fetched<180?"NWS ALERTS":"NWS / UNVERIFIED",12,9,kCyan);text("tap header to close",190,13,kMuted,&fonts::Font0);
  if(!network.alert_count){wrapped(network.alerts_valid && time(nullptr)-network.alert_fetched<180?"No active alerts. Updated by NWS.":"Alert status unverified. "+network.alert_error,12,55,296,150);return;}
  alert_index%=network.alert_count;const auto& a=network.alerts[alert_index];
  text(String(alert_index+1)+"/"+String(network.alert_count)+" "+a.event.substring(0,33),12,38,kAmber,&fonts::Font0);
  wrapped(a.headline+"\n\n"+a.description+"\n"+a.instruction,12,57,296,149,detail_scroll);
  text("tap bottom: next / drag: scroll",12,221,kMuted,&fonts::Font0);
}
void draw() {
  if (!display_ok) return;
  network=services::state();
  // Explicit USB replay previews affect this display snapshot only. They do
  // not alter the NWS service cache, alert membership, or published weather.
  if(replay && theme_preview>=0){
    network.alert_theme=theme_preview>=2?theme_preview:0;
    network.alert_count=theme_preview==3?2:theme_preview==2?1:0;
    network.alerts_valid=true;network.alert_fetched=time(nullptr);
    network.alerts[0].event=theme_preview==3?"Severe Thunderstorm Warning":"Severe Thunderstorm Watch";
  }
  palette(theme_preview>=0?theme_preview:network.alert_theme);
  if(setup_open || alerts_open){if(setup_open)renderSetup();else renderAlerts();canvas.pushSprite(0,0);dirty=false;last_draw=uptimeMs();return;}
  canvas.fillScreen(kBackground);
  using namespace home_fonts;
  if(measurement_detail>=0)renderMeasurement();else{
  const char* names[]={"WEATHER LINK","TRENDS","FORECAST"};
  homeText(theme_preview>=0?"THEME PREVIEW":names[page],10,15,kWhite,header,1);
  const String identity=replay?"REPLAY":services::settings().callsign;
  homeText(identity,195-homeWidth(identity,home_fonts::header)/2,15,replay?kAmber:kCyan,home_fonts::header);
  const String clock=replayClock();homeText(clock,310-homeWidth(clock,home_fonts::clock),15,kMuted,home_fonts::clock);
  const auto sensor=sensorState();
  canvas.fillCircle(13,33,3,replay?kAmber:sensor==sensor_health::State::Current?kGreen:sensor==sensor_health::State::DataStale||sensor==sensor_health::State::NoSignal?kAmber:kMuted);
  const String source=replay?"Atlas · recorded":String("Atlas · ")+sensor_health::label(sensor);
  homeText(homeFit(source,136,body),22,37,kMuted,body);
  homeText(radio_receiver::ready()?"RF +":"RF X",167,37,radio_receiver::ready()?kGreen:kMuted,home_fonts::status);
  homeText(network.wifi?"Wi-Fi +":"Wi-Fi X",196,37,network.wifi?kGreen:kMuted,home_fonts::status);
  speakerIcon(238,26,false); // No audio hardware/configuration is fitted yet.
  const bool linked=network.aprs_connected&&network.wifi;
  const uint16_t link_color=linked?kGreen:services::settings().aprs_enabled?kAmber:kMuted;
  const String link_label=String(services::settings().aprs_profile=="cwop"?"CWOP":"APRS")+(linked?" +":" X");
  canvas.drawRoundRect(268,26,43,15,3,linked?kGreen:services::settings().aprs_enabled?kAmber:kDivider);
  homeText(link_label,271,37,link_color,home_fonts::status);
  canvas.drawFastHLine(10,44,300,kDivider);
  if (page==0) renderHome(); else if (page==1) renderTrends(); else renderForecast();
  }
  // Dots occupy the gutter, leaving the concept's compact status header intact.
  for(int i=0;i<3;++i)canvas.fillCircle(153+i*7,186,1,i==page?kCyan:kDivider);
  const bool verified=network.alerts_valid && services::clockValid() && time(nullptr)-network.alert_fetched<180;
  const bool all_clear=verified&&!network.alert_count;
  const uint16_t alert_background=all_clear||network.alert_theme>=2?kAlertCard:kCard;
  const uint16_t alert_color=network.alert_theme==3?kMuted:network.alert_theme==2?kGreen:verified?kGreen:kMuted;
  canvas.fillRoundRect(8,190,304,44,5,alert_background);
  canvas.drawRoundRect(8,190,304,44,5,all_clear||network.alert_theme>=2?kAlertBorder:kDivider);
  String headline=!verified?"NWS · UNVERIFIED":network.alert_theme==3?"! NWS WARNING · "+String(network.alert_count>1?"1 OF ":"")+String(network.alert_count)+(network.alert_count>1?"":" ACTIVE"):network.alert_theme==2?"! NWS WATCH · "+String(network.alert_count)+" ACTIVE":network.alert_count?"NWS · "+String(network.alert_count)+" advisory":"NWS · No active alerts";
  const auto& headline_font=network.alert_theme==3?tile_label:network.alert_count?header:alert;
  homeText(homeFit(headline,278,headline_font),16,207,alert_color,headline_font,alert_background);
  const String check_age=network.alert_fetched&&services::clockValid()?String(max(0L,long(time(nullptr)-network.alert_fetched)))+"s":"--";
  if(network.alert_count){
    const auto& event_font=network.alert_theme==3?event_bold:event;
    homeText(homeFit(network.alerts[0].event,272,event_font),16,222,kWhite,event_font,alert_background);
    homeText(homeFit("Checked "+check_age+" ago · Tap for details",272,home_fonts::status),16,231,kMuted,home_fonts::status,alert_background);
  }else{
    const String subtitle=verified?"Checked "+check_age+" ago":network.alert_error=="Current"?"Last check "+check_age+" · status unknown":network.alert_error;
    homeText(homeFit(subtitle,269,body),16,223,kMuted,body,alert_background);
  }
  canvas.drawLine(295,210,300,215,kMuted);canvas.drawLine(300,215,295,220,kMuted);
  canvas.pushSprite(0,0);
  dirty=false;last_draw=uptimeMs();
}
void rfStatus(){
  const auto r=radio_receiver::diagnostics();StaticJsonDocument<1536> d;
  d["event"]="rf_status";d["request_id"]=request_id;d["version"]=kVersion;d["uptime_ms"]=millis();
  d["radio_ready"]=r.ready;d["receiver_enabled"]=r.enabled;d["source"]=replay?"usb_replay":"radio";
  d["current_rssi_dbm"]=r.current_rssi;d["average_rssi_dbm"]=r.average_rssi;d["threshold_dbm"]=r.threshold;
  d["decoded_signals"]=r.decoded_signals;d["decoded_messages"]=r.decoded_messages;d["zero_decodes"]=r.zero_decodes;
  d["capture_drops"]=r.capture_drops;d["decoder_drops"]=r.decoder_drops;d["app_queue_drops"]=radio_receiver::dropped();
  d["sensor_status"]=sensor_health::label(sensorState());d["accepted"]=store->accepted;d["repeats"]=store->duplicates;d["rejected"]=store->rejected;
  const uint64_t now=dataNow();JsonArray types=d.createNestedArray("type_ages_ms");
  for(auto stamp:sensor_monitor.types){if(stamp&&now>=stamp)types.add(now-stamp);else types.add(nullptr);}
  JsonObject ages=d.createNestedObject("field_ages_ms");
  auto age=[&](const char* key,uint64_t stamp,bool valid){if(valid&&now>=stamp)ages[key]=now-stamp;else ages[key]=nullptr;};
  age("temperature",store->temperature.time_ms,store->temperature.valid);age("humidity",store->humidity.time_ms,store->humidity.valid);
  age("wind",store->wind.time_ms,store->wind.valid);age("direction",store->direction.time_ms,store->direction.valid);
  age("lux",store->lux.time_ms,store->lux.valid);age("uv",store->uv.time_ms,store->uv.valid);
  const auto rain=store->rainState();age("rain",rain.time,rain.baseline);
  char buffer[1536];const size_t length=serializeJson(d,buffer,sizeof(buffer));Serial.write(reinterpret_cast<const uint8_t*>(buffer),length);Serial.println();
}
void status(const char* event = "status") {
  StaticJsonDocument<3072> out;
  out["event"]=event;out["request_id"]=request_id;out["version"]=kVersion;out["board"]="panellan_sc05_x";
  out["display_initialized"]=display_ok;out["width"]=display.width();out["height"]=display.height();
  out["flash_bytes"]=ESP.getFlashChipSize();out["psram_bytes"]=ESP.getPsramSize();
  out["free_heap"]=ESP.getFreeHeap();out["loop_stack_free"]=uxTaskGetStackHighWaterMark(nullptr);out["free_psram"]=ESP.getFreePsram();
  out["uptime_ms"]=uptimeMs();out["source"]=replay?"usb_replay":store->accepted?"radio":"none";
  out["radio_ready"]=radio_receiver::ready();out["radio_dropped"]=radio_receiver::dropped();
  out["wifi"]=network.wifi;out["clock_valid"]=services::clockValid();out["setup_ip"]=services::portalAddress();
  out["checkpoint_restored"]=checkpoint_restored;out["setup_open"]=setup_open;
  out["portal_tested"]=network.portal_tested;out["portal_passed"]=network.portal_passed;
  out["portal_attempts"]=network.portal_attempts;out["portal_test_status"]=network.portal_test_status;
  out["custom_admin_password"]=services::settings().admin_custom;
  out["setup_credentials_separated"]=services::settings().admin_custom && services::settings().setup_wifi_key!=services::settings().setup_key;
  out["forecast_periods"]=network.forecast_count;out["hourly_periods"]=network.hourly_count;
  out["sunrise"]=network.sunrise;out["sunset"]=network.sunset;out["moon_phase"]=network.moon_phase;out["moon_illumination"]=network.moon_illumination;out["sun_state"]=network.sun_error;
  out["rain_24h_in"]=store->rain_timeline.window(dataNow(),weather::kRainDayMs).tips/100.0f;
  const auto rain_today=todayRain(dataNow());
  if(rain_today.valid)out["rain_today_in"]=rain_today.tips/100.0f;else out["rain_today_in"]=nullptr;
  out["rain_today_complete"]=rain_today.complete;
  out["alerts_last_checked"]=network.alert_fetched;out["alerts_valid"]=network.alerts_valid;out["alert_poll_ms"]=network.alert_poll_ms;
  out["page"]=page;out["touch_events"]=touch_events;out["serial_errors"]=serial_errors;
  out["theme_preview"]=theme_preview;
  out["measurement_detail"]=measurement_detail;
  out["sensor_status"]=sensor_health::label(sensorState());out["bound_packets_heard"]=sensor_monitor.received;
  out["aprs_connected"]=network.aprs_connected&&network.wifi;
  out["accepted"]=store->accepted;out["duplicates"]=store->duplicates;out["rejected"]=store->rejected;
  const auto archive=archive_storage::state(clock_anchored?liveNow():0);
  out["archive_ready"]=archive.ready;out["archive_records"]=archive.records;out["archive_state"]=archive.message;
  out["archive_bytes"]=archive.bytes;out["filesystem_free_bytes"]=archive.free_bytes;out["archive_write_failures"]=archive.write_failures;
  out["rain_session_in"]=store->session_rain_tips/100.0;out["rain_discontinuities"]=store->rain_discontinuities;
  auto peak=store->gust(dataNow());if(peak.valid)out["gust_5m_mph"]=peak.value;
  if(store->temperature.valid)out["temperature_F"]=store->temperature.value;
  if(store->lux.valid)out["lux"]=store->lux.value;
  char buffer[3072];
  const size_t length=serializeJson(out,buffer,sizeof(buffer));
  Serial.write(reinterpret_cast<const uint8_t*>(buffer),length);Serial.println();
}
template <typename T> void load(JsonVariantConst v, weather::Field<T>& f) {
  if (v.is<T>()) {f.valid=true;f.value=v.as<T>();}
}
void resetData() {
  for (size_t i=0;i<weather::kHistoryBuckets;++i) history[i]=weather::Bucket{};
  for(size_t i=0;i<weather::kRainIntervals;++i)rain_history[i]=weather::RainInterval{};
  store->~Store(); new(store) weather::Store(history,services::settings().station_id,services::settings().channel,rain_history);
  daily_rain=weather::DailyRain{};have_sample_reply=false;
  sensor_monitor.reset(liveNow());
  replay=false;theme_preview=-1;measurement_detail=-1;source_time=last_receive=0;dirty=true;
  services::Observation cleared;services::publish(cleared);
}
void command(const char* input) {
  StaticJsonDocument<1536> doc;
  if (deserializeJson(doc,input)) { ++serial_errors;Serial.println("{\"event\":\"error\",\"reason\":\"invalid_json\"}");return; }
  request_id=doc["request_id"]|uint32_t(0);
  const char* op=doc["op"]|"";
  if (!strcmp(op,"status")) status();
  else if(!strcmp(op,"rf_status")){if(rf_stream_until&&int32_t(rf_stream_until-millis())>0)rf_stream_until=millis()+15000;rfStatus();}
  else if(!strcmp(op,"rf_monitor")){
    const bool enabled=doc["enabled"]|false;rf_stream_until=enabled?millis()+15000:0;
    Serial.printf("{\"event\":\"rf_monitor\",\"request_id\":%u,\"enabled\":%s}\n",request_id,enabled?"true":"false");
  }
  else if(!strcmp(op,"theme_preview") && replay){int theme=doc["theme"]|-1;if(theme>=-1 && theme<=3){theme_preview=theme;dirty=true;status("theme_preview");}}
  else if(!strcmp(op,"diagnostics")){bool passed=checkpoint::selftest();bool archive=archive_storage::selftest();Serial.printf("{\"event\":\"diagnostics\",\"request_id\":%u,\"checkpoint_passed\":%s,\"archive_passed\":%s,\"portal_passed\":%s}\n",request_id,passed?"true":"false",archive?"true":"false",network.portal_passed?"true":"false");}
  else if(!strcmp(op,"checkpoint_save")){
    const bool live=!replay&&clock_anchored&&store->accepted;
    const bool saved=live&&checkpoint::save(history,*store,daily_rain,liveNow(),services::settings().station_id,services::settings().channel);
    const bool archived=live&&archive_storage::flush();
    Serial.printf("{\"event\":\"checkpoint_save\",\"request_id\":%u,\"weather_saved\":%s,\"archive_saved\":%s}\n",request_id,saved?"true":"false",archived?"true":"false");
  }
  else if(!strcmp(op,"archive")){
    const uint32_t current=services::clockValid()?uint64_t(time(nullptr))*1000/history_archive::kIntervalMs:0;
    const uint32_t end=doc["end"]|archive_storage::endInterval(uint64_t(time(nullptr))*1000);const uint32_t offset=doc["offset"]|uint32_t(0);
    if(!current||!end||end>current||current-end>=history_archive::kRecords||offset>=history_archive::kRecords||!archive_storage::state(liveNow()).ready){Serial.println("{\"event\":\"error\",\"reason\":\"invalid_archive_window_or_storage\"}");}
    else{DynamicJsonDocument reply(16384);reply["event"]="archive";reply["request_id"]=request_id;reply["end"]=end;
      reply["next_offset"]=std::min(uint32_t(history_archive::kRecords),uint32_t(offset+archive_storage::kPageSize));
      reply["csv"]=(offset==0?String(archive_storage::csvHeader()):String(""))+archive_storage::csvPage(end,offset);
      serializeJson(reply,Serial);Serial.println();}
  }
  else if (!strcmp(op,"reset")) {resetData();status("reset");}
  else if (!strcmp(op,"page")) {
    if (doc["page"].is<unsigned>() && doc["page"].as<unsigned>()<3) {page=doc["page"];measurement_detail=-1;dirty=true;status("page");}
  } else if(!strcmp(op,"detail")) {
    if(doc["detail"].is<unsigned>() && doc["detail"].as<unsigned>()<6){measurement_detail=doc["detail"];dirty=true;status("detail");}
  } else if (!strcmp(op,"sample")) {
    if(!replay && store->accepted)resetData();
    if(have_sample_reply && request_id && request_id==last_sample_request_id){
      Serial.printf("{\"event\":\"sample\",\"request_id\":%u,\"result\":%u,\"accepted\":%u}\n",request_id,last_sample_result,store->accepted);
      request_id=0;return;
    }
    weather::Sample s;
    s.time_ms=doc["time_ms"]|uint64_t(0);s.station=doc["station"]|uint16_t(0);
    const char* channel=doc["channel"]|"";s.channel=channel[0];s.type=doc["type"]|uint8_t(0);
    s.exception=doc["exception"]|false;
    load(doc["temperature_F"],s.temperature);load(doc["humidity"],s.humidity);
    load(doc["wind_mph"],s.wind);load(doc["direction_deg"],s.direction);
    load(doc["lux"],s.lux);load(doc["uv"],s.uv);
    load(doc["rain_tips"],s.rain_tips);load(doc["strikes"],s.strikes);
    const uint32_t previous_rain=store->session_rain_tips;
    const auto previous_counter=store->rainState();
    auto result=store->ingest(s);
    sensor_monitor.observe(s,result);
    if(result==weather::Result::Duplicate){source_time=s.time_ms;last_receive=uptimeMs();replay=true;dirty=true;}
    if (result==weather::Result::Accepted) { source_time=s.time_ms;last_receive=uptimeMs();replay=true;dirty=true;
      if(s.rain_tips.valid)daily_rain.observe(localDay(s.time_ms),s.time_ms,store->session_rain_tips-previous_rain,previous_counter.baseline && s.time_ms>=previous_counter.time && s.time_ms-previous_counter.time<weather::kStaleMs && store->rain_discontinuities==previous_counter.discontinuities);
    }
    Serial.printf("{\"event\":\"sample\",\"request_id\":%u,\"result\":%u,\"accepted\":%u}\n",request_id,unsigned(result),store->accepted);
    last_sample_request_id=request_id;last_sample_result=unsigned(result);have_sample_reply=true;
  } else if (!strcmp(op,"screen") && display_ok) {
    if(setup_open){Serial.println("{\"event\":\"error\",\"reason\":\"setup_view_contains_key\"}");request_id=0;return;}
    draw();
    Serial.println("SCREEN RGB888 320 240 230400");
    uint8_t row[320*3];
    for(int y=0;y<240;++y){
      canvas.readRectRGB(0,y,320,1,row);
      // USB writes can be short when the host briefly stops draining data.
      // Retry the remaining bytes rather than silently dropping pixels.
      size_t sent=0;const uint64_t deadline=uptimeMs()+2000;
      while(sent<sizeof(row)){
        sent+=Serial.write(row+sent,sizeof(row)-sent);
        if(sent<sizeof(row)){if(uptimeMs()>=deadline){request_id=0;return;}delay(1);}
      }
    }
    Serial.println();
  } else {++serial_errors;Serial.println("{\"event\":\"error\",\"reason\":\"unknown_operation\"}");}
  request_id=0;
}
void receive() {
  // Bound serial work so a large replay cannot starve touch or rendering.
  for (unsigned n=0;n<256 && Serial.available();++n) {
    const int ch=Serial.read();
    if (ch=='\n') {
      if (!discard_line && line_size){line[line_size]=0;command(line);}
      else if(discard_line){++serial_errors;Serial.println("{\"event\":\"error\",\"reason\":\"line_too_long\"}");}
      line_size=0;discard_line=false;
    } else if(ch!='\r' && !discard_line) {
      if(line_size+1<sizeof(line))line[line_size++]=char(ch);else discard_line=true;
    }
  }
}
void touch() {
  static bool down=false;static int16_t start_x=0,start_y=0,last_x=0,last_y=0;
  static uint64_t pressed_at=0;
  int16_t x=0,y=0;
  const bool pressed=display_ok && display.getTouch(&x,&y);
  if (pressed) {
    if(!down){start_x=x;start_y=y;pressed_at=uptimeMs();++touch_events;}
    down=true;last_x=x;last_y=y;
  } else if(down) {
    down=false;const int dx=last_x-start_x,dy=last_y-start_y;
    if(setup_open){if(abs(dx)<15&&abs(dy)<15&&start_y<40){setup_open=false;dirty=true;}return;}
    if((alerts_open || page==2) && abs(dy)>20 && abs(dy)>abs(dx)){detail_scroll=max(0,min(6000,detail_scroll-dy));dirty=true;return;}
    if(alerts_open){if(start_y<40){alerts_open=false;detail_scroll=0;}else if(start_y>205){++alert_index;detail_scroll=0;}dirty=true;return;}
    if(measurement_detail>=0){
      if(abs(dx)>45 && abs(dx)>abs(dy)*3/2 && start_y>=44 && start_y<187 && measurement_detail<3){metric=measurement_detail==0?0:measurement_detail==1?1:3;page=1;measurement_detail=-1;dirty=true;}
      else if(abs(dx)<15 && abs(dy)<15){if(start_y<44){measurement_detail=-1;dirty=true;}else if(start_y>=188){alerts_open=true;detail_scroll=0;dirty=true;}}
      return;
    }
    if(abs(dx)>45 && abs(dx)>abs(dy)*3/2 && start_y>=44 && start_y<187) {
      page=(page+(dx<0?1:2))%3;
      dirty=true;status("swipe");
    } else if(abs(dx)<15 && abs(dy)<15 && uptimeMs()-pressed_at<1000) {
      if(start_y<26){setup_open=true;dirty=true;}
      else if(start_y<44){measurement_detail=5;dirty=true;}
      else if(start_y>=188){alerts_open=true;detail_scroll=0;dirty=true;}
      else if(page==2){if(start_y<62){forecast_hourly=!forecast_hourly;forecast_index=0;}else ++forecast_index;detail_scroll=0;dirty=true;}
      else if(page==1 && start_y>=44 && start_y<187){metric=(metric+1)%4;dirty=true;}
      else if(page==0 && start_y>=44 && start_y<184){measurement_detail=start_y<136?(start_x<180?0:1):start_x<108?2:start_x<212?3:4;dirty=true;}
      status("tap");
    }
  }
}
void liveReceive(){
  char packet[1024];
  for(int count=0;count<4 && radio_receiver::pop(packet,sizeof(packet));++count){
    if(replay)continue;
    StaticJsonDocument<2048>d;if(deserializeJson(d,packet))continue;
    weather::Sample s;if(!atlas::decode(d.as<JsonVariantConst>(),liveNow(),s))continue;
    uint32_t previous=store->session_rain_tips;auto counter=store->rainState();
    const auto result=store->ingest(s);sensor_monitor.observe(s,result);
    if(Serial&&rf_stream_until&&int32_t(rf_stream_until-millis())>0){
      StaticJsonDocument<2048> record;record["event"]="rf_packet";record["uptime_ms"]=millis();
      record["result"]=unsigned(result);record["sensor_status"]=sensor_health::label(sensorState());record["packet"]=d.as<JsonObjectConst>();
      char encoded[1536];const size_t length=serializeJson(record,encoded,sizeof(encoded));
      Serial.write(reinterpret_cast<const uint8_t*>(encoded),length);Serial.println();
    }
    if(clock_anchored)archive_storage::observe(s,result,store->session_rain_tips-previous,counter.time,
      counter.baseline&&s.time_ms>=counter.time&&s.time_ms-counter.time<weather::kStaleMs&&counter.discontinuities==store->rain_discontinuities);
    if(result==weather::Result::Accepted){
      if(s.rain_tips.valid && clock_anchored)daily_rain.observe(localDay(s.time_ms),s.time_ms,store->session_rain_tips-previous,counter.baseline && s.time_ms>=counter.time && s.time_ms-counter.time<weather::kStaleMs && counter.discontinuities==store->rain_discontinuities);
      dirty=true;
    }
  }
}
void publishWeather(){
  services::Observation o;uint64_t now=dataNow();o.epoch_ms=store->latest_ms;o.live=!replay && clock_anchored;o.fresh=store->wind.fresh(now)&&sensorState()==sensor_health::State::Current;
  o.sensor_health=uint8_t(sensorState());if(sensor_monitor.received&&now>=sensor_monitor.last_rx)o.sensor_packet_age_s=int32_t((now-sensor_monitor.last_rx)/1000);
  o.temperature_valid=store->temperature.fresh(now);o.temperature_f=store->temperature.value;
  o.humidity_valid=store->humidity.fresh(now);o.humidity=store->humidity.value;
  o.wind_valid=store->wind.fresh(now);o.wind_mph=store->wind.value;o.direction_valid=store->direction.fresh(now);o.direction_deg=store->direction.value;
  auto gust=store->gust(now);o.gust_valid=gust.valid && store->gustComplete(now);o.gust_mph=gust.value;
  o.lux_valid=store->lux.fresh(now);o.lux=store->lux.value;o.uv_valid=store->uv.fresh(now);o.uv=store->uv.value;
  const auto rain_today=todayRain(now);o.rain_today_observed=rain_today.valid;
  const auto counter=store->rainState();
  auto rain24=store->rain_timeline.window(now,weather::kRainDayMs);
  auto rain_hour=store->rain_timeline.window(now,3600000);
  o.rain_24h_observed=rain24.valid;o.rain_24h_valid=rain24.valid&&rain24.complete;o.rain_24h_in=rain24.tips/100.0f;
  o.rain_hour_valid=rain_hour.valid&&rain_hour.complete;o.rain_hour_in=rain_hour.tips/100.0f;
  o.rain_today_valid=rain_today.valid&&rain_today.complete&&now>=counter.time&&now-counter.time<weather::kStaleMs;o.rain_today_in=rain_today.tips/100.0f;
  o.radio_ready=radio_receiver::ready();o.packets=store->accepted;o.dropped=radio_receiver::dropped();services::publish(o);
}
}  // namespace

void setup() {
  firmware_updates::begin();
  // A stalled new image must reset before it can be confirmed healthy.
  esp_task_wdt_init(30,true);enableLoopWDT();
  Serial.setRxBufferSize(2048);Serial.setTxBufferSize(4096);
  Serial.begin(115200);Serial.setTxTimeoutMs(20);
  setenv("TZ","EST5EDT,M3.2.0,M11.1.0",1);tzset();
  history=static_cast<weather::Bucket*>(heap_caps_malloc(sizeof(weather::Bucket)*weather::kHistoryBuckets,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if (!history) {Serial.println("{\"event\":\"fatal\",\"reason\":\"history_allocation_failed\"}");return;}
  for(size_t i=0;i<weather::kHistoryBuckets;++i)new(history+i)weather::Bucket{};
  rain_history=static_cast<weather::RainInterval*>(heap_caps_calloc(weather::kRainIntervals,sizeof(weather::RainInterval),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!rain_history){Serial.println("{\"event\":\"fatal\",\"reason\":\"rain_history_allocation_failed\"}");return;}
  store=new weather::Store(history,679,'A',rain_history);
  display_ok=display.init();display.setRotation(1);display.setBrightness(160);
  canvas.setPsram(true);canvas.setColorDepth(16);
  display_ok=display_ok && canvas.createSprite(320,240)!=nullptr;
  canvas.setTextDatum(textdatum_t::top_left);
  if(display_ok)draw();
  services::begin();checkpoint::begin();archive_storage::begin(services::settings().station_id,services::settings().channel);
  store->~Store();new(store)weather::Store(history,services::settings().station_id,services::settings().channel,rain_history);
  if(services::settings().radio_enabled)radio_receiver::begin();
  status("boot");
}
void loop() {
  if(!store){firmware_updates::confirmBoot(false);delay(100);return;}
  receive();touch();services::loop();radio_receiver::loop();
  firmware_updates::confirmBoot(display_ok&&network.portal_passed,display_ok?network.portal_test_status.c_str():"Display initialization failed");
  if(firmware_updates::restartPending()){
    archive_storage::flush();
    if(!replay&&clock_anchored&&store->accepted)checkpoint::save(history,*store,daily_rain,liveNow(),services::settings().station_id,services::settings().channel);
    ESP.restart();
  }
  liveReceive();
  const uint64_t now=uptimeMs();
  if(services::clockValid() && !clock_anchored){clock_epoch=uint64_t(time(nullptr))*1000;clock_uptime=now;clock_anchored=true;if(!replay){resetData();checkpoint_restored=checkpoint::restore(history,*store,daily_rain,liveNow(),services::settings().station_id,services::settings().channel);}}
  if(now-last_publish>=1000){if(!replay&&clock_anchored)archive_storage::loop(liveNow(),sensorState()==sensor_health::State::Current);publishWeather();last_publish=now;}
  if(!replay && clock_anchored && store->accepted && now-last_checkpoint>=300000){checkpoint::save(history,*store,daily_rain,liveNow(),services::settings().station_id,services::settings().channel);last_checkpoint=now;}
  if((dirty && now-last_draw>=100) || now-last_draw>=1000)draw();
  if(now-last_status>=10000){status();last_status=now;}
  delay(2);
}

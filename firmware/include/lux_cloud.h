#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
namespace lux_cloud {
constexpr double pi=3.14159265358979323846;
constexpr double rad=pi/180;
enum class Kind:uint8_t { Unknown,Clear,PartlyCloudy,Cloudy };
inline const char* label(Kind kind){switch(kind){case Kind::Clear:return "Likely clear";case Kind::PartlyCloudy:return "Likely partly cloudy";case Kind::Cloudy:return "Likely cloudy";default:return "Unavailable";}}
// NOAA fractional-year equations, UTC; longitude is positive east.
inline double elevation(int64_t epoch,double latitude,double longitude){
  if(epoch<1700000000||!std::isfinite(latitude)||!std::isfinite(longitude)||latitude<-90||latitude>90||longitude<-180||longitude>180)return NAN;
  time_t seconds=epoch;tm utc{};if(!gmtime_r(&seconds,&utc))return NAN;
  const int year=utc.tm_year+1900;const bool leap=(year%4==0&&year%100!=0)||year%400==0;
  const double hour=utc.tm_hour+utc.tm_min/60.0+utc.tm_sec/3600.0;
  const double gamma=2*pi/(leap?366:365)*(utc.tm_yday+(hour-12)/24);
  const double eq=229.18*(0.000075+0.001868*cos(gamma)-0.032077*sin(gamma)-0.014615*cos(2*gamma)-0.040849*sin(2*gamma));
  const double decl=0.006918-0.399912*cos(gamma)+0.070257*sin(gamma)-0.006758*cos(2*gamma)+0.000907*sin(2*gamma)-0.002697*cos(3*gamma)+0.00148*sin(3*gamma);
  const double angle=(hour*60+eq+4*longitude)/4-180;
  const double sine=sin(latitude*rad)*sin(decl)+cos(latitude*rad)*cos(decl)*cos(angle*rad);
  return asin(std::max(-1.0,std::min(1.0,sine)))/rad;
}
// Haurwitz-like relative daylight shape, not lux-to-W/m² conversion.
inline double shape(double altitude){const double sine=sin(altitude*rad);return sine>0?sine*exp(-0.059/sine):0;}
inline double calibratedReference(double lux,double altitude){
  if(!std::isfinite(lux)||lux<1000||lux>=200000||!std::isfinite(altitude)||altitude<15)return 0;
  const double reference=lux*shape(45)/shape(altitude);
  return reference>=1000&&reference<=200000?reference:0;
}
struct Estimate {
  Kind kind=Kind::Unknown;
  double altitude=0,expected=0,ratio=0;
  const char* reason="Disabled";
  uint64_t sampled_ms=0;
  unsigned samples=0,candidate_age_s=0;
  bool valid()const{return kind!=Kind::Unknown;}
};
class Tracker {
  uint64_t last_sample=0,candidate_since=0;
  unsigned count=0;
  double mean=0;
  Kind candidate=Kind::Unknown,stable=Kind::Unknown;
public:
  Estimate estimate;
  void reset(const char* reason){last_sample=candidate_since=0;count=0;mean=0;candidate=stable=Kind::Unknown;estimate={};estimate.reason=reason;}
  void update(bool enabled,double reference,bool location,double lat,double lon,bool live,bool fresh,uint64_t sample_ms,double lux,uint64_t now_ms){
    if(!enabled){reset("Disabled");return;}
    if(!location||now_ms<1700000000000ULL){reset("Location and valid clock required");return;}
    const double altitude=elevation(now_ms/1000,lat,lon);
    if(!std::isfinite(altitude)||altitude<15){reset("Low sun or nighttime; using NWS");estimate.altitude=std::isfinite(altitude)?altitude:0;return;}
    if(!live||!fresh||!sample_ms||now_ms<sample_ms||now_ms-sample_ms>=120000){reset("Waiting for fresh live light");estimate.altitude=altitude;return;}
    if(!std::isfinite(reference)||reference<1000||reference>200000){reset("Calibrate in unobstructed direct sunlight");estimate.altitude=altitude;return;}
    const double expected=reference*shape(altitude)/shape(45);
    if(!std::isfinite(lux)||lux<0||lux>=200000||expected<1000){reset("Light range or calibration invalid");return;}
    const double ratio=lux/expected;
    if(ratio>1.6){reset("Above calibration; using NWS");return;}
    estimate.altitude=altitude;estimate.expected=expected;estimate.sampled_ms=sample_ms;
    if(sample_ms==last_sample){estimate.ratio=mean;return;}
    if(last_sample&&(sample_ms<last_sample||sample_ms-last_sample>=120000))reset("Warming up");
    const double alpha=last_sample?std::min(1.0,double(sample_ms-last_sample)/60000):1;
    mean=last_sample?mean+alpha*(ratio-mean):ratio;last_sample=sample_ms;
    const Kind proposed=mean<0.4?Kind::Cloudy:mean<0.8?Kind::PartlyCloudy:Kind::Clear;
    if(proposed!=candidate){candidate=proposed;candidate_since=sample_ms;count=1;}else if(count<1000)++count;
    // A new icon requires multiple distinct packets and two minutes stability.
    if(sample_ms-candidate_since>=120000&&count>=4)stable=candidate;
    estimate.samples=count;estimate.candidate_age_s=unsigned((sample_ms-candidate_since)/1000);
    estimate.kind=stable;estimate.ratio=mean;estimate.altitude=altitude;
    estimate.expected=expected;estimate.sampled_ms=sample_ms;
    estimate.reason=stable==Kind::Unknown?"Warming up: 2 minutes of stable light":"Experimental daylight estimate";
  }
};
}

#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace weather_units {
inline bool valid(const char* units){return units&&(!std::strcmp(units,"metric")||!std::strcmp(units,"imperial"));}
inline double temperature(double fahrenheit,bool metric){return metric?(fahrenheit-32.0)/1.8:fahrenheit;}
inline double wind(double mph,bool metric){return metric?mph*1.609344:mph;}
inline double distance(double miles,bool metric){return metric?miles*1.609344:miles;}
inline const char* distanceUnit(bool metric){return metric?"km":"mi";}
inline double rain(double inches,bool metric){return metric?inches*25.4:inches;}
inline double pressure(double pa,bool metric){return metric?pa/100.0:pa/3386.389;}
inline const char* temperatureUnit(bool metric){return metric?"°C":"°F";}
inline const char* windUnit(bool metric){return metric?"km/h":"mph";}
inline const char* rainUnit(bool metric){return metric?"mm":"in";}
inline const char* pressureUnit(bool metric){return metric?"hPa":"inHg";}
// Convert only the dedicated NWS wind descriptor, never forecast prose.
inline std::string windDescription(const std::string& source,bool metric){
  const auto unit=source.find("mph");
  if(!metric||unit==std::string::npos||unit+3!=source.size())return source;
  std::string out;const char* at=source.c_str();const char* stop=at+unit;
  while(at<stop){
    if(*at>='0'&&*at<='9'){
      char* end=nullptr;const double value=std::strtod(at,&end);
      if(end>at&&end<=stop&&std::isfinite(value)){
        char digits[24];std::snprintf(digits,sizeof(digits),"%.0f",wind(value,true));out+=digits;at=end;continue;
      }
    }
    out+=*at++;
  }
  return out+"km/h";
}
}

#include "lightning.h"
#include "weather_units.h"
#include "home_fonts.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
static int width(const char* text,const home_fonts::Font& font){
  unsigned sum=0;for(const char* c=text;*c;++c)for(unsigned i=0;i<font.count;++i)if(font.glyphs[i].code==unsigned(*c)){sum+=font.glyphs[i].advance;break;}
  return (sum+32)/64;
}
int main(){
 using namespace lightning;
 Tracker t;
 assert(!t.view(1).capable&&!t.view(1).show_lightning);
 t.observe(1,17,true,8);assert(t.view(1).capable&&!t.view(1).event_known&&!t.view(1).active);
 t.observe(10001,17,true,8);assert(!t.view(10001).active);
 t.observe(20001,18,true,8);auto v=t.view(20001);
 assert(v.active&&v.show_lightning&&v.count==1&&!v.complete&&v.distance_known&&v.distance_miles==8);
 assert(t.view(25000).show_lightning&&!t.view(25001).show_lightning);
 t.observe(26001,19,true,8);assert(!t.view(26001).show_lightning); // busy storm cannot starve Barometer
 assert(t.view(30001).show_lightning);
 t.observe(32001,19,true,6);v=t.view(32001);
 assert(v.count==2&&v.event_age_ms==6000&&v.change_age_ms==0&&v.distance_miles==6);
 t.observe(33001,19,true,31);assert(!t.view(33001).distance_known&&t.view(33001).change_age_ms==1000);
 t.observe(34001,19,true,6);assert(t.view(34001).change_age_ms==2000); // same valid value, not new activity
 t.observe(35001,19,true,7);assert(t.view(35001).change_age_ms==0&&!t.view(35001).show_lightning);
 for(unsigned minute=1;minute<30;++minute)t.observe(35001+minute*60000,19,true,7);
 assert(t.view(35001+kActivityMs-1).active);
 assert(!t.view(35001+kActivityMs).active);
 t.observe(35001+kActivityMs,19,true,7);assert(!t.view(35001+kActivityMs).active); // repeats do not refresh
 t.observe(35002+kActivityMs,19,true,8);v=t.view(35002+kActivityMs);
 assert(v.active&&v.count==0&&v.event_age_ms>kActivityMs&&v.change_age_ms==0); // correction isn't a strike
 t.observe(35003+kActivityMs,0,true,8);assert(!t.view(35003+kActivityMs).active); // reset
 t.observe(35004+kActivityMs,1,false,0);v=t.view(35004+kActivityMs);
 assert(v.active&&v.count==1&&!v.distance_known&&!v.complete); // new strike cannot borrow previous distance
 assert(!t.view(35004+kActivityMs+kReportFreshMs).active);
 t.observe(35004+kActivityMs+kReportFreshMs,20,true,8);assert(!t.view(35004+kActivityMs+kReportFreshMs).active); // gap: no known event time
 Tracker wrap;wrap.observe(1,511,true,8);wrap.observe(10001,0,true,8);assert(!wrap.view(10001).active);
 wrap.observe(20001,1,true,8);assert(wrap.view(20001).count==1&&wrap.view(20001).active);
 wrap.observe(19999,2,true,6);wrap.observe(20002,512,true,6);wrap.observe(20002,2,true,32);
 assert(wrap.view(20002).count==1&&wrap.view(20002).distance_miles==8);
 Tracker unknown;unknown.observe(1,0,false,0);unknown.observe(10001,1,true,0);
 assert(unknown.view(10001).active&&!unknown.view(10001).distance_known);
 unknown.observe(20001,2,true,26);assert(!unknown.view(20001).distance_known);
 unknown.observe(30001,3,true,25);assert(unknown.view(30001).distance_known);
 Tracker capacity;capacity.observe(1,0,true,8);
 for(unsigned minute=1;minute<=10;++minute)capacity.observe(1+minute*60000,0,true,8);
 assert(capacity.view(600001).complete&&!capacity.view(600001).active);
 for(unsigned i=1;i<=70;++i)capacity.observe(600001+i*100,uint16_t(i),true,8);
 assert(capacity.view(607001).count==64&&!capacity.view(607001).complete);
 for(unsigned minute=1;minute<=10;++minute)capacity.observe(607001+minute*60000,70,true,8);
 assert(capacity.view(1207001).count==0&&capacity.view(1207001).complete);
 assert(std::abs(weather_units::distance(8,true)-12.874752)<.000001);
 assert(weather_units::distance(8,false)==8&&!std::strcmp(weather_units::distanceUnit(true),"km"));
 assert(width("LIGHTNING 10m*",home_fonts::status)<=82);
 assert(width("Last 40.2 km",home_fonts::small)<=51);
 assert(width("Last --",home_fonts::status)<=51);
 assert(width("999m ago",home_fonts::status)<=51);
 assert(width("99+",home_fonts::body)<=21&&width("99",home_fonts::clock)<=21);
 static_assert(sizeof(Tracker)<1400,"Keep lightning storage bounded");
 std::cout<<"Lightning checks passed: capability, baseline, non-starving 5s rotation, exact 30m expiry, repeats, distance-only changes, gaps/resets/wraps, overflow/coverage, units and packed-font fit.\n";
}

#include "lux_cloud.h"
#include "nws_logic.h"
#include <cassert>
#include <iostream>
int main(){
  const auto noon=nws::isoTime("2026-03-20T12:00:00Z");
  assert(lux_cloud::elevation(noon,0,0)>87);
  assert(lux_cloud::elevation(noon+12*3600,0,0)<-87);
  assert(lux_cloud::elevation(nws::isoTime("2026-06-21T12:00:00Z"),45,0)>68);
  assert(lux_cloud::elevation(nws::isoTime("2026-12-21T12:00:00Z"),45,0)<22);
  assert(std::isnan(lux_cloud::elevation(noon,91,0)));
  assert(lux_cloud::calibratedReference(15000,45)>14999);
  assert(lux_cloud::calibratedReference(15000,10)==0);
  using K=lux_cloud::Kind;lux_cloud::Tracker t;
  uint64_t now=uint64_t(noon)*1000;
  auto update=[&](uint64_t stamp,double ratio){const double a=lux_cloud::elevation(stamp/1000,0,0);t.update(true,15000,true,0,0,true,true,stamp,ratio*15000*lux_cloud::shape(a)/lux_cloud::shape(45),stamp);};
  for(int i=0;i<5;++i)update(now+i*30000,1);
  assert(t.estimate.kind==K::Clear);
  for(int i=5;i<24;++i)update(now+i*30000,.2);
  assert(t.estimate.kind==K::Cloudy);
  t.reset("millisecond clock regression");
  for(int i=0;i<5;++i)update(now+789+i*30001,1);
  assert(t.estimate.kind==K::Clear&&t.estimate.samples>=4);
  t.reset("test");update(now,.6);
  for(int i=0;i<10;++i)update(now,.6);assert(!t.estimate.valid());
  for(int i=1;i<=4;++i)update(now+i*30000,.6);assert(t.estimate.kind==K::PartlyCloudy);
  t.update(true,15000,true,0,0,false,true,now,0,now);assert(!t.estimate.valid());
  t.update(true,15000,true,0,0,true,true,now,0,now+120000);assert(!t.estimate.valid());
  t.update(true,15000,true,0,0,true,true,now+1,0,now);assert(!t.estimate.valid());
  t.update(true,0,true,0,0,true,true,now,15000,now);assert(!t.estimate.valid());
  t.update(true,15000,true,0,0,true,true,now+43200000,0,now+43200000);assert(!t.estimate.valid());
  t.update(true,15000,true,0,0,true,true,now,200000,now);assert(!t.estimate.valid());
  std::cout<<"Lux cloud checks passed: solar seasons/night, calibration, stabilization, sparse/stale/replay and saturation.\n";
}

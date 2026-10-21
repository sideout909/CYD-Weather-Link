#include "sensor_health.h"
#include <cassert>
#include <iostream>
int main(){
  weather::Store store;sensor_health::Monitor monitor;constexpr uint64_t start=1000000;
  monitor.reset(start);using S=sensor_health::State;
  assert(monitor.state(false,false,start,store)==S::Off);
  assert(monitor.state(true,false,start,store)==S::NoRadio);
  assert(monitor.state(true,true,start,store)==S::Waiting);
  assert(monitor.state(true,true,start+120000,store)==S::NoSignal);
  auto packet=[](uint64_t now,uint8_t type){weather::Sample s;s.time_ms=now;s.type=type;s.wind.set(8.2f,now);if(type==37){s.temperature.set(56.3f,now);s.humidity.set(58,now);}if(type==38){s.rain_tips.set(100,now);s.direction.set(225,now);}if(type==39){s.lux.set(24000,now);s.uv.set(2,now);}return s;};
  for(unsigned i=0;i<3;++i){auto p=packet(start+i*10000,37+i);auto result=store.ingest(p);monitor.observe(p,result);}
  assert(monitor.state(true,true,start+20000,store)==S::Current);
  auto repeat=packet(start+21000,39);monitor.observe(repeat,store.ingest(repeat));
  assert(store.duplicates==1&&monitor.last_rx==start+21000);
  // Repeated copies of one Atlas type must not conceal missing other types.
  for(unsigned t=30000;t<=180000;t+=10000){auto p=packet(start+t,37);monitor.observe(p,store.ingest(p));}
  assert(store.wind.fresh(start+180000));
  assert(monitor.state(true,true,start+180000,store)==S::DataStale);
  assert(monitor.state(true,true,start+300000,store)==S::NoSignal);
  auto wrong=packet(start+310000,38);wrong.station=999;monitor.observe(wrong,store.ingest(wrong));
  assert(monitor.last_rx==start+180000);
  auto bad=packet(start+320000,37);bad.exception=true;monitor.observe(bad,store.ingest(bad));
  assert(monitor.last_rx==start+320000&&monitor.state(true,true,start+320000,store)==S::DataStale);
  for(unsigned i=0;i<3;++i){auto p=packet(start+330000+i*10000,37+i);monitor.observe(p,store.ingest(p));}
  assert(monitor.state(true,true,start+350000,store)==S::Current);
  monitor.reset(start);weather::Store stationary;
  for(unsigned t=0;t<=180000;t+=10000){auto p=packet(start+t,37+(t/10000)%3);monitor.observe(p,stationary.ingest(p));}
  assert(monitor.state(true,true,start+180000,stationary)==S::Current); // Stable weather is valid.
  monitor.reset(start);assert(monitor.received==0&&monitor.last_rx==0);
  std::cout<<"Sensor health checks passed: receiver states, normal cycle, duplicates, stuck type, stopped transmitter, flagged data, wrong sensor, recovery and stable weather.\n";
}

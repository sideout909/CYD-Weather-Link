#pragma once
#include "weather_store.h"
#include <array>
namespace sensor_health {
enum class State { Off,NoRadio,Waiting,Collecting,Current,DataStale,NoSignal };
inline const char* label(State state){
  switch(state){case State::Off:return "Receiver off";case State::NoRadio:return "No receiver";case State::Waiting:return "Waiting";case State::Collecting:return "Collecting";case State::Current:return "Current";case State::DataStale:return "Data stale";case State::NoSignal:return "No signal";}
  return "Unknown";
}
struct Monitor {
  uint64_t started=0,last_rx=0;
  std::array<uint64_t,3> types{};
  uint32_t received=0;
  void reset(uint64_t now){*this=Monitor{};started=now;}
  void observe(const weather::Sample& packet,weather::Result result){
    if(!packet.time_ms||packet.type<37||packet.type>39)return;
    if(result!=weather::Result::Accepted&&result!=weather::Result::Duplicate&&result!=weather::Result::Flagged)return;
    if(last_rx&&packet.time_ms<last_rx)return;
    if(!received)started=packet.time_ms;
    last_rx=packet.time_ms;++received;
    if(result!=weather::Result::Flagged)types[packet.type-37]=packet.time_ms;
  }
  State state(bool enabled,bool ready,uint64_t now,const weather::Store& store)const {
    if(!enabled)return State::Off;
    if(!ready)return State::NoRadio;
    if(!received)return now>=started&&now-started>=weather::kStaleMs?State::NoSignal:State::Waiting;
    if(now<last_rx||now-last_rx>=weather::kStaleMs)return State::NoSignal;
    bool full=true;for(auto time:types)full&=time&&now>=time&&now-time<weather::kStaleMs;
    const auto rain=store.rainState();
    full=full&&store.temperature.fresh(now)&&store.humidity.fresh(now)&&store.wind.fresh(now)&&store.direction.fresh(now)&&store.lux.fresh(now)&&store.uv.fresh(now)&&rain.baseline&&now>=rain.time&&now-rain.time<weather::kStaleMs;
    if(full)return State::Current;
    return now>=started&&now-started>=weather::kStaleMs?State::DataStale:State::Collecting;
  }
};
}

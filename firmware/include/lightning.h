#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
namespace lightning {
constexpr uint64_t kWindowMs=600000,kActivityMs=1800000,kReportFreshMs=120000,kPhaseMs=5000;
struct View {
  bool capable=false,event_known=false,fresh=false,active=false,show_lightning=false,complete=false,distance_known=false;
  uint32_t count=0;
  uint8_t distance_miles=0;
  uint64_t event_age_ms=0,change_age_ms=0;
};
// Selected native station only. No restored/replay counters enter this tracker.
// Session-only by design: after reboot a first sample is a baseline, not an event.
class Tracker {
 public:
  void observe(uint64_t now,uint16_t counter,bool has_distance,uint16_t distance){
    if(!now||counter>511||(has_distance&&distance>31)||(baseline_&&now<last_report_))return;
    if(!baseline_){baseline(now,counter,has_distance,distance);return;}
    // A gap or a decrease could conceal a reset/wrap and event timing. Rebaseline
    // rather than invent a fresh strike or hundreds of events. Next increases work.
    if(now-last_report_>=kReportFreshMs||counter<counter_){baseline(now,counter,has_distance,distance);return;}
    prune(now);last_report_=now;
    const bool valid_distance=has_distance&&distance>=1&&distance<=25;
    if(counter>counter_){
      const uint16_t delta=counter-counter_;
      if(size_==events_.size()){
        lost_=true;lost_at_=events_[head_].time;head_=(head_+1)%events_.size();--size_;
      }
      events_[(head_+size_)%events_.size()]={now,delta};++size_;
      activity(now);event_=true;last_event_=now;
      distance_seen_=valid_distance;distance_known_=valid_distance;
      distance_=valid_distance?uint8_t(distance):0;
    }else if(has_distance){
      if(valid_distance){
        if(event_&&(!distance_seen_||distance_!=distance))activity(now);
        distance_=uint8_t(distance);distance_seen_=true;distance_known_=true;
      }else distance_known_=false; // Unknown codes never refresh activity.
    }
    counter_=counter;
  }
  View view(uint64_t now)const {
    View v;v.capable=baseline_;v.event_known=event_;
    v.fresh=baseline_&&now>=last_report_&&now-last_report_<kReportFreshMs;
    v.active=v.fresh&&event_&&now>=last_change_&&now-last_change_<kActivityMs;
    v.show_lightning=v.active&&now>=rotation_start_&&((now-rotation_start_)/kPhaseMs)%2==0;
    v.complete=v.fresh&&now>=coverage_start_&&now-coverage_start_>=kWindowMs&&
      (!lost_||(now>=lost_at_&&now-lost_at_>=kWindowMs));
    for(size_t i=0;i<size_;++i){const auto& e=events_[(head_+i)%events_.size()];if(now>=e.time&&now-e.time<kWindowMs)v.count+=e.count;}
    v.distance_known=event_&&distance_known_;v.distance_miles=distance_;
    if(event_&&now>=last_event_)v.event_age_ms=now-last_event_;
    if(event_&&now>=last_change_)v.change_age_ms=now-last_change_;
    return v;
  }
 private:
  struct Event {uint64_t time=0;uint16_t count=0;};
  std::array<Event,64> events_{};
  size_t head_=0,size_=0;
  bool baseline_=false,event_=false,distance_seen_=false,distance_known_=false,lost_=false;
  uint16_t counter_=0;uint8_t distance_=0;
  uint64_t last_report_=0,coverage_start_=0,last_event_=0,last_change_=0,rotation_start_=0,lost_at_=0;
  void baseline(uint64_t now,uint16_t counter,bool has_distance,uint16_t distance){
    baseline_=true;counter_=counter;last_report_=coverage_start_=now;
    head_=size_=0;event_=lost_=false;last_event_=last_change_=rotation_start_=0;
    distance_seen_=distance_known_=has_distance&&distance>=1&&distance<=25;
    distance_=distance_known_?uint8_t(distance):0;
  }
  void activity(uint64_t now){
    // New activity must not keep restarting the rotation during a busy storm.
    if(!event_||now<last_change_||now-last_change_>=kActivityMs)rotation_start_=now;
    last_change_=now;
  }
  void prune(uint64_t now){
    while(size_&&now>=events_[head_].time&&now-events_[head_].time>=kWindowMs){head_=(head_+1)%events_.size();--size_;}
  }
};
}

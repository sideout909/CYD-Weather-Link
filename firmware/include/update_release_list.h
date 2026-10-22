#pragma once
#include "update_policy.h"
#include <ArduinoJson.h>
#include <string>
namespace update_release_list {
// Filter GitHub's release bodies/assets out of the bounded document. Only these
// metadata fields choose a manifest; trust still comes from its signature.
template<typename Input> bool newest(Input& input,std::string& best){
  best.clear();StaticJsonDocument<192> filter;
  filter[0]["tag_name"]=true;filter[0]["draft"]=true;filter[0]["prerelease"]=true;
  DynamicJsonDocument releases(4096);
  if(deserializeJson(releases,input,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(10))||
     !releases.is<JsonArray>()||releases.size()>10)return false;
  for(JsonObjectConst release:releases.as<JsonArrayConst>()){
    if(!release["draft"].is<bool>()||!release["prerelease"].is<bool>()||!release["tag_name"].is<const char*>())continue;
    const auto candidate=update_policy::listedRelease(release["tag_name"].as<const char*>(),release["draft"].as<bool>(),release["prerelease"].as<bool>());
    if(!candidate.empty()&&(best.empty()||update_policy::newer(candidate,best)))best=candidate;
  }
  return !best.empty();
}
}

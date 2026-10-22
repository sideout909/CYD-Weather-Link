#pragma once
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
namespace update_policy {
// Only migrate the renamed project source; user-selected forks remain untouched.
inline std::string migratedProjectRepository(const std::string& value,
                                              const std::string& project_default){
  std::string lower=value;
  for(char& ch:lower)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return value.empty()||lower=="sideout909/cyd-weather-link"?project_default:value;
}
inline bool repository(const std::string& value){
  const auto slash=value.find('/');
  if(slash==std::string::npos||slash==0||slash+1==value.size()||value.size()>180||value.find('/',slash+1)!=std::string::npos)return false;
  if(value.substr(0,slash)=="."||value.substr(0,slash)==".."||value.substr(slash+1)=="."||value.substr(slash+1)=="..")return false;
  for(unsigned char ch:value)if(ch!='/'&&!std::isalnum(ch)&&ch!='-'&&ch!='_'&&ch!='.')return false;
  return true;
}
inline bool version(const std::string& value,std::array<unsigned,3>& parts){
  size_t at=0;
  for(unsigned i=0;i<3;++i){
    unsigned number=0,count=0;
    while(at<value.size()&&std::isdigit(static_cast<unsigned char>(value[at]))){number=number*10+unsigned(value[at++]-'0');if(++count>5||number>65535)return false;}
    if(!count)return false;
    parts[i]=number;
    if(i<2){if(at==value.size()||value[at++]!='.')return false;}
  }
  return at==value.size();
}
inline bool newer(const std::string& candidate,const std::string& current){
  std::array<unsigned,3>a{},b{};return version(candidate,a)&&version(current,b)&&a>b;
}
inline bool sha256(const std::string& value){
  if(value.size()!=64)return false;
  for(unsigned char ch:value)if(!std::isxdigit(ch))return false;
  return true;
}
inline bool firmwareUrl(const std::string& url,const std::string& repo,const std::string& expected_version=""){
  if(!repository(repo))return false;
  std::string prefix="https://github.com/"+repo+"/releases/download/",observed=url.substr(0,prefix.size());
  for(char& c:prefix)c=std::tolower(static_cast<unsigned char>(c));
  for(char& c:observed)c=std::tolower(static_cast<unsigned char>(c));
  if(url.size()>=1024||observed!=prefix)return false;
  const std::string tail=url.substr(prefix.size());const auto slash=tail.find('/');
  if(slash==std::string::npos||tail.empty()||tail[0]!='v'||tail.substr(slash+1)!="weather-link-panellan_sc05_x.bin")return false;
  const std::string release=tail.substr(1,slash-1);std::array<unsigned,3> parts{};
  return version(release,parts)&&(expected_version.empty()||release==expected_version);
}
inline bool downloadHost(const std::string& url){
  for(const char* prefix:{"https://github.com/","https://release-assets.githubusercontent.com/","https://objects.githubusercontent.com/","https://github-releases.githubusercontent.com/"})if(url.compare(0,std::char_traits<char>::length(prefix),prefix)==0)return url.find_first_of("\r\n")==std::string::npos;
  return false;
}
}

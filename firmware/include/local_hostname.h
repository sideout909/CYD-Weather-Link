#pragma once
#include <string>
namespace local_hostname {
inline bool valid(const std::string& name){
  if(name.empty()||name.size()>32||name.front()=='-'||name.back()=='-')return false;
  for(unsigned char c:name)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'))return false;
  bool numeric=true;for(char c:name)numeric&=c>='0'&&c<='9';return !numeric;
}
inline std::string url(const std::string& name){return "http://"+name+".local";}
}

#pragma once
#include "update_policy.h"
#include <string>
#include <cstdint>
namespace ota_manifest {
inline constexpr const char* algorithm="ecdsa-p256-sha256";
struct Manifest {
  std::string board,partition_layout,version,sha256,firmware_url,channel,key_id;
  uint32_t flash_bytes=0,size=0;
};
inline bool lowerHex(const std::string& text,size_t length){
  if(text.size()!=length)return false;
  for(char c:text)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
  return true;
}
inline bool identifier(const std::string& text){
  if(text.empty()||text.size()>64)return false;
  for(unsigned char c:text)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
  return true;
}
// Byte-exact ASCII, LF separated, final LF included. JSON whitespace/key order
// is irrelevant. Every installation-relevant field is covered by this envelope.
inline bool payload(const Manifest& m,const std::string& repo,std::string& out){
  if(!identifier(m.board)||!identifier(m.partition_layout)||!m.flash_bytes||!m.size||
     !lowerHex(m.sha256,64)||!lowerHex(m.key_id,32)||
     !update_policy::eligible(m.version,m.channel)||
     (m.channel=="dev")!=(update_policy::releaseCandidate(m.version)>0)||
     !update_policy::firmwareUrl(m.firmware_url,repo,m.version))return false;
  out="WX-Link OTA signature v1\n"+std::string(algorithm)+"\n"+m.key_id+"\n"+
      m.board+"\n"+m.partition_layout+"\n"+std::to_string(m.flash_bytes)+"\n"+
      m.version+"\n"+m.channel+"\n"+std::to_string(m.size)+"\n"+m.sha256+"\n"+m.firmware_url+"\n";
  return true;
}
}

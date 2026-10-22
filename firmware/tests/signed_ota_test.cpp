#include "ota_signature.h"
#include <ArduinoJson.h>
#include <iostream>
int main(int argc,char** argv){
  if(argc!=2)return 2;
  DynamicJsonDocument doc(4096);
  if(deserializeJson(doc,std::cin)||doc["schema"]!=1||doc["signature_schema"]!=1||doc["signature_algorithm"]!=ota_manifest::algorithm)return 1;
  ota_manifest::Manifest m;
  m.board=doc["board"]|"";m.partition_layout=doc["partition_layout"]|"";
  m.flash_bytes=doc["flash_bytes"]|uint32_t(0);m.size=doc["size"]|uint32_t(0);
  m.version=doc["version"]|"";m.sha256=doc["sha256"]|"";m.firmware_url=doc["firmware_url"]|"";
  m.channel=doc["channel"]|"";m.key_id=doc["signing_key_id"]|"";
  return ota_signature::verify(m,argv[1],doc["signature"]|"")?0:1;
}

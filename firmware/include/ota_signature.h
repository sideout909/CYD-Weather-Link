#pragma once
#include "ota_manifest.h"
#ifndef WX_LINK_OTA_TRUST_HEADER
#define WX_LINK_OTA_TRUST_HEADER "ota_trust.h"
#endif
#include WX_LINK_OTA_TRUST_HEADER
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <cstring>
namespace ota_signature {
// Never trust a key delivered by a release, a configurable repository or a web form.
inline bool verify(const ota_manifest::Manifest& manifest,const std::string& repo,
                   const std::string& signature){
  std::string content;
  if(manifest.key_id!=ota_trust::key_id||!ota_manifest::payload(manifest,repo,content)||
     signature.size()<128||signature.size()>160||signature.size()%2||
     !ota_manifest::lowerHex(signature,signature.size()))return false;
  unsigned char bytes[80],hash[32];
  auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
  for(size_t i=0;i<signature.size()/2;++i)bytes[i]=(nibble(signature[i*2])<<4)|nibble(signature[i*2+1]);
  if(mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(content.data()),content.size(),hash,0))return false;
  mbedtls_pk_context key;mbedtls_pk_init(&key);
  bool valid=false;
  if(!mbedtls_pk_parse_public_key(&key,reinterpret_cast<const unsigned char*>(ota_trust::public_key),sizeof(ota_trust::public_key))&&
     mbedtls_pk_can_do(&key,MBEDTLS_PK_ECDSA))
    valid=mbedtls_pk_verify(&key,MBEDTLS_MD_SHA256,hash,sizeof(hash),bytes,signature.size()/2)==0;
  mbedtls_pk_free(&key);return valid;
}
}

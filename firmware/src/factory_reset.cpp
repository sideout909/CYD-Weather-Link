#include "factory_reset.h"
#include <LittleFS.h>
#include <Preferences.h>

namespace factory_reset {
namespace {
struct Storage {
  Preferences marker;
  bool pending(bool& value){
    if(!marker.begin("wxreset",false))return false;
    value=marker.getBool("pending",false);
    return true;
  }
  bool mount(){return LittleFS.begin(false);}
  bool removeFile(const char* path){return !LittleFS.exists(path)||LittleFS.remove(path);}
  bool clearSettings(){
    Preferences settings;
    if(!settings.begin("wxlink",false))return false;
    const bool ok=settings.clear();settings.end();return ok;
  }
  bool complete(){return marker.putBool("pending",false)==1;}
};
}
bool arm(){
  Preferences marker;
  if(!marker.begin("wxreset",false))return false;
  const bool ok=marker.putBool("pending",true)==1;
  marker.end();return ok;
}
Recovery resume(){Storage storage;return recover(storage);}
}

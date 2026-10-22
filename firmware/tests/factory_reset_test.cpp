#include "factory_reset.h"
#include <cassert>
#include <set>
#include <string>
#include <iostream>
struct Backend {
  bool requested=false,settings=true,marker_ok=true,mount_ok=true,clear_ok=true,complete_ok=true;
  int attempts=0,fail_at=-1;
  std::set<std::string> files;
  bool pending(bool& value){value=requested;return marker_ok;}
  bool mount(){return mount_ok;}
  bool removeFile(const char* path){if(attempts++==fail_at)return false;files.erase(path);return true;}
  bool clearSettings(){if(!clear_ok)return false;settings=false;return true;}
  bool complete(){if(!complete_ok)return false;requested=false;return true;}
};
int main(){
  using namespace factory_reset;
  assert(confirmed("RESET WX LINK"));
  for(const char* value:{static_cast<const char*>(nullptr),"","reset wx link","RESET WX LINK "})assert(!confirmed(value));
  assert(!held(10999,1000,true)&&held(11000,1000,true));
  assert(!held(12000,1000,false)&&!held(1,1000,true));
  Backend idle;idle.files.insert("/weather.bin");
  assert(recover(idle)==Recovery::NotRequested&&idle.settings&&idle.files.count("/weather.bin"));
  for(int fail=0;fail<int(kFiles.size());++fail){
    Backend b;b.requested=true;b.fail_at=fail;
    for(const char* path:kFiles)b.files.insert(path);
    b.files.insert("/connect/captures/keep.wxc");
    assert(recover(b)==Recovery::Failed&&b.requested&&b.settings);
    b.fail_at=-1;assert(recover(b)==Recovery::Complete&&!b.requested&&!b.settings);
    assert(b.files.size()==1&&b.files.count("/connect/captures/keep.wxc"));
  }
  Backend b;b.requested=true;b.mount_ok=false;
  assert(recover(b)==Recovery::Failed&&b.requested&&b.settings);
  b.mount_ok=true;b.clear_ok=false;
  assert(recover(b)==Recovery::Failed&&b.requested&&b.settings);
  b.clear_ok=true;b.complete_ok=false;
  assert(recover(b)==Recovery::Failed&&b.requested&&!b.settings);
  b.complete_ok=true;assert(recover(b)==Recovery::Complete&&!b.requested);
  std::cout<<"Factory reset checks passed: confirmation, hold, interruption, retry and project-only deletion.\n";
}

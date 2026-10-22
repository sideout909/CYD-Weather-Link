#include "update_policy.h"
#include <cassert>
#include <iostream>
int main(){
  using namespace update_policy;
  assert(migratedProjectRepository("", "sideout909/WX-Link")=="sideout909/WX-Link");
  assert(migratedProjectRepository("sideout909/CYD-Weather-Link", "sideout909/WX-Link")=="sideout909/WX-Link");
  assert(migratedProjectRepository("Sideout909/CYD-Weather-Link", "sideout909/WX-Link")=="sideout909/WX-Link");
  assert(migratedProjectRepository("owner/custom-fork", "sideout909/WX-Link")=="owner/custom-fork");
  assert(repository("owner/CYD-Weather-Link"));
  for(const char* bad:{"","owner","/repo","owner/","owner/repo/more","../repo","owner/..","owner/repo?token=SECRET","https://github.com/owner/repo"})assert(!repository(bad));
  assert(newer("0.3.1","0.3.0"));assert(newer("1.0.0","0.99.99"));
  assert(!newer("0.3.0","0.3.0"));assert(!newer("0.2.9","0.3.0"));
  assert(newer("0.8.0","0.8.0-rc.1"));assert(newer("0.8.0-rc.2","0.8.0-rc.1"));
  assert(newer("0.8.0-rc.1","0.7.5"));assert(!newer("0.8.0-rc.1","0.8.0"));
  assert(!newer("0.8.0-rc.1","0.8.0-rc.1"));
  std::array<unsigned,3> parts{};
  assert(version("0.8.0-rc.1",parts));
  for(const char* bad:{"0.8.0-rc.","0.8.0-rc.0","0.8.0-rc.-1","0.8.0-rc.65536","0.8.0-rc.1/evil"})assert(!version(bad,parts));
  assert(firmwareUrl("https://github.com/owner/repo/releases/download/v0.8.0-rc.1/weather-link-panellan_sc05_x.bin","owner/repo"));
  for(const char* bad:{"0.3","0.3.0-beta","v0.3.0","0.3.0.1","-1.0.0","65536.0.0","0..0"})assert(!version(bad,parts));
  assert(sha256(std::string(64,'a')));assert(!sha256(std::string(63,'a')));assert(!sha256(std::string(64,'z')));
  assert(firmwareUrl("https://github.com/owner/repo/releases/download/v0.3.1/weather-link-panellan_sc05_x.bin","owner/repo"));
  assert(firmwareUrl("https://github.com/Owner/Repo/releases/download/v0.3.1/weather-link-panellan_sc05_x.bin","owner/repo"));
  assert(!firmwareUrl("https://github.com/attacker/repo/releases/download/v0.3.1/weather-link-panellan_sc05_x.bin","owner/repo"));
  assert(!firmwareUrl("https://github.com/owner/repo/releases/download/../file.bin","owner/repo"));
  assert(!firmwareUrl("https://github.com/owner/repo/releases/download/v0.3.1/weather-link-panellan_sc05_x.bin","owner/repo","0.3.2"));
  assert(!firmwareUrl("https://github.com/owner/repo/releases/download/%2e%2e/weather-link-panellan_sc05_x.bin","owner/repo"));
  assert(downloadHost("https://release-assets.githubusercontent.com/a?signature=example"));
  assert(!downloadHost("http://github.com/owner/repo"));assert(!downloadHost("https://github.com.attacker.test/file"));
  assert(!downloadHost("https://github.com@attacker.test/file"));assert(!downloadHost("https://attacker.test/file"));
  std::cout<<"Update policy checks passed: repository, version ordering, digest, release ownership and HTTPS redirect hosts.\n";
}

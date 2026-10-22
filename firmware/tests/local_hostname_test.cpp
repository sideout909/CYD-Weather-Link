#include "local_hostname.h"
#include <cassert>
#include <iostream>
int main(){
  for(const char* name:{"wxlink","wx-link-2","a","0wx"})assert(local_hostname::valid(name));
  for(const char* bad:{"","-wx","wx-","wx.local","WXLink","a_b","123","a/b","a b","a\r\n"})assert(!local_hostname::valid(bad));
  assert(local_hostname::valid(std::string(32,'a')));assert(!local_hostname::valid(std::string(33,'a')));
  assert(local_hostname::url("wx-link-2")=="http://wx-link-2.local");
  std::cout<<"Local hostname checks passed: DNS label safety, length and URL.\n";
}

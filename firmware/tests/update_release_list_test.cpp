#include "update_release_list.h"
#include <sstream>
#include <cassert>
#include <iostream>
int main(){
  auto best=[](const std::string& json){std::istringstream input(json);std::string version;return update_release_list::newest(input,version)?version:std::string();};
  assert(best(R"([
    {"tag_name":"v0.9.0-rc.2","draft":false,"prerelease":true,"body":"Ignored text","assets":[{"url":"https://attacker.invalid"}]},
    {"tag_name":"v0.9.0-rc.1","draft":false,"prerelease":true},
    {"tag_name":"v0.8.2","draft":false,"prerelease":false},
    {"tag_name":"v9.0.0","draft":true,"prerelease":false},
    {"tag_name":"v8.0.0","draft":false,"prerelease":true},
    {"tag_name":"v7.0.0-rc.1","draft":false,"prerelease":false},
    {"tag_name":"v6.0.0","draft":"false","prerelease":false},
    {"tag_name":"v5.0.0","prerelease":false}
  ])")=="0.9.0-rc.2");
  assert(best(R"([{"tag_name":"v0.9.0-rc.2","draft":false,"prerelease":true},{"tag_name":"v0.9.0","draft":false,"prerelease":false}])")=="0.9.0");
  for(const char* bad:{"[]","{}","null","garbage",R"({"message":"API rate limit"})",R"([{"tag_name":"evil","draft":false,"prerelease":false}])",R"([42,"bad",null])"})assert(best(bad).empty());
  std::string many="[";for(unsigned i=0;i<11;++i){if(i)many+=',';many+=R"({"tag_name":"v0.9.0","draft":false,"prerelease":false})";}many+=']';assert(best(many).empty());
  assert(best("[{\"tag_name\":\""+std::string(5000,'x')+"\",\"draft\":false,\"prerelease\":false}]").empty());
  std::cout<<"Dev release list checks passed: actual filtered JSON, newest Stable/RC ordering, drafts, type mismatches, malformed/oversized lists.\n";
}

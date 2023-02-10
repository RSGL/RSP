#include <vector>
#include <algorithm>
#include <map>


enum tokenType {open, close, key, value};

struct token{
  tokenType t;
  std::string data;
};


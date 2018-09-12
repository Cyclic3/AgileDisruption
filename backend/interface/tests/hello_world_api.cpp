#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <iostream>

using namespace agiledisruption;

api a;

json print(const json& js) {
  std::cout << static_cast<std::string>(js) << std::endl;
  return "world";
}

int main(int argc, char *argv[]) {
  a.add("print", print);

  std::string ret = (*a.get("print"))("Hello");

  std::cout << ret << std::endl;
  return 0;
}

#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <iostream>

using namespace agiledisruption;

auto a = std::make_shared<api>();

json print(const json& js) {
  std::cout << static_cast<std::string>(js) << std::endl;
  return "world";
}

std::string request_fifo_path = "/tmp/agiledisruption.interface.test.fifo_api";

int main(int argc, char *argv[]) {
  a->add("print", print);

  auto server = channel_server::fifo(request_fifo_path);
  auto client = channel_client::fifo(request_fifo_path);

  server->bind(a);

  auto future = client->request("print", { "hello" });

  std::string ret = *future.get();

  std::cout << ret << std::endl;
  return 0;
}

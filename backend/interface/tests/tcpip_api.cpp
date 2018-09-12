#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <iostream>

using namespace agiledisruption;

static auto a = std::make_shared<api>();

json print(const json& js) {
  std::cout << static_cast<std::string>(js["message"]) << std::endl;
  std::string ret = "world";
  return ret;
}

static uint16_t request_tcpip_port = 42069;

int main() {
  a->add("print", print);

  auto server = channel_server::tcp_ip(request_tcpip_port);
  auto client = channel_client::tcp_ip(request_tcpip_port);

  server->bind(a);

  auto future = client->request("print", json::object({ { "message", "hello" } }));

  future.wait();

  auto ret_json = future.get().value();

  std::string ret = ret_json;

  std::cout << ret << std::endl;
  return 0;
}

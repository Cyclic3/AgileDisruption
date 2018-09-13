#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <iostream>

constexpr uint16_t request_tcpip_port = 42070;

namespace agiledisruption::tests::store_fetch::server {
  static std::unordered_map<uint64_t, json> memory;
  static std::atomic<uint64_t> next_id = 0;

  AGILEDISRUPTION_EXPOSE_INTERFACE;

  AGILEDISRUPTION_DEFINE_INTERFACE(
    { "store", [](json js) -> json { memory[js["id"]] = js["data"]; return nullptr; } },
    { "fetch", [](json js) -> json {
      auto iter = memory.find(js["id"]);
      if (iter == memory.end()) return { nullptr }; else return iter->second;
    }},
    {"next", [](json) -> json { return next_id++; }}
  );
}

namespace agiledisruption::tests::store_fetch::client {
  class data {
  private:
    uint64_t _id = {};
    std::shared_ptr<channel_client> _client;

  public:
    json fetch() { return _client->call("fetch", { { "id", _id } }).value(); }
    void store(const json& js) { _client->call("store", { { "id", _id }, { "data", js }}); }

  public:
    operator json() { return fetch(); }
    data& operator=(const json& js) { store(js); return *this; }

  public:
    data() : _client(channel_client::tcp_ip(request_tcpip_port)) {
      _id = _client->call("next", {}).value();
    }
  };
}

using namespace agiledisruption;

int main() {
  auto server = channel_server::tcp_ip(request_tcpip_port);
  server->bind(tests::store_fetch::server::interface);

  agiledisruption::tests::store_fetch::client::data c;

  c = "Hello, world!";
  json val1 = c;
  std::cout << val1 << std::endl;
  if (val1 != "Hello, world!") throw std::runtime_error("First value was inconsistent");

  c = "Goodbye!";
  json val2 = c;
  std::cout << val2 << std::endl;
  if (val2 != "Goodbye!") throw std::runtime_error("Second value was inconsistent");

  return 0;
}

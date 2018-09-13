#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <iostream>

constexpr uint16_t request_tcpip_port = 42070;

namespace agiledisruption::tests::calculator::server {
  std::unordered_map<uint64_t, double> memory;

  json add(json js) {
    memory[js["id"]] += static_cast<double>(js["value"]);
    return nullptr;
  }

  json subtract(json js) {
    memory[js["id"]] -= static_cast<double>(js["value"]);
    return nullptr;
  }

  json set(json js) {
    memory[js["id"]] = static_cast<double>(js["value"]);
    return nullptr;
  }

  json multiply(json js) {
    memory[js["id"]] *= static_cast<double>(js["value"]);
    return nullptr;
  }

  json divide(json js) {
    memory[js["id"]] /= static_cast<double>(js["value"]);
    return nullptr;
  }

  json get(json js) {
    return memory[js["id"]];
  }

  json next_id(json) {
    static std::atomic<uint64_t> id = 0;
    return id++;
  }

  AGILEDISRUPTION_EXPOSE_INTERFACE;

  AGILEDISRUPTION_DEFINE_INTERFACE(
    { "*", multiply },
    { "/", divide },
    { "+", add },
    { "-", subtract },
    { "=", set },
    { "?", get },
    { "n", next_id}
  );
}

namespace agiledisruption::tests::calculator::client {
  class calc {
  private:
    uint64_t _id = {};
    std::shared_ptr<channel_client> _client;

  public:
    operator double() { return get(); }

  public:
    void add(double d) { _client->call("+", { { "id", _id }, { "value", d } }); }
    void subtract(double d) { _client->call("-", { { "id", _id }, { "value", d } }); }
    void multiply(double d) { _client->call("*", { { "id", _id }, { "value", d } }); }
    void divide(double d) { _client->call("/", { { "id", _id }, { "value", d } }); }
    double get() { return _client->call("?", { { "id", _id } }).value(); }
    void set(double d) { _client->call("=", { { "id", _id }, { "value", d } }); }

  public:
    calc() : _client(channel_client::tcp_ip(request_tcpip_port)) {
      _id = _client->call("n", {}).value();
    }
  };
}

using namespace agiledisruption;

int main() {
  auto server = channel_server::tcp_ip(request_tcpip_port);
  server->bind(tests::calculator::server::interface);

  agiledisruption::tests::calculator::client::calc c;

  c.set(4);
  c.multiply(100);
  c.add(40);
  c.subtract(20);

  if (c.get() != 420) throw std::runtime_error("Got incorrect result");
  return 0;
}

#pragma once
#include "agiledisruption/internal/json.hpp"

#include <unordered_map>
#include <functional>
#include <string>
#include <optional>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <thread>

namespace agiledisruption {
  using json = nlohmann::json;

  class api {
  public:
    using handler = std::function<json(json)>;
  private:
    std::unordered_map<std::string, handler> _api_calls;
    mutable std::shared_mutex _api_calls_mutex;

  public:
    inline void add(std::string name, handler handler) {
      std::unique_lock lock(_api_calls_mutex);
      _api_calls.insert_or_assign(name, handler);
    }
    inline void remove(std::string name) {
      std::unique_lock lock(_api_calls_mutex);
      _api_calls.erase(name);
    }

    inline std::optional<handler> get(std::string name) const {
      std::shared_lock lock(_api_calls_mutex);

      auto iter = _api_calls.find(name);

      if (iter == _api_calls.end()) return std::nullopt;
      else return { iter->second };
    }

    json handle(const json&);
  };

  class channel_server {
  public:
    virtual void bind(const api&);
    virtual void unbind();

  public:
    static std::shared_ptr<channel_server> tcp_ip(std::string ip, uint16_t port);
    static std::shared_ptr<channel_server> fifo(std::string path);

  public:
    virtual ~channel_server() = default;
  };

  class channel_client {
  public:
    virtual json request(std::string op, json);
    virtual void submit(std::string op, json);

  public:
    static std::shared_ptr<channel_client> tcp_ip(std::string ip, uint16_t port);
    static std::shared_ptr<channel_client> fifo(std::string path);

  public:
    virtual ~channel_client() = default;
  };
}

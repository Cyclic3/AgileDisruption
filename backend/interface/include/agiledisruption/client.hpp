#pragma once
#include "agiledisruption/internal/common.hpp"

#include <string>
#include <future>

namespace agiledisruption {
  class channel_client {
  public:
    virtual std::future<std::optional<json>> request(const std::string& op, const json&) = 0;
    inline std::optional<json> call(const std::string& op, const json& js) {
      auto f = request(op, js);
      f.wait();
      return f.get();
    }

  public:
    static std::unique_ptr<channel_client> tcp_ip(uint16_t port);
    //static std::unique_ptr<channel_client> fifo(const std::string& path);

  public:
    virtual ~channel_client() = default;
  };
}

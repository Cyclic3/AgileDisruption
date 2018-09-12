#pragma once
#include "agiledisruption/internal/common.hpp"

#include <string>
#include <future>

namespace agiledisruption {
  class channel_client {
  public:
    virtual std::future<std::optional<json>> request(std::string op, const json&) = 0;

  public:
    static std::unique_ptr<channel_client> tcp_ip(uint16_t port);
    //static std::unique_ptr<channel_client> fifo(const std::string& path);

  public:
    virtual ~channel_client() = default;
  };
}

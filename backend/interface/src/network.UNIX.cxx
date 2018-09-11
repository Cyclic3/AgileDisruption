/*
#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>

constexpr size_t WINDOW_SIZE = 65536;

namespace agiledisruption {
  class unix_tcpip_server : channel_server {
  public:
    class client {
    private:
      std::string current_str;
      int fd;

    public:
      std::optional<std::string> get_window() {
        auto str_end = current_str.c_str();
        ::read(fd, current_str, )
      }
    };

  private:
    std::shared_ptr<api> bound;
    std::shared_mutex bound_mutex;

    std::thread worker;
    std::atomic<bool> keep_working;

  public:
    void bind(std::shared_ptr<api> b) override {
      std::unique_lock lock(bound_mutex);
      bound = b;
      worker = std::thread{ &unix_tcpip::worker, this };
    }

    void unbind() override {
      std::unique_lock lock(bound_mutex);
      bound = nullptr;
      worker.join();
    }

    void client_handler_body(int fd) {
      while (bound) {
        std::shared_lock lock(bound_mutex);
        // do some receive stuff until null
        std::string data;

        ::read(fd, )

        auto js = json::parse(data);

        std::string ret = bound->handle(js).dump();

        // send the return value, even if empty
      }
    }

    void worker_body() {
      while (keep_working) {

      }
    }
  };
}
*/

#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

constexpr size_t WINDOW_SIZE = 65536;
constexpr size_t BACKLOG = 128;

static uint8_t LOOPBACK_IP[4] = { 127, 69, 4, 20 };

// TODO: maybe implement max_size to prevent slow loris-esque bugs?

namespace agiledisruption {
  sockaddr_in loopback_ep(uint16_t port) {
    sockaddr_in ret;
    ::memcpy(&ret.sin_addr.s_addr, LOOPBACK_IP, sizeof(LOOPBACK_IP));
    ::memset(ret.sin_zero, 0, sizeof(ret.sin_zero));
    ret.sin_family = AF_INET;
    ret.sin_port = htons(port);
    return ret;
  }

  class unix_tcpip_server : public channel_server {
  private:
    std::shared_ptr<const api> base = nullptr;
    std::shared_mutex base_mutex = {};

    std::thread worker = {};
    std::atomic<bool> keep_working = false;

    uint16_t port;
    int fd = -1;

  public:
    void bind(std::shared_ptr<const api> b) override {
      std::unique_lock lock{ base_mutex };
      keep_working = false;
      ::shutdown(fd, SHUT_RDWR);
      if (worker.joinable()) worker.join();
      fd = -1;

      try {
        if ((fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
          throw std::runtime_error("Could not create socket");
        {
          auto ep = loopback_ep(port);
          // Allow the port to be used again quickly
          // We don't care if this fails, it's just for convenience
          int one = 1;
          setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

          if (
            ::bind(fd, reinterpret_cast<const sockaddr*>(&ep), sizeof(ep)) ||
            ::listen(fd, BACKLOG)
          ) throw std::runtime_error("Could not bind socket");
        }
      }
      catch(...) { ::close(fd); throw; }

      base = b;
      keep_working = true;
      worker = std::thread{ &unix_tcpip_server::worker_body, this };

    }

    void unbind() override {
      std::unique_lock lock{ base_mutex };
      keep_working = false;
      ::shutdown(fd, SHUT_RDWR);
      if (worker.joinable()) worker.join();
      fd = -1;
      base = nullptr;
    }

    void worker_body() {
      std::shared_mutex wait_for_me;

      while (keep_working) {
        // Accept the next client
        int client = ::accept(fd, nullptr, nullptr);

        // If we failed, skip this loop and try again
        if (client < 0) continue;

        // If we succeeded, spawn us a new thread to handle this client
        std::shared_lock lock{wait_for_me};

        // We need a copy of client, as the loop's value will change next iteration
        std::thread([this, client, lock{std::move(lock)}](){
          (void) lock;

          try {
            std::string current_msg;
            char buffer[WINDOW_SIZE];

            // Read until null
            do {
              ssize_t n_read = ::read(client, buffer, WINDOW_SIZE);
              // If we hit an error or need to stop, give up
              if (n_read < 0 || !keep_working) throw 0;
              else if (n_read == 0) std::this_thread::yield();
              current_msg.append(buffer, static_cast<size_t>(n_read));
            }
            while (current_msg.size() == 0 || current_msg.back());

            current_msg.pop_back();

            auto js = json::parse(current_msg);

            auto payload = js.find("payload");
            auto op = js.find("op");

            // Check we have a valid message
            // We will just drop it if it's invalid
            if (
              payload == js.end() ||
              op == js.end()
            ) throw 0;

            json response;

            {
              std::shared_lock base_lock{base_mutex};
              if (auto handler = base->get(*op))
                response["payload"] = (*handler)(*payload);
            }

            std::string response_str = response.dump();

            {
              const char* data = response_str.c_str();
              size_t remaining = response_str.size() + 1;
              do {
                ssize_t written = write(client, data, remaining);
                if (written < 0) throw 0;
                data += written;
                remaining -= static_cast<size_t>(written);
              }
              while (remaining);
            }
          }
          catch (...) {}

          ::close(client);
        }).detach();
      }

      wait_for_me.lock();
      wait_for_me.unlock();
      ::close(fd);
    }

  public:
    unix_tcpip_server(uint16_t port) : port(port) {}
    ~unix_tcpip_server() override { unbind(); }
  };

  class unix_tcpip_client : public channel_client {
  public:
    uint16_t port;

    std::shared_mutex wait_for_me = {};

    std::atomic<bool> keep_working = true;

  public:
    std::future<std::optional<json>> request(std::string op, const json& js) override {
      std::shared_lock lock{wait_for_me};

      std::promise<std::optional<json>> promise;
      auto ret = promise.get_future();

      json request_js = json::object();
      request_js["op"] = op;
      request_js["payload"].update(js);

      // Do this here, because the json library is weird about move constructors
      std::string request_str = request_js.dump();

      std::thread{[this, request_str{std::move(request_str)}, promise{std::move(promise)}, lock{std::move(lock)}]() mutable {
        (void) lock;

        auto ep = loopback_ep(port);
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        try {
          if (fd < 0) throw 0;

          if (auto code = ::connect(fd, reinterpret_cast<const sockaddr*>(&ep), sizeof(ep)))
            throw 0;

          {
            const char* data = request_str.c_str();
            size_t remaining = request_str.size() + 1;
            do {
              ssize_t written = write(fd, data, remaining);
              if (written < 0) throw 0;
              data += written;
              remaining -= static_cast<size_t>(written);
            }
            while (remaining);
          }

          std::string response_str;
          char buffer[WINDOW_SIZE];

          // Read until null
          do {
            ssize_t n_read = ::read(fd, buffer, WINDOW_SIZE);
            // If we hit an error or need to stop, give up
            if (n_read < 0 || !keep_working) throw 0;
            else if (n_read == 0) std::this_thread::yield();
            else response_str.append(buffer, static_cast<size_t>(n_read));
          }
          while (response_str.size() == 0 || response_str.back());
          response_str.pop_back();

          // Parse the json and work out what to do with it
          json response = json::parse(response_str);

          auto maybe_data = response.find("payload");

          if (maybe_data == response.end())
            promise.set_value(std::nullopt);
          else
            promise.set_value(std::move(*maybe_data));
        }
        catch(...) { promise.set_value(std::nullopt); }

        ::close(fd);
      }}.detach();

      return ret;
    }

  public:
    unix_tcpip_client(uint16_t port) : port(port) {}
    ~unix_tcpip_client() override {
      wait_for_me.lock();
      wait_for_me.unlock();
    }
  };

  std::unique_ptr<channel_server> channel_server::tcp_ip(uint16_t port) {
    return std::make_unique<unix_tcpip_server>(port);
  }

  std::unique_ptr<channel_client> channel_client::tcp_ip(uint16_t port) {
    return std::make_unique<unix_tcpip_client>(port);
  }
}

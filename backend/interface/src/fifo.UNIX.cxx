/*
#include "agiledisruption/server.hpp"
#include "agiledisruption/client.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <fstream>
#include <thread>
#include <shared_mutex>
#include <memory>
#include <atomic>

namespace agiledisruption {
  std::optional<std::string> poll_fifo_json(FILE* fifo, std::atomic<bool>& keep_working) {
    std::string buffer;

    while (keep_working) {
      int c = ::fgetc(fifo);

      if (c == EOF) std::this_thread::yield();
      else if (c == 0) return buffer;
      else buffer.push_back(static_cast<char>(c));
    }

    return std::nullopt;
  }

  void lock_fd()

  class unix_fifo_server : public channel_server {
  public:
    std::shared_ptr<const api> base = nullptr;
    std::shared_mutex base_mutex = {};

    FILE* request_fifo = {};
    std::string request_path;

    std::thread worker = {};
    std::atomic<bool> keep_working = false;

  public:
    void stop_working() {
      keep_working = false;
      if (worker.joinable())
        worker.join();
    }

    void start_working() {
      keep_working = true;
      worker = std::thread{&unix_fifo_server::worker_body, this};
    }

    void bind(std::shared_ptr<const api> a) override {
      stop_working();

      std::unique_lock lock{base_mutex};
      base = a;

      start_working();
    }

    void unbind() override {
      std::unique_lock lock{base_mutex};
      base.reset();
    }

    // Not resiliant to unterminated strings or slow writes
    void worker_body() {
      std::shared_mutex wait_for_me;

      while (auto current_msg = poll_fifo_json(request_fifo, keep_working)) {
        // Now we have our data, we can process it elsewhere
        // and continue reading from the fifo
        std::shared_lock lock{wait_for_me};
        std::thread([&, current_msg](){
          try {
            std::shared_lock our_lock = std::move(lock);

            auto js = json::parse(*current_msg);

            auto response_path = js.find("response_path");
            auto payload = js.find("payload");
            auto id = js.find("id");
            auto op = js.find("op");

            // Check we have a valid message
            // We will just drop it if it's invalid
            if (
              response_path == js.end() ||
              payload == js.end() ||
              id == js.end() ||
              op == js.end()
            ) return;

            json response;

            {
              std::shared_lock base_lock{base_mutex};
              if (auto handler = base->get(*op))
                response["payload"] = (*handler)(*payload);
            }

            response["id"] = *id;

            std::ofstream response_fifo{static_cast<std::string>(*response_path)};

            response_fifo << response << '\0';
          }
          catch (...) {}
        }).detach();
      }

      wait_for_me.lock();
    }

  public:
    unix_fifo_server(const std::string& path) : request_path(path) {
      // We don't care if this fails
      ::unlink(request_path.c_str());

      if (::mkfifo(request_path.c_str(), 0600))
        throw std::invalid_argument{"Could not create fifo"};

      request_fifo = ::fopen(request_path.c_str(), "rb");
    }

    ~unix_fifo_server() override {
      ::fclose(request_fifo);
      ::unlink(request_path.c_str());
    }
  };

  class unix_fifo_client : public channel_client {
  public:
    std::ifstream response_fifo = {};
    std::ofstream request_fifo;
    std::string tmpdir = "/tmp/AgileDisruption.Client.XXXXXX";
    std::string response_path = {};

    std::unordered_map<uint64_t, std::promise<std::optional<json>>> waiting_for = {};
    std::shared_mutex waiting_for_mutex = {};
    // If there are > 2^64 simultaneous requests, we will have bigger problems...
    // This would take > 3500 years if this counter was just incremented without waiting
    std::atomic<uint64_t> next_id = 0;

    std::atomic<bool> keep_working = true;
    std::thread worker = {};

  public:
    void worker_body() {
      while (auto current_msg = poll_fifo_json(request_fifo, keep_working)) {
        std::string raw_json;
        std::getline(response_fifo, raw_json, '\0');
        auto js = json::parse(raw_json);

        auto id = js.find("id");
        if (id != js.end()) {
          auto maybe_payload = js.find("payload");

          std::unique_lock lock{waiting_for_mutex};
          auto iter = waiting_for.find(*id);
          if (iter != waiting_for.end()) {
            if (maybe_payload == js.end())
              iter->second.set_value(std::nullopt);
            else
              iter->second.set_value(*maybe_payload);
          }
        }
      }
    }

    std::future<std::optional<json>> request(std::string op, const json& js) override {
      json to_send;

      auto id = next_id++;
      auto promise = std::promise<std::optional<json>>{};

      auto ret = promise.get_future();

      to_send["response_path"] = response_path;
      to_send["id"] = id;
      to_send["op"] = op;
      to_send["payload"] = js;

      request_fifo << to_send << '\0';

      waiting_for.insert_or_assign(id, std::move(promise));

      return ret;
    }

  public:
    unix_fifo_client(const std::string& path) : request_fifo(path) {
      char* fifo = &tmpdir[0];

      ::mkdtemp(fifo);

      response_path = tmpdir + "/pipe";

      ::mkfifo(response_path.c_str(), 0600);

      worker = std::thread{&unix_fifo_client::worker_body, this};
    }

    ~unix_fifo_client() override {
      ::unlink(response_path.c_str());
      ::rmdir(tmpdir.c_str());
    }
  };

  std::unique_ptr<channel_server> channel_server::fifo(const std::string& path) {
    return std::make_unique<unix_fifo_server>(path);
  }

  std::unique_ptr<channel_client> channel_client::fifo(const std::string& path) {
    return std::make_unique<unix_fifo_client>(path);
  }
}
*/

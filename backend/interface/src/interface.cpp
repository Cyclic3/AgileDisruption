#include "agiledisruption/interface.hpp"

namespace agiledisruption {
  json api::handle(const json& js) {
    json ret;

    auto op = js.find("op");
    auto id = js.find("id");

    // With no id, there is nothing we can do
    if (id == js.end()) return {};

    bool valid = (op != js.end());

    ret["valid"] = valid;

    ret["id"] = *id;

    if (valid) {
      if (auto func = get(*op)) {
        ret["found"] = true;
        ret["result"] = (*func)(js);
      }
      else ret["found"] = false;
    }

    return ret;
  }
}

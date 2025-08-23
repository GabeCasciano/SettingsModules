#ifndef SETTING_H_
#define SETTING_H_

#include "SQLiteWrapper/include/SQL_Datatypes.h"
#include "SQLiteWrapper/include/SQL_Value.h"
#include <algorithm>
#include <nlohmann/json_fwd.hpp>
#include <sqlite3.h>

#ifndef ARDUNIO
#include <format>
#include <nlohmann/json.hpp>
#else
#include <Arduino>
#endif

struct Setting_t {
  std::string name;
  SqlValue value;

  Setting_t() = default;
  Setting_t(std::string name, SqlValue value) : name(name), value(value) {}
  ~Setting_t() { destroy(); }

  // copy
  Setting_t(const Setting_t &other) { copy_from(other) };
  // copy assignment
  Setting_t &operator=(const Setting_t &other) {
    if (this != &other) {
      copy_from(other);
    }
    return *this;
  }

  // move
  Setting_t(Setting_t &&other) noexcept { move_from(std::move(other)); }
  // move assignment
  Setting_t &operator=(Setting_t &&other) noexcept {
    if (this != &other) {
      move_from(std::move(other));
    }
    return *this;
  }

  const char *toString() {
    str = std::format("Setting_t (name: {}, value:{})", name, value.toString());
    return str.c_str();
  }

  Row_t toRow() {
    Row_t r(3);
    r.values[0] = name.c_str();
    r.values[1] = value;
    r.values[2] = value.type();
    return r;
  }

  nlohmann::json toJson() {
    nlohmann::json j;
    j["name"] = name.c_str();
    j["value"] = value.toString();
    j["dType"] = value.type();
    return j;
  }

  static Setting_t fromRow(Row_t r) {
    if (r.colCount != 3)
      return Setting_t();

    SqlValue v;
    switch (r.values[3].as_int()) {
    case SqlValue::Type::Null:
      v = nullptr;
      break;
    case SqlValue::Type::Integer:
      v = r.values[1].as_int();
      break;
    case SqlValue::Type::Real:
      v = r.values[1].as_real();
      break;
    case SqlValue::Type::Text:
      v = r.values[1].as_text();
      break;
    }
    return Setting_t(r.values[0].as_text(), v);
  }

  static Setting_t fromJson(nlohmann::json j) {
    SqlValue::Type dType = j["dType"];
    switch (dType) {
    case SqlValue::Type::Null:
      return Setting_t(j["name"], nullptr);
    case SqlValue::Type::Integer:
      return Setting_t(j["name"], (int64_t)j["value"]);
    case SqlValue::Type::Real:
      return Setting_t(j["name"], (double)j["value"]);
    case SqlValue::Type::Text: {
      std::string _str = j["value"];
      return Setting_t(j["name"], _str.c_str());
    }
    case SqlValue::Type::Blob: {
      std::string _str = j["value"];
      return Setting_t(j["name"], SqlValue(&j["value"], _str.length()));
    }
    }
  }

private:
  std::string str;

  void destroy() {
    name = "";
    value = SqlValue();
    str = "";
  }

  void copy_from(const Setting_t &o) {
    destroy();
    name = o.name;
    value = o.value;
    str = o.str;
  }

  void move_from(Setting_t &&o) noexcept {
    destroy();
    name = std::move(o.name);
    value = std::move(o.value);
    str = std::move(o.str);
    o.destroy();
  }
};
#endif

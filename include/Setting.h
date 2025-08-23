#ifndef SETTING_H_
#define SETTING_H_

#include "SQLiteWrapper/include/SQL_Datatypes.h"
#include "SQLiteWrapper/include/SQL_Value.h"
#include <nlohmann/json_fwd.hpp>
#include <sqlite3.h>

#ifndef ARDUNIO
#include <format>
#include <nlohmann/json.hpp>
#else
#include <Arduino>
#endif

struct Setting {
  std::string name;
  SqlValue value;

  Setting(std::string name, SqlValue value) : name(name), value(value) {}

  Setting() = default;
  ~Setting() = default;

  const char *toString() {
    str = std::format("Setting (name: {}, value:{})", name, value.toString());
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
    j["name"] = name;
    j["value"] = value.toString();
    j["dType"] = value.type();
    return j;
  }

  static Setting fromRow(Row_t r) {
    if (r.colCount != 3)
      return Setting();

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
    return Setting(r.values[0].as_text(), v);
  }

private:
  std::string str;
};
#endif

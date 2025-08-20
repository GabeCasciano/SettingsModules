#ifndef SETTING_H_
#define SETTING_H_

#include "SQLiteWrapper/include/SQL_Value.h"
#include <sqlite3.h>

#ifndef ARDUNIO
#include <cstring>
#include <exception>
#include <format>
#include <stdexcept>
#include <variant>
#include <vector>
#else 
#include <Arduino>
#endif

inline const char *GET_ALL_STR = "SELECT name, value, dType FROM settings;";

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

private:
  std::string str;
};
#endif

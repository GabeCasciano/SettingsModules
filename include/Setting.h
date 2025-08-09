#ifndef SETTING_H_
#define SETTING_H_

#include <SQL_Wrapper.h>
#include <exception>
#include <format>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

inline const char *GET_ALL_STR = "SELECT name, value, dType FROM settings;";

struct Setting {
  std::string name;
  SqlValue value;

  Setting(std::string name, SqlValue value) : name(name), value(value) {}

  Setting() = default;
  ~Setting() = default;

  std::string toString() const {
    return std::format("Setting (name: {}, value:{})", name,
                       sqlValueToString(value));
  }

  std::vector<std::pair<std::string, SqlValue>> toDataObj() const {
    std::vector<std::pair<std::string, SqlValue>> sql;
    sql.push_back(std::make_pair("name", name));
    sql.push_back(std::make_pair("value", value));
    return sql;
  }
};
#endif

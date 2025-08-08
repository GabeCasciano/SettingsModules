#ifndef SETTING_H_
#define SETTING_H_

#include <concepts>
#include <cstdlib>
#include <exception>
#include <format>
#include <stdexcept>
#include <string>

enum DataType_t {
  Null = 0,
  Integer = 1,
  Real = 2,
  Text = 3,
  Blob = 4,
  Boolean = 10,
  Timestamp = 11,
  Enum = 12,
};

struct Setting {
  std::string name;
  std::string value;
  short dType;

  Setting(std::string name, std::string value, DataType_t dType)
      : name(name), value(value), dType(dType) {}
  Setting() = default;
  ~Setting() = default;

  std::string createOrReplace() {
    return std::format("INSERT OR REPLACE INTO settings (key, value, dType) "
                       "VALUES ('{}', '{}', {});",
                       name, value, dType);
  }
};

#endif

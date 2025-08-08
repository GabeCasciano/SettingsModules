#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Setting.h"
#include <concepts>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <sqlite3.h>

class Settings {

  Settings(const char * filename) {

  }
  ~Settings() {}

  template <typename T> inline void addSetting(std::string name, T value) {
    if constexpr (!std::is_integral_v<T>)
      throw std::runtime_error("Non integral data type");

    DataType_t dt;
    if constexpr (std::same_as<T, int>)
      dt = Integer;
    else if constexpr (std::same_as<T, float> || std::same_as<T, double>)
      dt = Real;
    else if constexpr (std::same_as<T, std::string>)
      dt = Text;

    settings.push_back(new Setting(name, std::format("{}", value), dt));

  }

  inline void getSettings() {}

private:
  std::vector<Setting *> settings;
};
#endif

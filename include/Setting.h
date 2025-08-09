#ifndef SETTING_H_
#define SETTING_H_

#include <exception>
#include <sqlite3.h>
#include <format>
#include <stdexcept>
#include <vector>
#include <string>

inline const char *CHECK_TABLE_STR = "SELECT name "
                                     "FROM sqlite_master "
                                     "WHERE type='table' AND name='settings';";

inline const char *GET_ALL_STR = "SELECT name, value, dType FROM settings;";

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

  std::string toString() {
    return std::format("Setting (name: {}, value:{}, dType:{})", name, value,
                       dType);
  }
};

template <typename T>
inline void addSetting(sqlite3 *db, std::string name, T value){

}

inline Setting getSettingByName(std::vector<Setting> settings, std::string name){
  for(auto s : settings){
    if(s.name == name)
      return s;
  }
  throw std::runtime_error("Setting not found");
}




#endif

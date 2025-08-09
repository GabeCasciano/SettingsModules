#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "SQL_Wrapper.h"
#include "Setting.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

const char *TABLE_NAME = "settings";
const char *DEFAULT_DB = "settings.db";

using Settings_t = std::vector<Setting>;

inline SqlValue getValueBySettingName(Settings_t settings, std::string name) {
  for (auto s : settings) {
    if (s.name == name)
      return s.value;
  }
  return nullptr;
}

class Settings {

  Settings() { sql = new SQL_Wrapper(DEFAULT_DB); }
  ~Settings() { free(sql); }

  /** @function addSetting
   * @brief use this function to add a new setting to the list, if the setting
   * exists this will overwrite it
   * @param name - the name of the setting
   * @param value - templated typename value, for the setting
   */
  inline void addUpdateSetting(std::string name, SqlValue value) {
    // Create setting
    Setting _setting(name, value);

    // Add to db
    sql->insertInto(TABLE_NAME, _setting.toDataObj());
  }

  inline Settings_t getSettingsFromDb() {

    Settings_t settings;

    Table table = sql->selectAllFromTable(TABLE_NAME);

    for (size_t nSet = 0; nSet < table[0].second.size() - 1; ++nSet) {
      Setting _setting(std::get<std::string>(table[0].second[nSet]),
                       table[1].second[nSet]);
      settings.push_back(std::move(_setting));
    }
    return settings;
  }

private:
  SQL_Wrapper *sql;
};

#endif

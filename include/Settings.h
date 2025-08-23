#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "SQLiteWrapper/include/SQL_Datatypes.h"
#include "SQLiteWrapper/include/SQL_Wrapper.h"
#include "Setting.h"
#include <string>

static const char *TABLE_NAME = "settings";
static const char *DEFAULT_DB = "settings.db";

struct Settings_t {
  uint32_t count;
  Setting *settings;

  Settings_t(uint32_t count) : count(count) {
    settings = (Setting *)malloc(sizeof(Setting) * count);
  }
};

inline SqlValue getValueBySettingName(Settings_t settings, std::string name) {
  for (uint32_t i = 0; i < settings.count; ++i) {
    if (settings.settings[i].name == name)
      return settings.settings[i].value;
  }
  return nullptr;
}

class Settings {

  Settings() { sql = new SQL_DB(DEFAULT_DB); }
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

    sql->insertInto(Matrix_t(name.c_str(), 3), _setting.toRow());
  }

  inline Settings_t getSettingsFromDb() {
    Matrix_t matrix = sql->selectFromTable(TABLE_NAME);
    Settings_t _settings = Settings_t(matrix.rowCount);

    for(uint32_t i = 0 ; i < _settings.count; ++i)
      _settings.settings[i] = Setting::fromRow(matrix.getRow(i));

    return _settings;
  }

private:
  SQL_DB *sql;
};

#endif

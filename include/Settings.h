#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "SQLiteWrapper/include/SQL_Datatypes.h"
#include "SQLiteWrapper/include/SQL_Wrapper.h"
#include "Setting.h"
#include <cinttypes>
#include <nlohmann/json_fwd.hpp>
#include <string>

static const char *TABLE_NAME = "settings";
static const char *DEFAULT_DB = "settings.db";

struct Settings_t {
  unsigned long count;
  Setting_t *settings;

  Settings_t() = default;
  Settings_t(unsigned long count) : count(count) {
    settings = new Setting_t[count];
  }
  ~Settings_t() { destroy(); }
  // copy
  Settings_t(const Settings_t &other) : count(other.count) { copy_from(other); }
  // copy assignment
  Settings_t &operator=(const Settings_t &other) {
    if (this != &other) {
      copy_from(other);
    }
    return *this;
  }
  // move
  Settings_t(Settings_t &&other) noexcept { move_from(std::move(other)); }
  // move assignment
  Settings_t &operator=(Settings_t &&other) {
    if (this != &other) {
      move_from(std::move(other));
    }
    return *this;
  }

  Matrix_t toMatrix(){
    Matrix_t _m = Matrix_t(3, count);

    return _m;
  }

  static Settings_t fromMatric(Matrix_t m){
    Settings_t _s = Settings_t(m.rowCount);

    return _s;
  }

  nlohmann::json toJson(){
    nlohmann::json j;

    return j;
  }

  static Settings_t fromJson(nlohmann::json j){
    Settings_t _s = Settings_t(j.size());


    return _s;
  }

private:
  void destroy() {}

  void copy_from(const Settings_t &o) {}

  void move_from(Settings_t &&o) noexcept {}
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
    Setting_t _setting(name, value);

    sql->insertInto(Matrix_t(name.c_str(), 3), _setting.toRow());
  }

  inline Settings_t getSettingsFromDb() {
    Matrix_t matrix = sql->selectFromTable(TABLE_NAME);
    Settings_t _settings = Settings_t(matrix.rowCount);

    for (uint32_t i = 0; i < _settings.count; ++i)
      _settings.settings[i] = Setting_t::fromRow(matrix.getRow(i));

    return _settings;
  }

private:
  SQL_DB *sql;
};

#endif

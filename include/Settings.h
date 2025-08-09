#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Setting.h"
#include <concepts>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <set>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

inline const char *CHECK_TABLE_STR = "SELECT name "
                                     "FROM sqlite_master "
                                     "WHERE type='table' AND name='settings';";

inline const char *GET_ALL_STR = "SELECT name, value, dType FROM settings;";

class Settings_t {

  Settings_t(const char *filename) {

    // Check if the file actually exists
    if (!std::filesystem::exists(filename))
      throw std::runtime_error(std::format("Db does not exists: {}", filename));

    // Check if we can open the DB, do so
    if (sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE, nullptr) !=
        SQLITE_OK)
      throw std::runtime_error(std::format(
          "Open failed:\n, {} : {}", sqlite3_errcode(db), sqlite3_errmsg(db)));

    sql_err = nullptr;

    // This bool and call back are used to check if the table exists in the db
    bool exists = false;
    auto callback = [](void *data, int argc, char **argv,
                       char **colNames) -> int {
      bool *exists_ptr = static_cast<bool *>(data);
      *exists_ptr = (argc > 0 && argv[0] != nullptr);
      return 0;
    };

    // Execute the command and give it the callback
    if (sqlite3_exec(db, CHECK_TABLE_STR, callback, &exists, &sql_err) !=
        SQLITE_OK)
      throw std::runtime_error(sql_error());
  }
  ~Settings_t() {
    sqlite3_free(sql_err);
    sqlite3_close(db);
  }

  /** @function addSetting
   * @brief use this function to add a new setting to the list, if the setting
   * exists this will overwrite it
   * @param name - the name of the setting
   * @param value - templated typename value, for the setting
   */
  template <typename T> inline void addUpdateSetting(std::string name, T value) {
    // Check that it's an integral data type
    if constexpr (!std::is_integral_v<T>)
      throw std::runtime_error("Non integral data type");

    // Check what data type it is
    DataType_t dt;
    if constexpr (std::same_as<T, int>)
      dt = Integer;
    else if constexpr (std::same_as<T, float> || std::same_as<T, double>)
      dt = Real;
    else if constexpr (std::same_as<T, std::string>)
      dt = Text;

    // Create a new setting, convert the value to a string
    Setting stng(name, std::string(value), dt);

    // Add the value to the db, overwrite if already exists
    if (sqlite3_exec(db, stng.createOrReplace().c_str(), nullptr, nullptr,
                     &sql_err) != SQLITE_OK)
      throw std::runtime_error(sql_error());
  }

  inline std::vector<Setting> getSettingsFromDb() {

    std::vector<Setting> settings;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, GET_ALL_STR, -1, &stmt, nullptr) != SQLITE_OK)
      throw std::runtime_error(sql_error());

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string name =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      std::string value =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      int dType = sqlite3_column_int(stmt, 2);

      if (dType != Integer && dType != Real && dType != Text)
        throw std::runtime_error("dType from setting is wrong");

      Setting stng(name, value, (DataType_t)dType);
      settings.push_back(stng);
    }

    sqlite3_finalize(stmt);
    return settings;
  }

private:
  sqlite3 *db;
  char *sql_err;

  inline std::string sql_error() {
    std::string str = std::format("SQL Error: {}", sql_err);
    std::cerr << str << std::endl;
    sqlite3_free(sql_err);
    return str;
  }
};

#endif

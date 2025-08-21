#ifndef SETTING_H_
#define SETTING_H_

#include "SQLiteWrapper/include/SQL_Datatypes.h"
#include "SQLiteWrapper/include/SQL_Value.h"
#include <sqlite3.h>

#ifndef ARDUNIO
#include <format>
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

  Row_t toRow() {
    Row_t r(3);
    r.columns[0] = Column_t("name", name.c_str(), true);
    r.columns[1] = Column_t("value", value.toString(), false);
    r.columns[2] = Column_t("dType", value.type(), false);
    return r;
  }

  static Setting fromRow(Row_t r) {
    if (r.colCount != 3)
      return Setting();

    SqlValue v;
    switch (r.columns[2].value.as_int()) {
    case SqlValue::Type::Null:
      v = nullptr;
      break;
    case SqlValue::Type::Integer:
      v = r.columns[1].value.as_int();
      break;
    case SqlValue::Type::Real:
      v = r.columns[1].value.as_real();
      break;
    case SqlValue::Type::Text:
      v = r.columns[1].value.as_text();
      break;
    }
    return Setting(r.columns[0].value.as_text(), v);
  }

private:
  std::string str;
};
#endif

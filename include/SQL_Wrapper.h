#ifndef SQL_WRAPPER_H
#define SQL_WRAPPER_H

#include "SQL_Value.h"
#include <asm-generic/errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef ARDUINO
#include <cstdint>
#include <format>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#endif
#include <sqlite3.h>

#define MAX_COLUMN_NAME_LENGTH (32)

void safeNameCopy(char *dest, const char *src) {
  if (strlen(src) < MAX_COLUMN_NAME_LENGTH)
    strcpy(dest, src);
  else
    memcpy(dest, src, MAX_COLUMN_NAME_LENGTH);
}

struct Column_t {
  char name[MAX_COLUMN_NAME_LENGTH];
  SqlValue value;
  bool primaryKey = false;
  Column_t(const char *name, SqlValue value, bool primaryKey)
      : value(value), primaryKey(primaryKey) {
    safeNameCopy((char *)&this->name, name);
  }
  Column_t() = default;
};

struct Row_t {
  short colCount;
  Column_t *columns;
  Row_t(short colCount) : colCount(colCount) {
    columns = new Column_t[colCount];
  }
  Row_t(Column_t *columns, short colCount) : colCount(colCount) {
    this->columns = new Column_t[colCount];
    memcpy((void *)this->columns, columns, sizeof(Column_t) * colCount);
  }
  Row_t() = default;
  ~Row_t() {
    if (columns != nullptr)
      free(columns);
  }
};

struct ColumnOfData {
  long rowCount;
  char name[MAX_COLUMN_NAME_LENGTH];
  SqlValue *values;

  ColumnOfData(const char *name, long rowCount) : rowCount(rowCount) {
    safeNameCopy((char *)&this->name, name);
    values = new SqlValue[rowCount];
  }

  ColumnOfData(const char *name, SqlValue *values, long rowCount)
      : rowCount(rowCount) {
    safeNameCopy((char *)&this->name, name);
    this->values = new SqlValue[rowCount];
    memcpy((void *)this->values, values, sizeof(SqlValue) * rowCount);
  }

  ~ColumnOfData() {
    if (values != nullptr)
      free(values);
  }
};

struct TableOfData {
  short colCount;
  ColumnOfData *columns;
};

class SQL_Wrapper {

public:
  SQL_Wrapper(const char *filename) : filename(filename) {
    // Check if we can open the DB, do so
    if (sqlite3_open_v2(filename, &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK)
      throw std::runtime_error(std::format(
          "Open failed:\n, {} : {}", sqlite3_errcode(db), sqlite3_errmsg(db)));

    sql_err = nullptr;
  }

  ~SQL_Wrapper() { sqlite3_close_v2(db); }

  inline bool tableExists(const char *tableName) {
    // This bool and call back are used to check if the table exists in the db
    bool exists = false;
    auto callback = [](void *data, int argc, char **argv,
                       char **colNames) -> int {
      bool *exists_ptr = static_cast<bool *>(data);
      *exists_ptr = (argc > 0 && argv[0] != nullptr);
      return 0;
    };

    std::string check = std::format("SELECT name "
                                    "FROM sqlite_master "
                                    "WHERE type='table' AND name='{}';",
                                    tableName);

    // Execute the command and give it the callback
    if (sqlite3_exec(db, check.c_str(), callback, &exists, &sql_err) !=
        SQLITE_OK)
      throw std::runtime_error(sql_error());

    return exists;
  }

  inline void createTable(const char *tableName, Row_t rowOfNames) {
    std::string sql_str =
        std::format("CREATE TABLE IF NOT EXISTS {}(", tableName);

    for (short i = 0; i < rowOfNames.colCount; ++i) {
      sql_str += std::format("{} {} {}", rowOfNames.columns[i].name,
                             rowOfNames.columns[i].value.typeString(),
                             (rowOfNames.columns[i].primaryKey) ? "PRIMARY KEY"
                                                                : "NOT NULL");

      if (i != (rowOfNames.colCount - 1))
        sql_str += ",";
    }
    sql_str += ");";

    execSimpleSQL(sql_str.c_str());
  }

  inline void dropTable(const char *tableName) {
    execSimpleSQL(std::format("DROP TABLE IF EXISTS {};", tableName).c_str());
  }

  inline void insertInto(const char *tableName, RowOfData data) {
    std::string sql_str = std::format("INSERT INTO {}", tableName);
    std::string dName_str = "(";
    std::string data_str = "VALUES (";
    for (size_t i = 0; i < data.size(); ++i) {
      dName_str += data[i].first;

      if (std::holds_alternative<long>(data[i].second))
        data_str += std::format("{}", std::get<long>(data[i].second));
      else if (std::holds_alternative<double>(data[i].second))
        data_str += std::format("{}", std::get<double>(data[i].second));
      else if (std::holds_alternative<std::string>(data[i].second))
        data_str = std::format("'{}'", std::get<std::string>(data[i].second));

      if (i != (data.size() - 1)) {
        dName_str += ", ";
        data_str += ", ";
      }
    }
    dName_str += ")";
    data_str += ")";

    sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);
    execSimpleSQL(sql_str.c_str());
  }

  inline void insertManySameTypeInto(const char *tableName, Table data) {

    execSimpleSQL("BEGIN TRANSACTION;");

    size_t col_count = data.size();
    size_t row_count = data[0].second.size();

    sqlite3_stmt *stmt;

    std::string sql_str = std::format("INSERT INTO {}", tableName);
    std::string dName_str = "(";
    std::string data_str = "VALUES (";
    for (size_t i = 0; i < data.size(); ++i) {
      dName_str += data[i].first;
      data_str += "?";

      if (i != (data.size() - 1)) {
        dName_str += ", ";
        data_str += ", ";
      }
    }
    dName_str += ")";
    data_str += ")";

    sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);

    if (sqlite3_prepare_v2(db, sql_str.c_str(), -1, &stmt, nullptr) !=
        SQLITE_OK)
      throw std::runtime_error(db_error_msg("Prepare"));

    for (size_t row = 0; row < row_count; ++row) {
      for (size_t col = 0; col < col_count; ++col) {
        if (std::holds_alternative<long>(data[col].second[row]))
          sqlite3_bind_int64(stmt, col, std::get<long>(data[col].second[row]));
        else if (std::holds_alternative<double>(data[col].second[row]))
          sqlite3_bind_double(stmt, col,
                              std::get<double>(data[col].second[row]));
        else if (std::holds_alternative<std::string>(data[col].second[row])) {
          std::string str = std::get<std::string>(data[col].second[row]);
          sqlite3_bind_text(stmt, col, str.c_str(), str.length(), nullptr);
        }
      }

      if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error(db_error_msg("Step"));

      sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);

    execSimpleSQL("COMMIT;");
  }

  Table selectSameTypeFromTable(const char *tableName, Columns colNames) {
    Table selection;

    std::string sql_str = "SELECT ";
    for (size_t i = 0; i < colNames.size(); ++i) {
      sql_str += colNames[i].first.first;
      if (i != colNames.size() - 1)
        sql_str += ", ";
    }

    sql_str = std::format("{} FROM {};", sql_str, tableName);
    return queryToTable(sql_str.c_str());
  }

  inline Table selectAllFromTable(const char *tableName) {
    Table selection;

    std::string sql_str = std::format("SELECT * FROM {};", tableName);
    return queryToTable(sql_str.c_str());
  }

private:
  sqlite3 *db;
  std::string filename;
  char *sql_err;

  inline Table queryToTable(const char *query) {
    Table selection;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
      throw std::runtime_error(db_error_msg("Prepare"));

    for (int i = 0; i < sqlite3_column_count(stmt); ++i)
      selection.push_back(
          makeColOfData(sqlite3_column_name(stmt, i), std::vector<SqlValue>()));

    do {
      for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
        std::string dtp = sqlite3_column_decltype(stmt, i);
        std::cout << dtp << std::endl;
        if (dtp == "INTEGER")
          selection[i].second.push_back(sqlite3_column_int(stmt, i));
        else if (dtp == "REAL")
          selection[i].second.push_back(sqlite3_column_double(stmt, i));
        else if (dtp == "TEXT") {
          const unsigned char *txt = sqlite3_column_text(stmt, i);
          selection[i].second.push_back(
              txt ? reinterpret_cast<const char *>(txt) : "");
        } else if (dtp == "NULL") {
        } else
          throw std::runtime_error(std::format("Unexpected dT, {}", dtp));
      }
    } while (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return selection;
  }

  inline void execSimpleSQL(const char *sql_str) {
    if (sqlite3_exec(db, sql_str, nullptr, nullptr, &sql_err) != SQLITE_OK)
      throw std::runtime_error(sql_error());
  }

  inline std::string db_error_msg(const char *error) {
    return std::format("{} Error: {}", error, sqlite3_errmsg(db));
  }

  inline std::string sql_error() {
    std::string str = std::format("SQL Error: {}", sql_err);
    sqlite3_free(sql_err);
    return str;
  }
};
#endif

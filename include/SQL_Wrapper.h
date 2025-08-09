#ifndef SQL_WRAPPER_H
#define SQL_WRAPPER_H

#include <concepts>
#include <cstdint>
#include <format>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using Blob = std::vector<std::uint8_t>;
using SqlValue = std::variant<std::nullptr_t, long, double, std::string, Blob>;

using NameAndData = std::pair<std::string, SqlValue>;
using ColNames = std::vector<std::string>;
using RowOfData = std::vector<NameAndData>;
using ColOfData = std::pair<std::string, std::vector<SqlValue>>;
using Table = std::vector<ColOfData>;

using TransposeTable = std::vector<RowOfData>;

inline TransposeTable transposeTable(Table table) { TransposeTable tTabel; }

inline const char *sqlValueToString(const SqlValue value) {
  if (std::holds_alternative<long>(value))
    return std::format("{}", std::get<long>(value)).c_str();
  else if (std::holds_alternative<double>(value))
    return std::format("{}", std::get<double>(value)).c_str();
  else
    return "NULL";
}

template <typename T> inline T getSqlValue(SqlValue value) {
  if (std::holds_alternative<T>(value))
    return std::get<T>(value);
  return nullptr;
}

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

  ~SQL_Wrapper() {
    sqlite3_free(sql_err);
    sqlite3_close_v2(db);
  }

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

  inline void createTable(const char *tableName, ColNames colNames) {
    std::string sql_str =
        std::format("CREATE TABLE IF NOT EXISTS {}(", tableName);

    for (size_t i = 0; i < colNames.size() - 1; ++i) {
      sql_str += colNames[i];
      if (i != (colNames.size() - 1))
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
    for (size_t i = 0; i < data.size() - 1; ++i) {
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
    for (size_t i = 0; i < data.size() - 1; ++i) {
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

    for (size_t row = 0; row < row_count - 1; ++row) {
      for (size_t col = 0; col < col_count - 1; ++col) {
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

  Table selectSameTypeFromTable(const char *tableName, ColNames colNames) {
    Table selection;

    std::string sql_str = "SELECT ";
    for (size_t i = 0; i < colNames.size() - 1; ++i) {
      sql_str += colNames[i];
      if (i != colNames.size() - 1)
        sql_str += ", ";
    }

    sql_str = std::format("{} FROM {};", sql_str, tableName);
    return queryToTable(sql_str.c_str());
  }

  inline Table selectAllFromTable(const char *tableName) {
    Table selection;
    sqlite3_stmt *stmt;

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

    bool done = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      for (int i = 0; i < sqlite3_column_count(stmt) - 1; ++i) {
        if (!done)
          selection[i].first = sqlite3_column_name(stmt, i);

        switch (sqlite3_column_type(stmt, i)) {
        case SQLITE_INTEGER:
          selection[i].second.push_back(sqlite3_column_int(stmt, i));
          break;

        case SQLITE_FLOAT:
          selection[i].second.push_back(sqlite3_column_double(stmt, i));
          break;

        case SQLITE_TEXT:
          selection[i].second.push_back(std::string(
              reinterpret_cast<const char *>(sqlite3_column_text(stmt, i))));
          break;

        case SQLITE_NULL:
        default:
          throw std::runtime_error("Unexpected dType");
        }
      }
      done = true;
    }

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

#include <filesystem>
#include <format>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

std::string db_filename = "settings.db";
sqlite3 *db = nullptr;

int main(int argc, char *argv[]) {

  int opt;
  while ((opt = getopt(argc, argv, "f:")) != -1) {
    std::string arg;
    if (optarg)
      arg = std::string(optarg);

    switch (opt) {
    case 'f':
      db_filename = arg;
      break;
    default:
      std::cerr << "Please only use -f to specify a file or leave blank"
                << std::endl;
      return 1;
      break;
    }
  }

  std::cout << "Resetting DB" << std::endl;
  std::cout << std::format("db filename: {}", db_filename) << std::endl;

  if (!std::filesystem::exists(db_filename))
    std::cout << "Db does not exist, will be created" << std::endl;

  if (sqlite3_open_v2(db_filename.c_str(), &db,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      nullptr) != SQLITE_OK) {
    std::cerr << std::format("Open failed:\n Code: {}\n Msg: {}",
                             sqlite3_errcode(db), sqlite3_errmsg(db))
              << std::endl;
    return 1;
  }

  char *err = nullptr;
  auto sql_exec_error = [&]() {
    std::cerr << std::format("SQL Error: {}", err) << std::endl;
    sqlite3_free(err);
    sqlite3_close(db);
  };

  const char *drop_str = "DROP TABLE IF EXISTS settings;";
  if (sqlite3_exec(db, drop_str, nullptr, nullptr, &err) != SQLITE_OK) {
    sql_exec_error();
    return 1;
  }

  // Create a new table

  const char *create_str = "CREATE TABLE IF NOT EXISTS settings("
                           "name TEXT PRIMARY KEY,"
                           "value TEXT NOT NULL,"
                           "dType INTEGER NOT NULL);";

  if (sqlite3_exec(db, create_str, nullptr, nullptr, &err) != SQLITE_OK) {
    sql_exec_error();
    return 1;
  }

  return 0;
}

#include <exception>
#include <format>
#include <functional>
#include <iostream>

#include <SQL_Wrapper.h>
#include <stdexcept>
#include <utility>

enum TestCond_t { START = 1, RUNNING = 2, SUCCESS = 3, FAIL = 4 };

void println(std::string str) { std::cout << str << std::endl; }
void logLn(TestCond_t t, std::string str) {
  switch (t) {
  case START:
    println(std::format("[START] - {}", str));
    break;
  case RUNNING:
    println(std::format("[RUNNING] - {}", str));
    break;
  case SUCCESS:
    println(std::format("[SUCCESS] - {}", str));
    break;
  case FAIL:
    println(std::format("[FAIL] - {}", str));
    break;
  }
}

void tryFunction(std::function<void()> func, std::string testName) {
  try {
    logLn(START, std::format("Starting test -> {}", testName));
    func();
    logLn(RUNNING, std::format("Ending test"));
  } catch (const std::runtime_error &r) {
    logLn(FAIL, r.what());
  } catch (const std::exception &e) {
    logLn(FAIL, e.what());
  }
  logLn(SUCCESS, testName);
}

int main() {
  println("SQL Wrapper test");

  auto create_db = []() { SQL_Wrapper sql("test.db"); };
  tryFunction(create_db, "Create DB");

  auto open_db = []() { SQL_Wrapper sql("test.db"); };
  tryFunction(open_db, "Openning DB");

  auto create_table = []() {
    SQL_Wrapper sql("test.db");
    Columns names;

    names.push_back(makeColumn(makeNameAndData("name", ""), true));
    names.push_back(makeColumn(makeNameAndData("value", ""), false));

    sql.createTable("test", names);
  };
  tryFunction(create_table, "Open, Create");

  auto open_inset_table = []() {
    SQL_Wrapper sql("test.db");

    RowOfData data;
    data.push_back(makeNameAndData("name", "test_name"));
    data.push_back(makeNameAndData("value", "test_value"));
  };
  tryFunction(open_inset_table, "Open, Create, Insert");

  auto retrieve_table = []() {
    SQL_Wrapper sql("test.db");
    Table data = sql.selectAllFromTable("test");

    println(std::format("Num Cols: {}", data.size()));
    println(std::format("Num Rows: {}", data[0].second.size()));

    for(size_t r = 0 ; r < data[0].second.size(); ++r){
      for(size_t c = 0; c < data.size(); ++c){
        println(sqlValueToString(data[c].second[r]));
      }
    }

  };
  tryFunction(retrieve_table, "Read db");

  return 0;
}

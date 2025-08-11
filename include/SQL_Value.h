#ifndef SQL_VALUE_H
#define SQL_VALUE_H

#include <cstddef>
#include <cstring>
#ifndef ARDUINO
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#endif
#include <sqlite3.h>

struct SqlValue {
  enum class Type : uint8_t { Null, Integer, Real, Text, Blob };

  SqlValue() : kind(Type::Null) {}
  SqlValue(int64_t v) : kind(Type::Integer) { st.i = v; }
  SqlValue(double v) : kind(Type::Real) { st.r = v; }
  SqlValue(const char *s) : kind(Type::Text), size(strlen(s)) {
    st.s = new char[size];
    strcpy(st.s, s);
  }
  SqlValue(const void *data, size_t n) : kind(Type::Blob), size(n) {
    st.b = new uint8_t[size];
    memcpy((void *)&st.b, data, size);
  }

  // Copy
  SqlValue(const SqlValue &other) : kind(Type::Null) { copy_from(other); }
  SqlValue &operator=(const SqlValue &other) {
    if (this != &other) {
      destroy();
      copy_from(other);
    }
    return *this;
  }

  // Move
  SqlValue(SqlValue &&other) noexcept : kind(Type::Null) {
    move_from(std::move(other));
  }
  SqlValue &operator=(SqlValue &&other) noexcept {
    if (this != &other) {
      destroy();
      move_from(std::move(other));
    }
    return *this;
  }

  ~SqlValue() { destroy(); }

  Type type() const { return kind; }

  const char *typeString() const {
    switch (kind) {
    case Type::Null:
      return "NULL";
    case Type::Integer:
      return "INTEGER";
    case Type::Real:
      return "REAL";
    case Type::Text:
      return "TEXT";
    case Type::Blob:
      return "BLOB";
    default:
      return "NULL";
    }
  }

  char *toString(char *str) const {
    str = new char[32];
    switch (kind) {
    case Type::Null:
      sprintf(str, "NULL");
      break;
    case Type::Integer:
      sprintf(str, "%ld", st.i);
      break;
    case Type::Real:
      sprintf(str, "%f", st.r); // print 6 decimals
      break;
    case Type::Text:
    case Type::Blob:
      free(str);
      str = new char[size];
      strcpy(str, st.s);
      break;
    }
    return str;
  }

  // Accessors (assert on wrong type for simplicity)
  int64_t as_int() const {
    assert(kind == Type::Integer);
    return st.i;
  }
  double as_real() const {
    assert(kind == Type::Real);
    return st.r;
  }
  const char *as_text() const { return (const char *)&st.s; }
  const uint8_t *as_blob() const { return (uint8_t *)st.b; }

  // Helpers to create from sqlite3 column
  static SqlValue from_column(sqlite3_stmt *stmt, int col) {
    int t = sqlite3_column_type(stmt, col);
    switch (t) {
    case SQLITE_NULL:
      return SqlValue{};
    case SQLITE_INTEGER:
      return SqlValue(static_cast<int64_t>(sqlite3_column_int64(stmt, col)));
    case SQLITE_FLOAT:
      return SqlValue(sqlite3_column_double(stmt, col));
    case SQLITE_TEXT: {
      const unsigned char *p = sqlite3_column_text(stmt, col);
      int n = sqlite3_column_bytes(stmt, col);
      return SqlValue(p, n);
    }
    case SQLITE_BLOB: {
      const void *p = sqlite3_column_blob(stmt, col);
      int n = sqlite3_column_bytes(stmt, col);
      return SqlValue(p, n);
    }
    default:
      return SqlValue{}; // defensive
    }
  }

  // Helpers to bind to sqlite3 parameter (1-based index)
  static int bind(sqlite3_stmt *stmt, int idx, const SqlValue &v) {
    switch (v.kind) {
    case Type::Null:
      return sqlite3_bind_null(stmt, idx);
    case Type::Integer:
      return sqlite3_bind_int64(stmt, idx, v.st.i);
    case Type::Real:
      return sqlite3_bind_double(stmt, idx, v.st.r);
    case Type::Text: {
      // Use SQLITE_TRANSIENT so SQLite copies the bytes
      return sqlite3_bind_text(stmt, idx, v.st.s, v.size, SQLITE_TRANSIENT);
    }
    case Type::Blob: {
      const void *p = v.st.b;
      return sqlite3_bind_blob(stmt, idx, p, v.size, SQLITE_TRANSIENT);
    }
    }
    return SQLITE_MISUSE; // unreachable
  }

private:
  Type kind;
  size_t size; // in bytes

  union Storage {
    int64_t i;
    double r;
    char *s;    // non-trivial -> placement new + manual dtor
    uint8_t *b; // non-trivial
    Storage() {}
    ~Storage() {}
  } st;

  void destroy() {
    switch (kind) {
    case Type::Text:
      free(st.s);
      break;
    case Type::Blob:
      free(st.b);
      break;
    }
    kind = Type::Null;
  }

  void copy_from(const SqlValue &o) {
    kind = o.kind;
    switch (o.kind) {
    case Type::Null:
      break;
    case Type::Integer:
      st.i = o.st.i;
      break;
    case Type::Real:
      st.r = o.st.r;
      break;
    case Type::Text:
      st.s = new char[o.size];
      strcpy(st.s, o.st.s);
      break;
    case Type::Blob:
      st.b = new uint8_t[o.size];
      memcpy((void *)&st.b, o.st.b, o.size);
      break;
    }
  }

  void move_from(SqlValue &&o) noexcept {
    kind = o.kind;
    switch (o.kind) {
    case Type::Null:
      break;
    case Type::Integer:
      st.i = o.st.i;
      break;
    case Type::Real:
      st.r = o.st.r;
      break;
    case Type::Text:
      st.s = new char[o.size];
      st.s = std::move(o.st.s);
      break;
    case Type::Blob:
      st.b = new uint8_t[o.size];
      st.b = std::move(o.st.b);
      break;
    }
    o.destroy();
  }
};

#endif

#ifndef MAPPING_H_
#define MAPPING_H_

#include "SQL_Wrapper.h"
#include "SQLiteWrapper/include/SQL_Value.h"

#define MAX_KEY_LENGTH (32)

struct Mapping_t{
  unsigned short count;
  char ** keys;
  SqlValue * values;

  // default 
  Mapping_t() = default;
  // constructor 
  Mapping_t(unsigned short count) : count(count){
    keys = new char*[count];
    values = new SqlValue[count];
  }

  // destructor
  ~Mapping_t(){
    destroy();
  }

  // copy 
  // copy assignment 
  //
  // move 
  // move assignment 

private:
  void destroy(){
    if(keys)
      delete [] keys;
    if(values)
      delete [] values;
  }

  void copy_from(const Mapping_t &o){}

  void move_from(Mapping_t &&o){}
};

#endif // !MAPPING_H_

#ifndef __MYEXCEPTION__H__
#define __MYEXCEPTION__H__

#include "helper.h"

class myException : public exception {
 private:
  string msg;

 public:
  explicit myException(string message) : msg(message) {}

  virtual const char * what() { return msg.c_str(); }
};
#endif
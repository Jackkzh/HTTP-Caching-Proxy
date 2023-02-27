#ifndef __MYEXCEPTION__H__
#define __MYEXCEPTION__H__

#include "helper.h"

class myException : public std::exception {
 private:
  std::string msg;

 public:
  explicit myException(std::string message) : msg(message) {}

  virtual const char * what() { return msg.c_str(); }
};
#endif
/*
  Judy Arrays allow for fast and memory efficient access to in-memory hash tables.
  Instantiation is slow, but continued use shows speed increases over native v8::Array implementation.
  Some code cleanup was done to allow for clean compilation with g++ or clang++.

  © 2011-2013 by Jerry Sievert
*/

#ifndef JUDYNODE_H
#define JUDYNODE_H

// node
#include <node.h>
#include <node_buffer.h>

// nan
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

using namespace v8; // v8 default, NaN - Namespace
using namespace node;

class JudyNode : public Nan::ObjectWrap {
 public:
  static NAN_MODULE_INIT(Init);

 private:
  void *container;
  // explicit JudyNode() {};
  // ~JudyNode() {};

  static NAN_METHOD(New);
  static NAN_METHOD(Set);
  static NAN_METHOD(Get);
  static NAN_METHOD(Delete);
  static NAN_METHOD(Keys);

  // static Nan::Persistent<v8::FunctionTemplate> constructor;
  /// static Nan::Persistent<v8::Function> constructor_func;
};

void *jg_init(int, int);
int jg_set(void *, uchar *, uchar *, unsigned long, int);
uchar *jg_get(void *, uchar *, unsigned long *, int *);
void jg_delete(void *, uchar *);

#endif /* JUDYNODE_H */

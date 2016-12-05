/*
  Judy Arrays allow for fast and memory efficient access to in-memory hash tables.
  Instantiation is slow, but continued use shows speed increases over native v8::Array implementation.
  Some code cleanup was done to allow for clean compilation with g++ or clang++.

  © 2011-2013 by Jerry Sievert
*/

#include <memory.h>
#include <stdio.h>

#include "judy64nb.h"
#include "judynode.h"

struct store {
  unsigned long length;
  uchar *ptr;
  int type;
};

#define FROM_BUFFER 0
#define FROM_STRING 1
#define MAX_JUDY_SIZE 1024

void *jg_init(int size, int depth = 0) {
  if (!size) {
    size = MAX_JUDY_SIZE;
  } else if (size > MAX_JUDY_SIZE) {
    return NULL;
  }

  return judy_open(size, depth);
}

int jg_set(void *judy, uchar *key, uchar *value, unsigned long len, int type) {
  JudySlot *cell;

  cell = judy_cell((Judy *)judy, key, (size_t)strlen((const char *)key));
  if (cell && *cell) {
    struct store *data = (struct store *)*cell;
    if (data->ptr) {
      free((void *)data->ptr);
    }
    free((void *)data);
  }
  if (cell) {
    struct store *data = (struct store *)malloc(sizeof(struct store));
    data->ptr = (uchar *)malloc(len);
    data->length = len;
    data->type = type;

    memcpy(data->ptr, value, len);

    *cell = (JudySlot)data;
    return 1;
  }

  return 0;
}

uchar *jg_get(void *judy, uchar *key, unsigned long *len, int *type) {
  JudySlot *cell;

  cell = judy_slot((Judy *)judy, key, (size_t)strlen((const char *)key));

  if (cell) {
    struct store *data = (struct store *)*cell;

    *len = data->length;
    *type = data->type;
    uchar *value = (uchar *)data->ptr;

    return value;
  }
  return NULL;
}

void jg_delete(void *judy, uchar *key) {
  JudySlot *cell;

  cell = judy_slot((Judy *)judy, key, (size_t)strlen((const char *)key));

  if (cell) {
    struct store *data = (struct store *)*cell;
    if (data) {
      if (data->ptr) {
        free((void *)data->ptr);
      }
      free((void *)data);
    }

    judy_del((Judy *)judy);
  }
}

Nan::Persistent<Function> constructor;

NAN_MODULE_INIT(JudyNode::Init) {
  Local<FunctionTemplate> tpl =
    Nan::New<FunctionTemplate>(JudyNode::New);
  tpl->SetClassName(Nan::New("Judy").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "get", JudyNode::Get);
  Nan::SetPrototypeMethod(tpl, "set", JudyNode::Set);
  Nan::SetPrototypeMethod(tpl, "delete", JudyNode::Delete);
  // Nan::SetPrototypeMethod(tpl, "rm", JudyNode::Delete);
  // Nan::SetPrototypeMethod(tpl, "del", JudyNode::Delete);
  Nan::SetPrototypeMethod(tpl, "keys", JudyNode::Keys);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("JudyNode").ToLocalChecked(),
           Nan::GetFunction(tpl).ToLocalChecked());
}

NODE_MODULE(JudyNode, JudyNode::Init)

// JudyNode::JudyNode() {};

/*
JudyNode::~JudyNode() {
    if (this->container == NULL) {
        return;
    }
    JudySlot *cell = (JudySlot *)judy_strt((Judy *)this->container, NULL, 0);
    while (cell) {
        free((void *)*cell);
        judy_del((Judy *)this->container);
        cell = judy_nxt((Judy *)this->container);
    }
}
*/

NAN_METHOD(JudyNode::New) {
  if (info.IsConstructCall()) {
    JudyNode *self = new JudyNode();
    self->container = jg_init(1024, 0);
    if (self->container == NULL) {
      info.GetReturnValue().SetUndefined();
    }
    self->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else { // Turn it into a construct call.
    Local<Function> cons =  Nan::New<Function>(constructor);
    info.GetReturnValue().Set(cons->NewInstance());
  }
}

NAN_METHOD(JudyNode::Get) {
  JudyNode *self = Nan::ObjectWrap::Unwrap<JudyNode>(info.This());

  if (info.Length() < 1) {
    return info.GetReturnValue().SetUndefined();
  }

  Nan::Utf8String key(info[0]->ToString());

  unsigned long len;
  int type;
  uchar *get = jg_get(self->container, (uchar *)*key, &len, &type);
  if (get == NULL) {
    return info.GetReturnValue().SetUndefined();
  }

  if (type == FROM_BUFFER) {
    Local<Object> slowBuffer = Nan::NewBuffer(len).ToLocalChecked();
    memcpy(node::Buffer::Data(slowBuffer), get, len);
    // slowBuffer->CopyBuffer(get, len);

    Local<Object> globalObj = Nan::GetCurrentContext()->Global();
    Local<Function> bufferConstructor =
      Local<Function>::Cast(
        globalObj->Get(Nan::New("Buffer").ToLocalChecked())
      );
    Local<Value> constructorArgs[3] = {
      slowBuffer,
      Nan::New<Integer>((int32_t) len),
      Nan::New<Integer>(0)
    };
    Local<Object> actualBuffer =
      Nan::NewInstance(bufferConstructor, 3, constructorArgs).ToLocalChecked();

    info.GetReturnValue().Set(actualBuffer);
  }

  info.GetReturnValue().Set(Nan::New<String>((const char *)get, len).ToLocalChecked());
}

NAN_METHOD(JudyNode::Set) {
  JudyNode *self = Nan::ObjectWrap::Unwrap<JudyNode>(info.This());

  if (info.Length() < 2) {
    return info.GetReturnValue().SetUndefined();
  }
  Nan::Utf8String key(info[0]->ToString());

  if (Buffer::HasInstance(info[1])) {
    Handle<Object> buffer = info[1]->ToObject();
    jg_set(self->container, (uchar *)*key, (uchar *)Buffer::Data(buffer),
           Buffer::Length(buffer), FROM_BUFFER);
  } else {
    Nan::Utf8String data(info[1]->ToString());
    jg_set(self->container, (uchar *)*key, (uchar *)*data, data.length(),
           FROM_STRING);
  }

  return info.GetReturnValue().SetUndefined();
}

NAN_METHOD(JudyNode::Delete) {
  JudyNode *self = Nan::ObjectWrap::Unwrap<JudyNode>(info.This());

  if (info.Length() < 1) {
    return info.GetReturnValue().SetUndefined();
  }
  Nan::Utf8String key(info[0]->ToString());

  jg_delete(self->container, (uchar *)*key);
  return info.GetReturnValue().SetUndefined();
}

NAN_METHOD(JudyNode::Keys) {
  JudyNode *self = Nan::ObjectWrap::Unwrap<JudyNode>(info.This());

  uint32_t counter = 0;
  uchar buf[1024];
  JudySlot *cell = (JudySlot *)judy_strt((Judy *)self->container, NULL, 0);
  Local<Array> arr = Nan::New<Array>();

  while (cell) {
    judy_key((Judy *)self->container, buf, 1024);
    Local<String> val = Nan::New<String>((char *)buf).ToLocalChecked();
    Nan::Set(arr, counter, val);
    cell = judy_nxt((Judy *)self->container);
    counter++;
  }

  info.GetReturnValue().Set(arr);
}


/*
    © 2011-2013 by Jerry Sievert
*/

#include "judy64nb.h"
#include "judy.h"

#include <memory.h>
#include <stdio.h>

#define FROM_BUFFER 0
#define FROM_STRING 1
#define MAX_JUDY_SIZE 1024

struct store {
    unsigned long length;
    uchar *ptr;
    int type;
};

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

NAN_METHOD(JudyNode::New) {
    if (info.IsConstructCall()) {
        JudyNode *self = new JudyNode();
        self->container = jg_init(1024, 0);
        if (self->container == NULL) {
            info.GetReturnValue().SetUndefined();
            // return NanEscapeScope(NanUndefined());
        }
        self->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    } else { // Turn it into a construct call.
        v8::Local<v8::Function> cons =
            Nan::GetFunction(Nan::New(constructor)).ToLocalChecked();
        info.GetReturnValue().Set(cons->NewInstance());
    }
}

NAN_METHOD(JudyNode::Get) {
    JudyNode *self = Nan::Unwrap<JudyNode>(Nan::New(constructor), info.Holder());
    HandleScope scope;

    if (args.Length() < 1) {
        return Undefined();
    }

    String::Utf8Value key(args[0]->ToString());

    unsigned long len;
    int type;
    uchar *get = jg_get(self->container, (uchar *)*key, &len, &type);
    if (get == NULL) {
        return Undefined();
    }

    if (type == FROM_BUFFER) {
        node::Buffer *slowBuffer = node::Buffer::New(len);
        memcpy(node::Buffer::Data(slowBuffer), get, len);

        v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
        v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(
                    globalObj->Get(v8::String::New("Buffer")));
        v8::Handle<v8::Value> constructorArgs[3] = {
            slowBuffer->handle_, v8::Integer::New(len), v8::Integer::New(0)
        };
        v8::Local<v8::Object> actualBuffer =
            bufferConstructor->NewInstance(3, constructorArgs);

        return scope.Close(actualBuffer);
    }

    return scope.Close(String::New((const char *)get, len));
}

NAN_METHOD(JudyNode::Set) {
    JudyNode *self = Nan::Unwrap<JudyNode>(Nan::New(constructor), info.Holder());

    if (args.Length() < 2) {
        return Undefined();
    }
    String::Utf8Value key(args[0]->ToString());

    if (Buffer::HasInstance(args[1])) {
        Handle<Object> buffer = args[1]->ToObject();
        jg_set(self->container, (uchar *)*key, (uchar *)Buffer::Data(buffer),
               Buffer::Length(buffer), FROM_BUFFER);
    } else {
        String::Utf8Value data(args[1]->ToString());
        jg_set(self->container, (uchar *)*key, (uchar *)*data, data.length(),
               FROM_STRING);
    }

    return Undefined();
}

NAN_METHOD(JudyNode::Delete) {
    JudyNode *self = Nan::Unwrap<JudyNode>(Nan::New(constructor), info.Holder());

    if (args.Length() < 1) {
        return Undefined();
    }
    String::Utf8Value key(args[0]->ToString());

    jg_delete(self->container, (uchar *)*key);
    return Undefined();
}

NAN_METHOD(JudyNode::Keys) {
    JudyNode *self = Nan::Unwrap<JudyNode>(Nan::New(constructor), info.Holder());
    HandleScope scope;

    Local<Array> arr = Array::New();
    uint32_t counter = 0;

    JudySlot *cell = (JudySlot *)judy_strt((Judy *)self->container, NULL, 0);
    uchar buf[1024];
    while (cell) {
        judy_key((Judy *)self->container, buf, 1024);
        v8::Handle<v8::String> val = v8::String::New((char *)buf);
        arr->Set(counter, val);

        cell = judy_nxt((Judy *)self->container);
        counter++;
    }

    return scope.Close(arr);
}

Nan::Persistent<v8::Function> JudyNode::constructor;

NAN_MODULE_INIT(MyObject::Init) {
    v8::Local<v8::FunctionTemplate> tpl =
        Nan::New<v8::FunctionTemplate>(JudyNode::New);
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

JudyNode::JudyNode() {};
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

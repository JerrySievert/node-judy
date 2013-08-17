/*

© 2011 by Jerry Sievert

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <node.h>
#include <node_buffer.h>
#include <memory.h>
#include <stdio.h>

#include "judy.h"
#include "judynode.h"

using namespace v8;
using namespace node;



#define MAX_JUDY_SIZE  1024

#define FROM_BUFFER 0
#define FROM_STRING 1

struct store {
    unsigned long length;
    uchar        *ptr;
    int           type;
};


void *jg_init(int size) {
    if (!size) {
        size = MAX_JUDY_SIZE;
    } else if (size > MAX_JUDY_SIZE) {
        return NULL;
    }
    
    return judy_open(size);
}

int jg_set(void *judy, uchar *key, uchar *value, unsigned long len, int type) {
    judyslot *cell;

    cell = judy_cell((Judy *) judy, key, (size_t) strlen((const char *) key));
    if (cell && *cell) {
        struct store *data = (struct store *) *cell;
        if (data->ptr) {
            free((void *) data->ptr);
        }
        free((void *) data);
    }
    if (cell) {
        struct store *data = (struct store *) malloc(sizeof(struct store));
        data->ptr    = (uchar *) malloc(len);
        data->length = len;
        data->type   = type;

        memcpy(data->ptr, value, len);
        
        *cell = (judyslot) data;
        return 1;
    }

    return 0;
}

uchar *jg_get(void *judy, uchar *key, unsigned long *len, int *type) {
    judyslot *cell;
    
    cell = judy_slot((Judy *) judy, key, (size_t) strlen((const char *) key));

    if (cell) {
        struct store *data = (struct store *) *cell;

        *len  = data->length;
        *type = data->type;
        uchar *value = (uchar *) data->ptr;

        return value;
    }
    return NULL;
}

void jg_delete(void *judy, uchar *key) {
    judyslot *cell;
    
    cell = judy_slot((Judy *) judy, key, (size_t) strlen((const char *) key));

    if (cell) {
        struct store *data = (struct store *) *cell;
        if (data) {
            if (data->ptr) {
                free((void *) data->ptr);
            }
            free((void *) data);
        }

        judy_del((Judy *) judy);
    }
}

void JudyNode::Initialize (Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    t->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(t, "get",    JudyNode::Get);
    NODE_SET_PROTOTYPE_METHOD(t, "set",    JudyNode::Set);
    NODE_SET_PROTOTYPE_METHOD(t, "delete", JudyNode::Delete);
    NODE_SET_PROTOTYPE_METHOD(t, "keys", JudyNode::Keys);

    target->Set(String::NewSymbol("JudyNode"), t->GetFunction());
}

Handle<Value> JudyNode::New(const Arguments& args) {
    HandleScope scope;

    JudyNode *judy_obj = new JudyNode();
    judy_obj->container = jg_init(1024);
    
    if (judy_obj->container == NULL) {
        return Undefined();
    }

    judy_obj->Wrap(args.This());

    return args.This();
}

Handle<Value> JudyNode::Get(const Arguments& args) {
    JudyNode *judy_obj = ObjectWrap::Unwrap<JudyNode>(args.This());
    HandleScope scope;

    if (args.Length() < 1) {
        return Undefined();
    }
    
    String::Utf8Value key(args[0]->ToString());
    
    unsigned long len;
    int type;
    uchar *get = jg_get(judy_obj->container, (uchar *) *key, &len, &type);
    if (get == NULL) {
        return Undefined();
    }
    
    if (type == FROM_BUFFER) {
        node::Buffer *slowBuffer = node::Buffer::New(len);
        memcpy(node::Buffer::Data(slowBuffer), get, len);
    
        v8::Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
        v8::Local<v8::Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
        v8::Handle<v8::Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(len), v8::Integer::New(0) };
        v8::Local<v8::Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
    
        return scope.Close(actualBuffer);
    }
    
    return scope.Close(String::New((const char *) get, len));
}

Handle<Value> JudyNode::Set(const Arguments& args) {
    JudyNode *judy_obj = ObjectWrap::Unwrap<JudyNode>(args.This());

    if (args.Length() < 2) {
        return Undefined();

    }
    String::Utf8Value key(args[0]->ToString());

    if (Buffer::HasInstance(args[1])) {
      Handle<Object> buffer = args[1]->ToObject();
      jg_set(judy_obj->container, (uchar *) *key, (uchar *) Buffer::Data(buffer), Buffer::Length(buffer), FROM_BUFFER);
    } else {
      String::Utf8Value data(args[1]->ToString());
      jg_set(judy_obj->container, (uchar *) *key, (uchar *) *data, data.length(), FROM_STRING);
    }

    return Undefined();
}

Handle<Value> JudyNode::Delete(const Arguments& args) {
    JudyNode *judy_obj = ObjectWrap::Unwrap<JudyNode>(args.This());

    if (args.Length() < 1) {
        return Undefined();
    }
    String::Utf8Value key(args[0]->ToString());

    jg_delete(judy_obj->container, (uchar *) *key);
    return Undefined();
}

Handle<Value> JudyNode::Keys(const Arguments& args) {
    JudyNode *judy_obj = ObjectWrap::Unwrap<JudyNode>(args.This());
    HandleScope scope;
    
    Local<Array> arr = Array::New();
    uint32_t counter = 0;
    
    judyslot *cell = (judyslot *) judy_strt((Judy *) judy_obj->container, NULL, 0);
    uchar buf[1024];
    while (cell) {
        judy_key((Judy *) judy_obj->container, buf, 1024);
        v8::Handle<v8::String> val = v8::String::New((char *) buf);
        arr->Set(counter, val);
        
        cell = judy_nxt((Judy *) judy_obj->container);
        counter++;
    }
    
    return scope.Close(arr);
}

JudyNode::~JudyNode() {
    if (this->container == NULL) {
        return;
    }
    judyslot *cell = (judyslot *) judy_strt((Judy *) this->container, NULL, 0);
    while (cell) {
        free((void *) *cell);
        judy_del((Judy *) this->container);
        cell = judy_nxt((Judy *) this->container);
    }
}


extern "C" void init(Handle<Object> target) {
    HandleScope scope;

    JudyNode::Initialize(target);
};

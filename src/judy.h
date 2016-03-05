/*
    © 2011-2013 by Jerry Sievert
*/

#include <nan.h>

class JudyNode : public Nan::ObjectWrap {
  public:
    static NAN_MODULE_INIT(Init);

  private:
    void *container;
    explicit JudyNode() {};
    ~JudyNode() {};

    static NAN_METHOD(New);
    static NAN_METHOD(Set);
    static NAN_METHOD(Get);
    static NAN_METHOD(Delete);
    static NAN_METHOD(Keys);
};

void *jg_init(int, int);
int jg_set(void *, uchar *, uchar *, unsigned long, int);
uchar *jg_get(void *, uchar *, unsigned long *, int *);
void jg_delete(void *, uchar *);

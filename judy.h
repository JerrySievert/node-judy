#ifndef JUDY_H
#define JUDY_H

#ifdef linux
  #include <endian.h>
#else
  #ifdef __BIG_ENDIAN__
    #ifndef BYTE_ORDER
      #define BYTE_ORDER 4321
    #endif
  #else
    #ifndef BYTE_ORDER
      #define BYTE_ORDER 1234
    #endif
  #endif
  #ifndef BIG_ENDIAN
    #define BIG_ENDIAN 4321
  #endif
#endif

typedef unsigned char uchar;
typedef unsigned int uint;
#define PRIuint      "u"

#if defined(__LP64__) || \
  defined(__x86_64__) || \
  defined(__amd64__) || \
  defined(_WIN64) || \
  defined(__sparc64__) || \
  defined(__arch64__) || \
  defined(__powerpc64__) || \
  defined (__s390x__) 
  //  defines for 64 bit
  
  typedef unsigned long long judyvalue;
  typedef unsigned long long judyslot;
  #define JUDY_key_mask (0x07)
  #define JUDY_key_size 8
  #define JUDY_slot_size 8
  #define JUDY_span_bytes (3 * JUDY_key_size)

  #define PRIjudyvalue  "llu"

#else
  //  defines for 32 bit
  
  typedef uint judyvalue;
  typedef uint judyslot;
  #define JUDY_key_mask (0x03)
  #define JUDY_key_size 4
  #define JUDY_slot_size 4
  #define JUDY_span_bytes (7 * JUDY_key_size)

  #define PRIjudyvalue  "u"

#endif


#define JUDY_mask (~(judyslot)0x07)

#define JUDY_seg  65536

enum JUDY_types {
  JUDY_radix    = 0,  // inner and outer radix fan-out
  JUDY_1      = 1,  // linear list nodes of designated count
  JUDY_2      = 2,
  JUDY_4      = 3,
  JUDY_8      = 4,
  JUDY_16      = 5,
  JUDY_32      = 6,
  JUDY_span    = 7,  // up to 28 tail bytes of key contiguously stored
};



typedef struct {
  void *seg;      // next used allocator
  uint next;      // next available offset
} JudySeg;

typedef struct {
  judyslot next;    // judy object
  uint off;      // offset within key
  int slot;      // slot within object
} JudyStack;

typedef struct {
  judyslot root[1];  // root of judy array
  void **reuse[8];  // reuse judy blocks
  JudySeg *seg;    // current judy allocator
  uint level;      // current height of stack
  uint max;      // max height of stack
  JudyStack stack[1];  // current cursor
} Judy;

#define JUDY_max  JUDY_32

void *judy_open(uint);
void judy_close(Judy *);
void *judy_alloc(Judy *, int);
void *judy_data(Judy *, uint);
void judy_free(Judy *, void *, int);
uint judy_key(Judy *, uchar *, uint);
judyslot *judy_slot(Judy *, uchar *, uint);
judyslot *judy_promote(Judy *, judyslot *, int, judyvalue, int);
void judy_radix(Judy *, judyslot *, uchar *, int, int, int, uchar);
void judy_splitnode(Judy *, judyslot *, uint, uint);
judyslot *judy_first(Judy *, judyslot, uint);
judyslot *judy_last(Judy *, judyslot, uint);
judyslot *judy_end(Judy *);
judyslot *judy_nxt(Judy *);
judyslot *judy_prv(Judy *);
judyslot *judy_del(Judy *);
judyslot *judy_strt(Judy *, uchar *, uint);
void judy_splitspan(Judy *, judyslot *, uchar *);
judyslot *judy_cell(Judy *, uchar *, uint);




#endif /* JUDY_H */
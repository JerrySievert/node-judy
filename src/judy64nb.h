#ifndef JUDY_H
#define JUDY_H

#ifdef linux
	#define _FILE_OFFSET_BITS 64
	#define _LARGEFILE_SOURCE
	#define __USE_FILE_OFFSET64

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
#define PRIuint			"u"

#if defined(__LP64__) || \
	defined(__x86_64__) || \
	defined(__amd64__) || \
	defined(_WIN64) || \
	defined(__sparc64__) || \
	defined(__arch64__) || \
	defined(__powerpc64__) || \
	defined (__s390x__) 
	//	defines for 64 bit
	
	typedef unsigned long long judyvalue;
	typedef unsigned long long JudySlot;
	#define JUDY_key_mask (0x07)
	#define JUDY_key_size 8
	#define JUDY_slot_size 8
	#define JUDY_span_bytes (3 * JUDY_key_size)
	#define JUDY_span_equiv JUDY_2
	#define JUDY_radix_equiv JUDY_8

	#define PRIjudyvalue	"llu"

#else
	//	defines for 32 bit
	
	typedef uint judyvalue;
	typedef uint JudySlot;
	#define JUDY_key_mask (0x03)
	#define JUDY_key_size 4
	#define JUDY_slot_size 4
	#define JUDY_span_bytes (7 * JUDY_key_size)
	#define JUDY_span_equiv JUDY_4
	#define JUDY_radix_equiv JUDY_8

	#define PRIjudyvalue	"u"

#endif

#define JUDY_mask (~(JudySlot)0x07)

//	define the alignment factor for judy nodes and allocations
//	to enable this feature, set to 64

#define JUDY_cache_line 8	// minimum size is 8 bytes

#define JUDY_seg	65536

enum JUDY_types {
	JUDY_radix		= 0,	// inner and outer radix fan-out
	JUDY_1			= 1,	// linear list nodes of designated count
	JUDY_2			= 2,
	JUDY_4			= 3,
	JUDY_8			= 4,
	JUDY_16			= 5,
	JUDY_32			= 6,
#ifdef ASKITIS
	JUDY_64			= 7
#else
	JUDY_span		= 7 	// up to 28 tail bytes of key contiguously stored
#endif
};

typedef struct {
	void *seg;			// next used allocator
	uint next;			// next available offset
} JudySeg;

typedef struct {
	JudySlot next;		// judy object
	uint off;			// offset within key
	int slot;			// slot within object
} JudyStack;

typedef struct {
	JudySlot root[1];	// root of judy array
	void **reuse[8];	// reuse judy blocks
	JudySeg *seg;		// current judy allocator
	uint level;			// current height of stack
	uint max;			// max height of stack
	uint depth;			// number of Integers in a key, or zero for string keys
	JudyStack stack[1];	// current cursor
} Judy;

#ifdef ASKITIS

#if JUDY_key_size < 8
#define JUDY_max	JUDY_16
#else
#define JUDY_max	JUDY_64
#endif
#else
#define JUDY_max	JUDY_32
#endif

void *judy_open (uint, uint);
void judy_close (Judy *);
void *judy_alloc (Judy *, uint);
void *judy_data (Judy *, uint);
void *judy_clone (Judy *);
void judy_free (Judy *, void *, int);
uint judy_key (Judy *, uchar *, uint);
JudySlot *judy_slot (Judy *, uchar *, uint);
JudySlot *judy_promote (Judy *, JudySlot *, int, judyvalue, int);
void judy_radix (Judy *, JudySlot *, uchar *, int, int, int, uchar, uint);
void judy_splitnode (Judy *, JudySlot *, uint, uint, uint);
JudySlot *judy_first (Judy *, JudySlot, uint, uint);
JudySlot *judy_last (Judy *, JudySlot, uint, uint);
JudySlot *judy_end (Judy *);
JudySlot *judy_nxt (Judy *);
JudySlot *judy_prv (Judy *);
JudySlot *judy_del (Judy *);
JudySlot *judy_strt (Judy *, uchar *, uint);
void judy_splitspan (Judy *, JudySlot *, uchar *);
JudySlot *judy_cell (Judy *, uchar *, uint);


#endif /* JUDY_H */
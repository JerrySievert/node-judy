//	Judy arrays	22 DEC 2010

//	Author Karl Malbrain, malbrain@yahoo.com
//	with assistance from Jan Weiss.

//	Simplified judy arrays for strings
//	Adapted from the ideas of Douglas Baskins of HP.

//	Map a set of strings to corresponding memory cells (uints).
//	Each cell must be set to a non-zero value by the caller.

//	STANDALONE is defined to compile into a string sorter.

//#define STANDALONE

//	functions:
//	judy_open:	open a new judy array returning a judy object.
//	judy_close:	close an open judy array, freeing all memory.
//	judy_data:	allocate data memory within judy array for external use.
//	judy_cell:	insert a string into the judy array, return cell pointer.
//	judy_strt:	retrieve the cell pointer greater than or equal to given key
//	judy_slot:	retrieve the cell pointer, or return NULL for a given key.
//	judy_key:	retrieve the string value for the most recent judy query.
//	judy_end:	retrieve the cell pointer for the last string in the array.
//	judy_nxt:	retrieve the cell pointer for the next string in the array.
//	judy_prv:	retrieve the cell pointer for the prev string in the array.
//	judy_del:	delete the key and cell for the current stack entry.


#include <stdlib.h>
#include <memory.h>

#include "judy.h"
//#define STANDALONE
#ifdef STANDALONE
#include <stdio.h>
#include <assert.h>

uint MaxMem = 0;

// void judy_abort (char *msg) __attribute__ ((noreturn)); // Tell static analyser that this function will not return
void judy_abort (char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
#endif

#if !defined(_WIN32)
void vfree (void *what, uint size)
{
	free (what);
}
#elif defined(_WIN32)
#include <windows.h>

void *valloc (uint size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void vfree (void *what, uint size)
{
	VirtualFree(what, 0, MEM_RELEASE);
}
#endif


int JudySize[] = {
  (JUDY_slot_size * 16),            // JUDY_radix node size
  (JUDY_slot_size + JUDY_key_size),      // JUDY_1 node size
  (2 * JUDY_slot_size + 2 * JUDY_key_size),
  (4 * JUDY_slot_size + 4 * JUDY_key_size),
  (8 * JUDY_slot_size + 8 * JUDY_key_size),
  (16 * JUDY_slot_size + 16 * JUDY_key_size),
  (32 * JUDY_slot_size + 32 * JUDY_key_size),
  (JUDY_span_bytes + JUDY_slot_size)
};

judyvalue JudyMask[9] = {
0, 0xff, 0xffff, 0xffffff, 0xffffffff,
#if JUDY_key_size > 4
0xffffffffffLL, 0xffffffffffffLL, 0xffffffffffffffLL, 0xffffffffffffffffLL
#endif
};
//	open judy object

void *judy_open (uint max)
{
JudySeg *seg;
Judy *judy;
uint amt;

	if( (seg = (JudySeg *) valloc(JUDY_seg)) ) {
		seg->next = JUDY_seg;
	} else {
#ifdef STANDALONE
		judy_abort ("No virtual memory");
#else
		return NULL;
#endif
	}


	amt = sizeof(Judy) + max * sizeof(JudyStack);
#ifdef STANDALONE
	MaxMem += JUDY_seg;
#endif

	if( amt & 0x07 )
		amt |= 0x07, amt++;

	seg->next -= amt;
	judy = (Judy *)((uchar *)seg + seg->next);
	memset(judy, 0, amt);
 	judy->seg = seg;
	judy->max = max;
	return judy;
}

void judy_close (Judy *judy)
{
JudySeg *seg, *nxt = judy->seg;

	while( (seg = nxt) )
        nxt = (JudySeg *) seg->seg, vfree ((void *)seg, JUDY_seg);
}

//	allocate judy node

void *judy_alloc (Judy *judy, int type)
{
void **block;
JudySeg *seg;
uint amt;

	amt = JudySize[type];

	if( amt & 0x07 )
		amt |= 0x07, amt += 1;

	if( (block = judy->reuse[type]) ) {
		judy->reuse[type] = (void **)*block;
		memset (block, 0, amt);
		return (void *)block;
	}

	if( !judy->seg || judy->seg->next < amt + sizeof(*seg) ) {
		if( (seg = (JudySeg *) valloc (JUDY_seg)) ) {
			seg->next = JUDY_seg, seg->seg = judy->seg, judy->seg = seg;
		} else {
#ifdef STANDALONE
			judy_abort("Out of virtual memory");
#else
			return NULL;
#endif
		}

#ifdef STANDALONE
		MaxMem += JUDY_seg;
#endif
	}

	judy->seg->next -= amt;

	block = (void **)((uchar *)judy->seg + judy->seg->next);
	memset (block, 0, amt);
	return (void *)block;
}

void *judy_data (Judy *judy, uint amt)
{
JudySeg *seg;
void *block;

	if( amt & 0x07 )
		amt |= 0x07, amt += 1;

	if( !judy->seg || judy->seg->next < amt + sizeof(*seg) ) {
		if( (seg = (JudySeg *) valloc (JUDY_seg)) ) {
			seg->next = JUDY_seg, seg->seg = judy->seg, judy->seg = seg;
		} else {
#ifdef STANDALONE
			judy_abort("Out of virtual memory");
#else
			return NULL;
#endif
		}
	
#ifdef STANDALONE
		MaxMem += JUDY_seg;
#endif
	}

	judy->seg->next -= amt;

	block = (void *)((uchar *)judy->seg + judy->seg->next);
	memset (block, 0, amt);
	return block;
}
void judy_free (Judy *judy, void *block, int type)
{
	*((void **)(block)) = judy->reuse[type];
	judy->reuse[type] = (void **)block;
	return;
}
		
//	assemble key from current path

uint judy_key (Judy *judy, uchar *buff, uint max)
{
int slot, cnt, /*size, */off, type;
uint len = 0, idx = 0;
uchar *base;
int keysize;

	max--;		// leave room for zero terminator

	while( len < max && ++idx <= judy->level ) {
		slot = judy->stack[idx].slot;
		type = judy->stack[idx].next & 0x07;
		//size = JudySize[type];
		switch( type ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			keysize = JUDY_key_size - (judy->stack[idx].off & JUDY_key_mask);
			base = (uchar *)(judy->stack[idx].next & JUDY_mask);
			//cnt = size / (sizeof(judyslot) + keysize);
			off = keysize;
#if BYTE_ORDER != BIG_ENDIAN
			while( off-- && len < max )
				buff[len++] = base[slot * keysize + off];
#else
			for( off = 0; off < keysize && len < max; off++ )
				buff[len++] = base[slot * keysize + off];
#endif
			continue;
		case JUDY_radix:
			if( !slot )
				break;
			buff[len++] = slot;
			continue;
		case JUDY_span:
			base = (uchar *)(judy->stack[idx].next & JUDY_mask);
			cnt = JUDY_span_bytes;

			for( slot = 0; slot < cnt && base[slot]; slot++ )
			  if( len < max )
				buff[len++] = base[slot];
			continue;
		}
	}
	buff[len] = 0;
	return len;
}

//	find slot & setup cursor

judyslot *judy_slot (Judy *judy, uchar *buff, uint max)
{
int slot, size, keysize, tst, cnt;
judyslot next = *judy->root;
judyvalue value, test = 0;
judyslot *table;
judyslot *node;
uint off = 0;
uchar *base;

	judy->level = 0;

	while( next ) {
		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].off = off;
		judy->stack[judy->level].next = next;
		size = JudySize[next & 0x07];

		switch( next & 0x07 ) {

		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			base = (uchar *)(next & JUDY_mask);
			node = (judyslot *)((next & JUDY_mask) + size);
			keysize = JUDY_key_size - (off & JUDY_key_mask);
			cnt = size / (sizeof(judyslot) + keysize);
			slot = cnt;
			value = 0;

			do {
				value <<= 8;
				if( off < max )
					value |= buff[off];
			} while( ++off & JUDY_key_mask );

			//  find slot > key

			while( slot-- ) {
				test = *(judyvalue *)(base + slot * keysize);
#if BYTE_ORDER == BIG_ENDIAN
				test >>= 8 * (JUDY_key_size - keysize); 
#else
				test &= JudyMask[keysize];
#endif
				if( test <= value )
					break;
			}

			judy->stack[judy->level].slot = slot;

			if( test == value ) {

				// is this a leaf?

				if( !(value & 0xFF) )
					return &node[-slot-1];

				next = node[-slot-1];
				continue;
			}

			return NULL;

		case JUDY_radix:
			table = (judyslot  *)(next & JUDY_mask); // outer radix

			if( off < max )
				slot = buff[off];
			else
				slot = 0;

			//	put radix slot on judy stack

			judy->stack[judy->level].slot = slot;

			if( (next = table[slot >> 4]) )
				table = (judyslot  *)(next & JUDY_mask); // inner radix
			else
				return NULL;

			if( !slot )	// leaf?
				return &table[slot & 0x0F];

			next = table[slot & 0x0F];
			off += 1;
			break;

		case JUDY_span:
			node = (judyslot *)((next & JUDY_mask) + JudySize[JUDY_span]);
			base = (uchar *)(next & JUDY_mask);
			cnt = tst = JUDY_span_bytes;
			if( tst > (int)(max - off) )
				tst = max - off;
			value = strncmp((const char *)base, (const char *)(buff + off), tst);
			if( !value && tst < cnt && !base[tst] ) // leaf?
				return &node[-1];

			if( !value && tst == cnt ) {
				next = node[-1];
				off += cnt;
				continue;
			}
			return NULL;
		}
	}

	return NULL;
}

//	promote full nodes to next larger size

judyslot *judy_promote (Judy *judy, judyslot *next, int idx, judyvalue value, int keysize)
{
uchar *base = (uchar *)(*next & JUDY_mask);
int oldcnt, newcnt, slot;
#if BYTE_ORDER == BIG_ENDIAN
	int i;
#endif
judyslot *newnode, *node;
judyslot *result;
uchar *newbase;
uint type;

	type = (*next & 0x07) + 1;
	node = (judyslot *)((*next & JUDY_mask) + JudySize[type-1]);
	oldcnt = JudySize[type-1] / (sizeof(judyslot) + keysize);
	newcnt = JudySize[type] / (sizeof(judyslot) + keysize);

	// promote node to next larger size

	newbase = (uchar *) judy_alloc (judy, type);
	newnode = (judyslot *)(newbase + JudySize[type]);
	*next = (judyslot)newbase | type;

	//	open up slot at idx

	memcpy(newbase + (newcnt - oldcnt - 1) * keysize, base, idx * keysize);	// copy keys

	for( slot = 0; slot < idx; slot++ )
		newnode[-(slot + newcnt - oldcnt)] = node[-(slot + 1)];	// copy ptr

	//	fill in new node

#if BYTE_ORDER != BIG_ENDIAN
	memcpy(newbase + (idx + newcnt - oldcnt - 1) * keysize, &value, keysize);	// copy key
#else
	i = keysize;

	while( i-- )
	  newbase[(idx + newcnt - oldcnt - 1) * keysize + i] = value, value >>= 8;
#endif
	result = &newnode[-(idx + newcnt - oldcnt)];

	//	copy rest of old node

	memcpy(newbase + (idx + newcnt - oldcnt) * keysize, base + (idx * keysize), (oldcnt - slot) * keysize);	// copy keys

	for( ; slot < oldcnt; slot++ )
		newnode[-(slot + newcnt - oldcnt + 1)] = node[-(slot + 1)];	// copy ptr

	judy->stack[judy->level].next = *next;
	judy->stack[judy->level].slot = idx + newcnt - oldcnt - 1;
	judy_free (judy, (void **)base, type - 1);
	return result;
}

//	construct new node for JUDY_radix entry
//	make node with slot - start entries
//	moving key over one offset

void judy_radix (Judy *judy, judyslot *radix, uchar *old, int start, int slot, int keysize, uchar key)
{
int size, idx, cnt = slot - start, newcnt;
judyslot *node, *oldnode;
uint type = JUDY_1 - 1;
judyslot *table;
uchar *base;

	//	if necessary, setup inner radix node

	if( !(table = (judyslot *)(radix[key >> 4] & JUDY_mask)) ) {
		table = (judyslot *) judy_alloc (judy, JUDY_radix);
		radix[key >> 4] = (judyslot)table | JUDY_radix;
	}

	oldnode = (judyslot *)(old + JudySize[JUDY_max]);

	// is this slot a leaf?

	if( !key || !keysize ) {
		table[key & 0x0F] = oldnode[-start-1];
		return;
	}

	//	calculate new node big enough to contain slots

	do {
		type++;
		size = JudySize[type];
		newcnt = size / (sizeof(judyslot) + keysize);
	} while( cnt > newcnt && type < JUDY_max );

	//	store new node pointer in inner table

	base = (uchar *) judy_alloc (judy, type);
	node = (judyslot *)(base + size);
	table[key & 0x0F] = (judyslot)base | type;

	//	allocate node and copy old contents
	//	shorten keys by 1 byte during copy

	for( idx = 0; idx < cnt; idx++ ) {
#if BYTE_ORDER != BIG_ENDIAN
		memcpy (base + (newcnt - idx - 1) * keysize, old + (start + cnt - idx - 1) * (keysize + 1), keysize);
#else
		memcpy (base + (newcnt - idx - 1) * keysize, old + (start + cnt - idx - 1) * (keysize + 1) + 1, keysize);
#endif
		node[-(newcnt - idx)] = oldnode[-(start + cnt - idx)];
	}
}
			
//	decompose full node to radix nodes

void judy_splitnode (Judy *judy, judyslot *next, uint size, uint keysize)
{
int cnt, slot, start = 0;
uint key = 0x0100, nxt;
judyslot *newradix;
uchar *base;

	base = (uchar  *)(*next & JUDY_mask);
	cnt = size / (sizeof(judyslot) + keysize);

	//	allocate outer judy_radix node

	newradix = (judyslot *) judy_alloc (judy, JUDY_radix);
	*next = (judyslot)newradix | JUDY_radix;

	for( slot = 0; slot < cnt; slot++ ) {
#if BYTE_ORDER != BIG_ENDIAN
		nxt = base[slot * keysize + keysize - 1];
#else
		nxt = base[slot * keysize];
#endif

		if( key > 0xFF )
			key = nxt;
		if( nxt == key )
			continue;

		//	decompose portion of old node into radix nodes

		judy_radix (judy, newradix, base, start, slot, keysize - 1, key);
		start = slot;
		key = nxt;
	}

	judy_radix (judy, newradix, base, start, slot, keysize - 1, key);
	judy_free (judy, (void **)base, JUDY_max);
}

//	return first leaf

judyslot *judy_first (Judy *judy, judyslot next, uint off)
{
judyslot *table, *inner;
uint keysize, size;
judyslot *node;
int slot, cnt;
uchar *base;

	while( next ) {
		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].off = off;
		judy->stack[judy->level].next = next;
		size = JudySize[next & 0x07];

		switch( next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			keysize = JUDY_key_size - (off & JUDY_key_mask);
			node = (judyslot *)((next & JUDY_mask) + size);
			base = (uchar *)(next & JUDY_mask);
			cnt = size / (sizeof(judyslot) + keysize);

			for( slot = 0; slot < cnt; slot++ )
				if( node[-slot-1] )
					break;

			judy->stack[judy->level].slot = slot;
#if BYTE_ORDER != BIG_ENDIAN
			if( !base[slot * keysize] )
				return &node[-slot-1];
#else
			if( !base[slot * keysize + keysize - 1] )
				return &node[-slot-1];
#endif
			next = node[-slot - 1];
			off = (off | JUDY_key_mask) + 1;
			continue;
		case JUDY_radix:
			table = (judyslot *)(next & JUDY_mask);
			for( slot = 0; slot < 256; slot++ )
			  if( (inner = (judyslot *)(table[slot >> 4] & JUDY_mask)) ) {
				if( (next = inner[slot & 0x0F]) ) {
				  judy->stack[judy->level].slot = slot;
				  if( !slot )
					return &inner[slot & 0x0F];
				  else
					break;
				}
			  } else
				slot |= 0x0F;
			off++;
			continue;
		case JUDY_span:
			node = (judyslot *)((next & JUDY_mask) + JudySize[JUDY_span]);
			base = (uchar *)(next & JUDY_mask);
			cnt = JUDY_span_bytes;
			if( !base[cnt - 1] )	// leaf node?
				return &node[-1];
			next = node[-1];
			off += cnt;
			continue;
		}
	}
	return NULL;
}

//	return last leaf cell pointer

judyslot *judy_last (Judy *judy, judyslot next, uint off)
{
judyslot *table, *inner;
uint keysize, size;
judyslot *node;
int slot, cnt;
uchar *base;

	while( next ) {
		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].off = off;
		judy->stack[judy->level].next = next;
		size = JudySize[next & 0x07];
		switch( next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			keysize = JUDY_key_size - (off & JUDY_key_mask);
			slot = size / (sizeof(judyslot) + keysize);
			base = (uchar *)(next & JUDY_mask);
			node = (judyslot *)((next & JUDY_mask) + size);
			judy->stack[judy->level].slot = --slot;

#if BYTE_ORDER != BIG_ENDIAN
			if( !base[slot * keysize] )
#else
			if( !base[slot * keysize + keysize - 1] )
#endif
				return &node[-slot-1];

			next = node[-slot-1];
			off += keysize;
			continue;
		case JUDY_radix:
			table = (judyslot *)(next & JUDY_mask);
			for( slot = 256; slot--; ) {
			  judy->stack[judy->level].slot = slot;
			  if( (inner = (judyslot *)(table[slot >> 4] & JUDY_mask)) ) {
				if( (next = inner[slot & 0x0F]) )
				  if( !slot )
					return &inner[0];
				  else
					break;
			  } else
				slot &= 0xF0;
			}
			off++;
			continue;
		case JUDY_span:
			node = (judyslot *)((next & JUDY_mask) + JudySize[JUDY_span]);
			base = (uchar *)(next & JUDY_mask);
			cnt = JUDY_span_bytes;
			if( !base[cnt - 1] )	// leaf node?
				return &node[-1];
			next = node[-1];
			off += cnt;
			continue;
		}
	}
	return NULL;
}

//	judy_end: return last entry

judyslot *judy_end (Judy *judy)
{
	judy->level = 0;
	return judy_last (judy, *judy->root, 0);
}

//	judy_nxt: return next entry

judyslot *judy_nxt (Judy *judy)
{
judyslot *table, *inner;
int slot, size, cnt;
judyslot *node;
judyslot next;
uint keysize;
uchar *base;
uint off;

	if( !judy->level )
		return judy_first (judy, *judy->root, 0);

	while( judy->level ) {
		next = judy->stack[judy->level].next;
		slot = judy->stack[judy->level].slot;
		off = judy->stack[judy->level].off;
		keysize = JUDY_key_size - (off & JUDY_key_mask);
		size = JudySize[next & 0x07];

		switch( next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			cnt = size / (sizeof(judyslot) + keysize);
			node = (judyslot *)((next & JUDY_mask) + size);
			base = (uchar *)(next & JUDY_mask);
			if( ++slot < cnt )
#if BYTE_ORDER != BIG_ENDIAN
				if( !base[slot * keysize] )
#else
				if( !base[slot * keysize + keysize - 1] )
#endif
				{
					judy->stack[judy->level].slot = slot;
					return &node[-slot - 1];
				} else {
					judy->stack[judy->level].slot = slot;
					return judy_first (judy, node[-slot-1], (off | JUDY_key_mask) + 1);
				}
			judy->level--;
			continue;

		case JUDY_radix:
			table = (judyslot *)(next & JUDY_mask);

			while( ++slot < 256 )
			  if( (inner = (judyslot *)(table[slot >> 4] & JUDY_mask)) ) {
				if( inner[slot & 0x0F] ) {
				  judy->stack[judy->level].slot = slot;
				  return judy_first(judy, inner[slot & 0x0F], off + 1);
				}
			  } else
				slot |= 0x0F;

			judy->level--;
			continue;
		case JUDY_span:
			judy->level--;
			continue;
		}
	}
	return NULL;
}

//	judy_prv: return ptr to previous entry

judyslot *judy_prv (Judy *judy)
{
int slot, size, keysize;
judyslot *table, *inner;
judyslot *node;
judyslot next;
uchar *base;
uint off;

	if( !judy->level )
		return judy_last (judy, *judy->root, 0);
	
	while( judy->level ) {
		next = judy->stack[judy->level].next;
		slot = judy->stack[judy->level].slot;
		off = judy->stack[judy->level].off;
		size = JudySize[next & 0x07];

		switch( next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			node = (judyslot *)((next & JUDY_mask) + size);
			if( !slot || !node[-slot] ) {
				judy->level--;
				continue;
			}

			base = (uchar *)(next & JUDY_mask);
			judy->stack[judy->level].slot--;
			keysize = JUDY_key_size - (off & JUDY_key_mask);

#if BYTE_ORDER != BIG_ENDIAN
			if( base[(slot - 1) * keysize] )
#else
			if( base[(slot - 1) * keysize + keysize - 1] )
#endif
				return judy_last (judy, node[-slot], (off | JUDY_key_mask) + 1);

			return &node[-slot];

		case JUDY_radix:
			table = (judyslot *)(next & JUDY_mask);

			while( slot-- ) {
			  judy->stack[judy->level].slot--;
			  if( (inner = (judyslot *)(table[slot >> 4] & JUDY_mask)) )
				if( inner[slot & 0x0F] )
				  if( slot )
				    return judy_last(judy, inner[slot & 0x0F], off + 1);
				  else
					return &inner[0];
			}

			judy->level--;
			continue;

		case JUDY_span:
			judy->level--;
			continue;
		}
	}
	return NULL;
}

//	judy_del: delete string from judy array
//		returning previous entry.

judyslot *judy_del (Judy *judy)
{
int slot, off, size, type, high;
judyslot *table, *inner;
judyslot next, *node;
int keysize, cnt;
uchar *base;

	while( judy->level ) {
		next = judy->stack[judy->level].next;
		slot = judy->stack[judy->level].slot;
		off = judy->stack[judy->level].off;
		size = JudySize[next & 0x07];

		switch( type = next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			keysize = JUDY_key_size - (off & JUDY_key_mask);
			cnt = size / (sizeof(judyslot) + keysize);
			node = (judyslot *)((next & JUDY_mask) + size);
			base = (uchar *)(next & JUDY_mask);

			//	move deleted slot to first slot

			while( slot ) {
				node[-slot-1] = node[-slot];
				memcpy (base + slot * keysize, base + (slot - 1) * keysize, keysize);
				slot--;
			}

			//	zero out first slot

			node[-1] = 0;
			memset (base, 0, keysize);

			if( node[-cnt] ) {	// does node have any slots left?
				judy->stack[judy->level].slot++;
				return judy_prv (judy);
			}

			judy_free (judy, base, type);
			judy->level--;
			continue;

		case JUDY_radix:
			table = (judyslot  *)(next & JUDY_mask);
			inner = (judyslot *)(table[slot >> 4] & JUDY_mask);
			inner[slot & 0x0F] = 0;
			high = slot & 0xF0;

			for( cnt = 16; cnt--; )
				if( inner[cnt] )
					return judy_prv (judy);

			judy_free (judy, inner, JUDY_radix);
			table[slot >> 4] = 0;

			for( cnt = 16; cnt--; )
				if( table[cnt] )
					return judy_prv (judy);

			judy_free (judy, table, JUDY_radix);
			judy->level--;
			continue;

		case JUDY_span:
			base = (uchar *)(next & JUDY_mask);
			judy_free (judy, base, type);
			judy->level--;
			continue;
		}
	}

	//	tree is now empty

	*judy->root = 0;
	return NULL;
}

//	return cell for first key greater than or equal to given key

judyslot *judy_strt (Judy *judy, uchar *buff, uint max)
{
judyslot *cell;

	judy->level = 0;

	if( !max )
		return judy_first (judy, *judy->root, 0);

	if( (cell = judy_slot (judy, buff, max)) )
		return cell;

	return judy_nxt (judy);
}

//	split open span node

void judy_splitspan (Judy *judy, judyslot *next, uchar *base)
{
judyslot *node = (judyslot *)(base + JudySize[JUDY_span]);
uint cnt = JUDY_span_bytes;
uchar *newbase;
uint off = 0;
#if BYTE_ORDER != BIG_ENDIAN
int i;
#endif

	do {
		newbase = (uchar *) judy_alloc (judy, JUDY_1);
		*next = (judyslot)newbase | JUDY_1;

#if BYTE_ORDER != BIG_ENDIAN
		i = JUDY_key_size;
		while( i-- )
			*newbase++ = base[off + i];
#else
		memcpy (newbase, base + off, JUDY_key_size);
		newbase += JUDY_key_size;
#endif
		next = (judyslot *)newbase;

		off += JUDY_key_size;
		cnt -= JUDY_key_size;
	} while( cnt && base[off - 1] );

	*next = node[-1];
	judy_free (judy, base, JUDY_span);
}

//	judy_cell: add string to judy array

judyslot *judy_cell (Judy *judy, uchar *buff, uint max)
{
int size, idx, slot, cnt, tst;
judyslot *next = judy->root;
judyvalue test = NULL, value;
uint off = 0, start;
judyslot *table;
judyslot *node;
uint keysize;
uchar *base;

	judy->level = 0;

	while( *next ) {
		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].off = off;
		judy->stack[judy->level].next = *next;
		size = JudySize[*next & 0x07];

		switch( *next & 0x07 ) {
		case JUDY_1:
		case JUDY_2:
		case JUDY_4:
		case JUDY_8:
		case JUDY_16:
		case JUDY_32:
			keysize = JUDY_key_size - (off & JUDY_key_mask);
			cnt = size / (sizeof(judyslot) + keysize);
			base = (uchar *)(*next & JUDY_mask);
			node = (judyslot *)((*next & JUDY_mask) + size);
			start = off;
			slot = cnt;
			value = 0;

			do {
				value <<= 8;
				if( off < max )
					value |= buff[off];
			} while( ++off & JUDY_key_mask );

			//  find slot > key

			while( slot-- ) {
				test = *(judyvalue *)(base + slot * keysize);
#if BYTE_ORDER == BIG_ENDIAN
				test >>= 8 * (JUDY_key_size - keysize); 
#else
				test &= JudyMask[keysize];
#endif
				if( test <= value )
					break;
			}

			judy->stack[judy->level].slot = slot;

			if( test == value ) {		// new key is equal to slot key
				next = &node[-slot-1];

				// is this a leaf?

				if( !(value & 0xFF) )
					return next;

				continue;
			}

			//	if this node is not full
			//	open up cell after slot

			if( !node[-1] ) { // if the entry before node is empty/zero
		 	  memmove(base, base + keysize, slot * keysize);	// move keys less than new key down one slot
#if BYTE_ORDER != BIG_ENDIAN
			  memcpy(base + slot * keysize, &value, keysize);	// copy new key into slot
#else
			  test = value;
			  idx = keysize;

			  while( idx-- )
				  base[slot * keysize + idx] = test, test >>= 8;
#endif
			  for( idx = 0; idx < slot; idx++ )
				node[-idx-1] = node[-idx-2];// copy tree ptrs/cells down one slot

			  node[-slot-1] = 0;			// set new tree ptr/cell
			  next = &node[-slot-1];

			  if( !(value & 0xFF) )
			  	return next;

			  continue;
			}

			if( size < JudySize[JUDY_max] ) {
			  next = judy_promote (judy, next, slot+1, value, keysize);

			  if( !(value & 0xFF) )
				return next;

			  continue;
			}

			//	split full maximal node into JUDY_radix nodes
			//  loop to reprocess new insert

			judy_splitnode (judy, next, size, keysize);
			judy->level--;
			off = start;
			continue;
		
		case JUDY_radix:
			table = (judyslot *)(*next & JUDY_mask); // outer radix

			if( off < max )
				slot = buff[off++];
			else
				slot = 0;

			// allocate inner radix if empty

			if( !table[slot >> 4] )
				table[slot >> 4] = (judyslot)judy_alloc (judy, JUDY_radix) | JUDY_radix;

			table = (judyslot *)(table[slot >> 4] & JUDY_mask);
			judy->stack[judy->level].slot = slot;
			next = &table[slot & 0x0F];

			if( !slot ) // leaf?
				return next;
			continue;

		case JUDY_span:
			base = (uchar *)(*next & JUDY_mask);
			node = (judyslot *)((*next & JUDY_mask) + JudySize[JUDY_span]);
			cnt = JUDY_span_bytes;
			tst = cnt;

			if( tst > (int)(max - off) )
				tst = max - off;

			value = strncmp((const char *)base, (const char *)(buff + off), tst);

			if( !value && tst < cnt && !base[tst] ) // leaf?
				return &node[-1];

			if( !value && tst == cnt ) {
				next = &node[-1];
				off += cnt;
				continue;
			}

			//	bust up JUDY_span node and produce JUDY_1 nodes
			//	then loop to reprocess insert

			judy_splitspan (judy, next, base);
			judy->level--;
			continue;
		}
	}

	// place JUDY_1 node under JUDY_radix node(s)

	if( off & JUDY_key_mask && off <= max ) {
		base = (uchar *) judy_alloc (judy, JUDY_1);
		keysize = JUDY_key_size - (off & JUDY_key_mask);
		node = (judyslot  *)(base + JudySize[JUDY_1]);
		*next = (judyslot)base | JUDY_1;

		//	fill in slot 0 with bytes of key

#if BYTE_ORDER != BIG_ENDIAN
		while( keysize )
			if( off + --keysize < max )
				*base++ = buff[off + keysize];
			else
				base++;
#else
		tst = keysize;

		if( tst > (int)(max - off) )
			tst = max - off;

		memcpy (base, buff + off, tst);
#endif
		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].next = *next;
		judy->stack[judy->level].slot = 0;
		judy->stack[judy->level].off = off;
		next = &node[-1];
		off |= JUDY_key_mask;
		off++;
	}

	//	produce span nodes to consume rest of key

	while( off <= max ) {
		base = (uchar *) judy_alloc (judy, JUDY_span);
		*next = (judyslot)base | JUDY_span;
		node = (judyslot  *)(base + JudySize[JUDY_span]);
		cnt = tst = JUDY_span_bytes;
		if( tst > (int)(max - off) )
			tst = max - off;
		memcpy (base, buff + off, tst);

		if( judy->level < judy->max )
			judy->level++;

		judy->stack[judy->level].next = *next;
		judy->stack[judy->level].slot = 0;
		judy->stack[judy->level].off = off;

		next = &node[-1];
		off += tst;
		if( !base[cnt-1] )	// done on leaf
			break;
	}
	return next;
}

#ifdef STANDALONE
int main (int argc, char **argv)
{
uchar buff[1024];
judyslot max = 0;
judyslot *cell;
FILE *in, *out;
void *judy;
uint len;
uint idx;

	if( argc > 1 )
		in = fopen (argv[1], "r");
	else
		in = stdin;

	if( argc > 2 )
		out = fopen (argv[2], "w");
	else
		out = stdout;

	if( !in )
		fprintf (stderr, "unable to open input file\n");

	if( !out )
		fprintf (stderr, "unable to open output file\n");

	judy = judy_open (512);

	while( fgets((char *)buff, sizeof(buff), in) ) {
		len = strlen((const char *)buff);
		buff[--len] = 0;
		if( len && buff[len - 1] == 0x0d ) // Detect and remove Windows CR
			buff[--len] = 0;
		*(judy_cell (judy, buff, len)) += 1;		// count instances of string
		max++;
	}

	cell = judy_strt (judy, NULL, 0);

	if( cell ) do {
		judy_key(judy, buff, sizeof(buff));
		for( idx = 0; idx < *cell; idx++ )		// spit out duplicates
			fprintf(out, "%s\n", buff);
	} while( (cell = judy_nxt (judy)) );

	fprintf(stderr, "%" PRIuint " memory used\n", MaxMem);

#if 1
	// test deletion all the way to an empty tree

	if( cell = judy_prv (judy) )
		do max -= *cell;
		while( cell = judy_del (judy) );

	assert (max == 0);
#endif
	judy_close(judy);
	return 0;
}
#endif


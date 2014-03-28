/********************************************************************\
File:   varray.h
Author: Hansong Zhang
Department of Computer Science
UNC-Chapel Hill

$Id: varray.h,v 1.2 1997/09/10 16:00:59 zhangh Exp $
\********************************************************************/

#ifndef __VARRAY_H__
#define __VARRAY_H__


// #include "alloc.h"
#include <stdlib.h>

#include <assert.h>
#include <stdio.h>

//#define DEBUG_ARRAY

template <class T>
class VArray {
public:
  typedef unsigned int INT;
protected:
	INT curLen;
	INT curMaxLen;
	T* array;
public:
	VArray() {
		curLen = curMaxLen = 0;
		array = NULL;
	}

	VArray(INT initMaxLen) {
		init(initMaxLen);
	}
	VArray(INT initMaxLen, INT initVal) {
		init(initMaxLen, initVal);
	}

	void init(INT initMaxLen) {
		curLen = 0;
		curMaxLen = initMaxLen;
		array = (T*)malloc(curMaxLen * sizeof(T));
		assert (array != NULL);
	}

	void init(INT initMaxLen, INT initVal) {
		curLen = 0;
		curMaxLen = initMaxLen;
		array = (T*)malloc(curMaxLen * sizeof(T));
		assert (array != NULL);
		memset(array, initVal, curMaxLen*sizeof(T));
	}

	inline INT getLen() { return curLen; }
	inline INT getNum() { return curLen; }
	inline void setLen(INT len) {
		curLen = len;
		if (curMaxLen < len) {
			curMaxLen = len;
			if (array)
				array = (T*)realloc(array, curMaxLen*sizeof(T));
			else
				array = (T*)malloc(curMaxLen*sizeof(T));
		}
		assert (array != NULL);
	}
	inline void setLen(INT len,const T &val) {
		curLen = len;
		if (curMaxLen < len) {
			curMaxLen = len;
			if (array)
				array = (T*)realloc(array, curMaxLen*sizeof(T));
			else
				array = (T*)malloc(curMaxLen*sizeof(T));
		}
		
		for (INT i=0;i<len;++i) {
			array[i] = val;
		}
		
		assert (array != NULL);
	}
	inline void resize(INT len) {
		curMaxLen = len;
		array = (T*)realloc(array, curMaxLen*sizeof(T));
		assert (array != NULL);
	}
	inline void reset() { curLen=0; }
	inline T& append() { 
		if (curLen < curMaxLen)
			curLen ++; 
		else {
			curMaxLen = (INT)(curMaxLen*1.3f);
			if (array)
				array = (T*)realloc(array, curMaxLen*sizeof(T));
			else
				array = (T*)malloc(curMaxLen*sizeof(T));

			assert(array != NULL);
			curLen ++;
		}
		return array[curLen-1];
	}

	inline void append(const T& data) {
		if (curMaxLen == 0) {
			curLen = 0;
			curMaxLen = 8;
			array = (T*)malloc(curMaxLen * sizeof(T));
		}

		if (curLen < curMaxLen) {
			array[curLen ++] = data;
		} else {
			// fprintf(stderr, "cur:%d, max:%d\n", curLen, curMaxLen);
			curMaxLen = (INT)(curMaxLen*1.3f);
			if (array)
				array = (T*)realloc(array, curMaxLen*sizeof(T));
			else
				array = (T*)malloc(curMaxLen*sizeof(T));
			assert(array != NULL);
			array[curLen ++] = data; 
		}
	}

	void fit() {
		if (curLen != curMaxLen) {
			array = (T*)realloc(array, curLen*sizeof(T));
			assert(array != NULL);
			curMaxLen = curLen;
		}
	}

	inline T &operator[](INT i) { 
		//if (array && i < curLen)
			return array[i];
		//else {
		//	fprintf(stderr, "VArray(%p)::[] reference out of bound (idx:%d size:%d)\n", this, i, curLen);
		//	assert(0);
		//	return array[0];
		//}
	}

	inline T & get(INT i) {
#ifdef DEBUG_ARRAY
		if (i >= curLen) {
			fprintf(stderr, "VArray(%p)::get() reference out of bound (%d).\n", this, i);
			assert(0);
			return array[0];
		}
#endif
		return (*this)[i];
	}

	inline T & _get(INT i) {
#ifdef DEBUG_ARRAY
		if (i >= curLen) {
			fprintf(stderr, "VArray(%p)::_get() reference out of bound (%d).\n", this, i);
			assert(0);
			return array[0];
		}
#endif
		return array[i];
	}

	inline void set(INT i, const T& elem) {
#ifdef DEBUG_ARRAY
		if (array && i >= curLen) {
			fprintf(stderr, "VArray(%p)::set() out of range (%d).\n", this, i);
		}
#endif
		array[i] = elem;
	}

	inline void _set(INT i, const T& elem) {
		array[i] = elem;
	}

	inline T & last() {
		return array[curLen-1];
	}

	inline T *getArray() {
		return array;
	}
	inline T *dupArray() {
		T *ret = (T*)malloc(curLen * sizeof(T));
		if (! ret) {
			fprintf(stderr, "Cannot alloc mem in dupArray().\n");
			return NULL;
		}
		memcpy(ret, array, curLen * sizeof(T));
		return ret;
	}

	~VArray() { if (array) free(array); }
	void freemem() { 
		if (array) {
			free(array);
			curLen = curMaxLen = 0;
		}
	}

	void clear() {
		if (array)
			memset(array, 0, sizeof(T)*curMaxLen);
	}
    
};

/*
template <class T>
class VPool : public VArray<T> {
	public:
	VPool(INT size):VArray<T>(size) {}	
	T& alloc(INT * offset=NULL) {
		append();
		if (offset) *offset = curLen-1;
		return array[curLen-1];
	}
};
*/



#endif

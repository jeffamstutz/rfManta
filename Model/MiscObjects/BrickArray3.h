
/*
 *  BrickArray3.h: Interface to dynamic 3D array class
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 * 
 *  Ported into Manta by Aaron Knoll.
 *  XXX - this should ideally be in Core/Containers. Move if necessary.
 */

#ifndef _MANTA_BRICKARRAY3_H_
#define _MANTA_BRICKARRAY3_H_

#include <math.h>

#define CACHESIZE	64
#define PAGESIZE	4096

template<class T>
class BrickArray3 {
    //this is the data
    T* objs;

    char* data;
    
    //use this to make sure the data is deleted only after all the referers are done with it
    int* refcnt;

public:
    //these are index arrays, the data is reorganized so that the memory for elem[x,y,z] is close to elem[x+-q,y+-q,z+-q]
    long* idx1;
    long* idx2;
    long* idx3;

    int dm1;
    int dm2;
    int dm3;
    int totaldm1;
    int totaldm2;
    int totaldm3;

    int L1, L2;
    void allocate();
    BrickArray3<T>& operator=(const BrickArray3&);
public:
    BrickArray3();
    BrickArray3(int, int, int);
    ~BrickArray3();

    //when you refer to an element, use the index arrays to find it's optimized memory location first
    inline T& operator()(int d1, int d2, int d3) const
	{
		return objs[idx1[d1]+idx2[d2]+idx3[d3]];
	}

    //accessors
    inline int dim1() const {return dm1;}
    inline int dim2() const {return dm2;}
    inline int dim3() const {return dm3;}

    void resize(int, int, int);

    void initialize(const T&);

    //allow the user to manipulate the data directly
    inline T* get_dataptr() {return objs;}

    //how big is it?
    inline long get_datasize()
    {
        return totaldm1*long(totaldm2*totaldm3*sizeof(T));
    }

    //allow more than one BrickArray to share the same data
    void share(const BrickArray3<T>& copy);
};

template<class T>
BrickArray3<T>::BrickArray3()
{
	objs=0;
	data=0;
	dm1=dm2=dm3=0;
	totaldm1=totaldm2=totaldm3=0;
	idx1=idx2=idx3=0;
	L1=L2=0;
}

template<class T>
void BrickArray3<T>::allocate()
{
	if(dm1==0 || dm2==0 || dm3==0){
		objs=0;
		data=0;
		refcnt=0;
		dm1=dm2=dm3=0;
		totaldm1=totaldm2=totaldm3=0;
		idx1=idx2=idx3=0;
		L1=L2=0;
		return;
	}
    
    //CACHESIZE byte cache line
    //CACHESIZE = sizeof(T) * L1^3
    //L1 = 3rdrt(CACHESIZE/sizeof(T))
	L1=(int)(pow((CACHESIZE*1.0)/(double)(sizeof(T)), 1./3.)+.1);

    // 16K page size
    //bricksize = (sizeof(T)*L1)^3
    //16K = bricksize * L2^3
    //L2 = 3rdrt(16K/bricksize)
	L2=(int)(pow((PAGESIZE*1.0)/(sizeof(T)*L1*L1*L1), 1./3.)+.1);

    //pad to make an even number of bricks in each dimension
	long totalx=(dm1+L2*L1-1)/(L2*L1);
	long totaly=(dm2+L2*L1-1)/(L2*L1);
	long totalz=(dm3+L2*L1-1)/(L2*L1);

    //create the x address index table
	idx1=new long[dm1];
	for(int x=0;x<dm1;x++){
		int m1x=x%L1;
		int xx=x/L1;
		int m2x=xx%L2;
		int m3x=xx/L2;
		idx1[x]=
				m3x*totaly*totalz*L2*L2*L2*L1*L1*L1+
				m2x*L2*L2*L1*L1*L1+
				m1x*L1*L1;
	}
    //create the y address index table
	idx2=new long[dm2];
	for(int y=0;y<dm2;y++){
		int m1y=y%L1;
		int yy=y/L1;
		int m2y=yy%L2;
		int m3y=yy/L2;
		idx2[y]=
				m3y*totalz*L2*L2*L2*L1*L1*L1+
				m2y*L2*L1*L1*L1+
				m1y*L1;
	}
    //create the z address index table
	idx3=new long[dm3];
	for(int z=0;z<dm3;z++){
		int m1z=z%L1;
		int zz=z/L1;
		int m2z=zz%L2;
		int m3z=zz/L2;
		idx3[z]=
				m3z*L2*L2*L2*L1*L1*L1+
				m2z*L1*L1*L1+
				m1z;
	}

    //these are the padded dimensions
	totaldm1=totalx*L2*L1;
	totaldm2=totaly*L2*L1;
	totaldm3=totalz*L2*L1;
	long totalsize=totaldm1*long(totaldm2*totaldm3);

    //create the space to hold the data
    //objs=new T[totalsize];
    //the padding by CACHESIZE+PAGESIZE followed by 
    //the %CACHESIZE aligns start on a double word boundary.
	data=new char[totalsize*sizeof(T)+CACHESIZE+4096]; //original used 4096, not 16384, why?
	long off=long(data)%CACHESIZE;

	if (off)
		objs=(T*)(data+CACHESIZE-off);
	else
		objs=(T*)data;
    
    //there is at least one referer to the data
	refcnt = new int;
	*refcnt=1;
}

template<class T>
		void BrickArray3<T>::resize(int d1, int d2, int d3)
{
	if(objs && dm1==d2 && dm2==d2 && dm3==d3)return;
	dm1=d1;
	dm2=d2;
	dm3=d3;
	if (objs) {
		(*refcnt--);
		if (*refcnt == 0) {
			delete[] data;
			delete[] idx1;
			delete[] idx2;
			delete[] idx3;
			delete refcnt;
		}
	}
	allocate();
}

template<class T>
		BrickArray3<T>::BrickArray3(int dm1, int dm2, int dm3)
	: dm1(dm1), dm2(dm2),dm3(dm3)
{
	allocate();
}

template<class T>
		BrickArray3<T>::~BrickArray3()
{
	if(objs){
		if(*refcnt == 0){
			delete[] data;
			delete[] idx1;
			delete[] idx2;
			delete[] idx3;
			delete refcnt;
		}
	}
}

template<class T>
		void BrickArray3<T>::initialize(const T& t)
{
	long n=dm1*long(dm2*dm3);
	for(long i=0;i<n;i++)
		objs[i]=t;
}

template<class T>
		void BrickArray3<T>::share(const BrickArray3<T>& copy)
{
	if(objs){
		(*refcnt--);
		if(*refcnt == 0){
			delete[] data;
			delete[] idx1;
			delete[] idx2;
			delete[] idx3;
		}
	}
	objs=copy.objs;
	data=copy.data;
	refcnt=copy.refcnt;
	dm1=copy.dm1;
	dm2=copy.dm2;
	dm3=copy.dm3;
	totaldm1=copy.totaldm1;
	totaldm2=copy.totaldm2;
	totaldm3=copy.totaldm3;
	idx1=copy.idx1;
	idx2=copy.idx2;
	idx3=copy.idx3;
	L1=copy.L1;
	L2=copy.L2;
	(*refcnt)++;
}

#endif

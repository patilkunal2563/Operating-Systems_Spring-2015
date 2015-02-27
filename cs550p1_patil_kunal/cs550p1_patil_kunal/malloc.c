#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utility.h"
#include <sys/mman.h>

void *malloc(size_t sz);
int freeListInit();

void *
cs550_mmap_wrapper(size_t sz);
int
cs550_munmap_wrapper(void *vp, size_t sz);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

size_t chunk_size = 128 * 1024 * 1024;
int initialized = 0;
static void * mymalloc_freelist[32];

struct freeList_struct {

	//char *addr;
	struct freeList_struct *next;

};

size_t roundup2(size_t v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
	return v;
}

size_t cs550_get_chunk_size() {
	size_t size = chunk_size;
	return size;
}

void cs550_set_chunk_size(size_t size) {
	chunk_size = size;
}

static void split(char * const begin, char * const end, size_t allocated) {

	size_t newSize = end - begin;
	if (allocated < newSize) {
				size_t actualSize = (newSize / 2);
				char *half_way = begin + actualSize;
				struct freeList_struct *ptr = (void *) half_way;
				int index = log2(end - half_way);
				mymalloc_freelist[index] = ptr;
				ptr->next = NULL;
				// Only one way split recurrsion implemented so that big memeory block are also available in freelist which reduces mmap calls
				// It also increases performance of "this" program slightly 
				split(begin, half_way, allocated);


	} //else if (allocated == newSize) {

		
	// struct BlockHdr *b = (struct BlockHdr *) begin;
		//b->chunk = ch;
		//b->log2_capacity = ilog2(sz);
		//b->next = heads[b->log2_capacity];
		//heads[b->log2_capacity] = b;
		//  return begin;
		//allocate that memmory
		//  ////////cs550_print("found\t%lu\t%lu\t%lu\n\n",begin,end,allocated);

	//}
}

void *malloc(size_t sz) {

	if (sz <= 0) {
		return NULL;
	} else {

		sz += 8;

		sz = roundup2(sz);
		if (initialized == 0) {
			freeListInit();
			if (sz >= chunk_size) {
				
				void *vp = cs550_mmap_wrapper(sz);
											char *currentPointer = vp;
											*(size_t *) currentPointer = sz;
											currentPointer += 8;
											initialized = 1;
											return currentPointer;
				
				
			
			
			} else {
				void *vp = cs550_mmap_wrapper(chunk_size);
												char *currentPointer = vp;
												*(size_t *) currentPointer = sz;
												split(currentPointer, currentPointer + chunk_size, sz);
												currentPointer += 8;
												initialized = 1;
												return currentPointer;
				
			}
		} else {
			int curr_index = log2(sz);
			int i;
			
			for (i = curr_index; i < 32; i++) {
				int newSize = pow(2, i);
				if (mymalloc_freelist[i]) {
					struct freeList_struct * ptr4 = mymalloc_freelist[i];
					void *vp = ptr4;
					char *currentPointer = vp;
					if (ptr4->next == NULL) {
						split(currentPointer, currentPointer + newSize, sz);
						mymalloc_freelist[i] = NULL;
					} else {
						struct freeList_struct * ptr5 = ptr4->next;
						mymalloc_freelist[i] = ptr5;
						ptr4->next = NULL;
						split(currentPointer, currentPointer + newSize, sz);
					
					}
				
					*(size_t *) currentPointer = sz;
					currentPointer += 8;
					return currentPointer;
				}

			}
		
				if (sz <= chunk_size) {
					//////cs550_print("Requesting more memory");
					// Requested memory is less than chunk size
					// Freelist do not have block of memory which is currently requested neither next greater memory blocks
					// blocks are present so we should call mmap to get more chunk
					void *vp = cs550_mmap_wrapper(chunk_size);
					char *currentPointer = vp;

					*(size_t *) currentPointer = sz;
					split(currentPointer, currentPointer + chunk_size, sz);

					currentPointer += 8;
					initialized = 1;
					return currentPointer;

				} else {
					// Requested memory is more than chunk size
					void *vp = cs550_mmap_wrapper(sz);
					char *currentPointer = vp;
					*(size_t *) currentPointer = sz;
					currentPointer += 8;

					////cs550_print("NOT INIT > return address::%lu\tActual Address::%lu\tsize::%lu\n",cp,cp-8,sz);
					return currentPointer;
				}
			
		}

	}
	return NULL;
}

void free(void *vp) {
	////cs550_print("\nIN FREE");
	////cs550_print("%lu\t%lu",vp,&vp);
	if ((vp != 0) || (vp != NULL)) {
		char *cureentPointer = vp;
		cureentPointer -= 8;
		// Extract the size.
		size_t actualSize = *(size_t *) cureentPointer;

		//    ////cs550_print("free() called on address %lu \tActual address(cp-8)::%lu\tsize::%lu\n",vp,cp,sz);

		////cs550_print("\nCHIU Freed %lu\t at address \t %lu\n",sz,cp);
		int index = log2(actualSize);

		if (mymalloc_freelist[index]) {
			struct freeList_struct * tempPtr = mymalloc_freelist[index];
			struct freeList_struct *ptr = (void *) cureentPointer;
			ptr->next = tempPtr;
			mymalloc_freelist[index] = ptr;

		} else {

			struct freeList_struct * tempPtr = (void *) cureentPointer;
			tempPtr->next = NULL;
			mymalloc_freelist[index] = tempPtr;
			// put error msg

		}
		// mymalloc_freelist[index] = cp;
	}
}

void *calloc(size_t nmemb, size_t size) {
	//cs550_print("\nIn CALLOC - Size -> %lu \t NMEMB -> %lu \n",size,nmemb);

	// If nmemb - Number of elements are zero then return NULL 
	// If size is zero

	size_t total = nmemb * size;

	//Basically calloc allocates allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory.
	// so total is multiplication of number of elements and size = total size required for those number of elements
	// Allocate total size using malloc and get the pointer

	void *ptr = malloc(total);
	if (ptr)

		// memset will set that memory size to zero which is the only reason why calloc is different than malloc
		// It copies the 0 to the first n elements pointed by the argument ptr.

		return memset(ptr, 0, size * nmemb);
	else
		return NULL;
}
void *realloc(void *ptr, size_t size) {
	// This function attempts to resize the memory block pointed to by ptr that was previously allocated with a call to malloc or calloc.
	// first two if and else-if conditions checks if size and ptr are valid if size is not valid then it will free that pointer (which is logical)
	// if ptr is not valid then we allocate that size using malloc and return that pointer to caller.
	if (size && !ptr) {

		return malloc(size);
	} else if (!size && ptr) {
		free(ptr);
				return NULL;
		
	} else {

		size_t sizeAdjust = size + 8;
		sizeAdjust = roundup2(sizeAdjust);

	

	char *currentPointer = ptr;
	// To get the actual size of the allocated block I am moving pointer back by 8 so I know what actual size is
	currentPointer -= 8;
	size_t sz = *(size_t *) currentPointer;

	if (sizeAdjust > sz) {

		void *newPtr = malloc(size);
				//If the capacity of the block is not enough to accommodate the new size, then a new block must be allocated, and the contents copied.
				memcpy(newPtr, ptr, sz - 8);
				free(ptr);
				return newPtr;
		

	} else {
		// To give size without giving pointer to my overhead, moving pointer forward by 8 which gives requested size rounded up to 8
				//If the capacity of the block is sufficient to accommodate the new size, then the current block should be just have the size updated internally. 
				//In this case, no copying should occur.
				return currentPointer + 8;
	}

}
}



int freeListInit() {
	// Basically I am initializing each elements of freeList array to NULL with simple for-loop
	int freeListIndex; 
	for (freeListIndex = 0; freeListIndex < 32; freeListIndex++)
		mymalloc_freelist[freeListIndex] = NULL;
	return 1;

}
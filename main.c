#include <stdio.h>
#include <stdio.h>
#include <zconf.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdint-gcc.h>

#define HEAP_SIZE 0x2000 // size of the fixed-allocation
#define Meta_Size sizeof(Meta_data)  //size of Meta_size
#define Find_Data(ptr)((void*)((unsigned long)ptr+sizeof(Meta_data)))//Find the pointer to the data
#define Find_Meta(ptr)((void*)((unsigned long)ptr-sizeof(Meta_data)))//Find Meta_data
/*
list[0]->2 for size 2
list[1]->4 for size 4
list[2]->8 for size 8
list[3]->16 for size 16
list[4]->32 for size 32
list[5]->64 for size 64
list[6]->128 for size 128
list[7]->256 for size 256
list[8]->512 for size 512
list[9]-> for all larger size ,can spilt and merge

 how  list[0] to list [8] work ?
 It add the item to the freelist when user calls free
  It remove the item from the freelist if user finds a free item
  can't perform merge or spilt function

  how list[9] work?
  it always add the item to the list ,it can perform best fit algorithm,merge free space with previous free space and next free space ,spilt between two blocks
   it can split the smallest free memory region of sufficient size, return any excess memory to the appropriate free list
*/
typedef struct Meta_data {
    size_t size;//size of the Meta_data
    struct Meta_data *next;//pointer to the next of the Meta data
    struct Meta_data *prev;//pointer to the previous of the Meta data
    int free;//to check if is free or not
} Meta_data;


Meta_data *list[10] = {NULL};//list for searching
int ArrayLength = 10;
Meta_data *head = NULL;
void *bottom;
size_t memory = HEAP_SIZE;//use this to allocate memory again
Meta_data *lastVisited = NULL;

/**
 * @param base
 * @param exp
 * @return  base^exp
 */
int ipow(int base, int exp) {
    int result = 1;
    for (;;) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return result;
}

/*
 * perform a log2 operation
 */
uint16_t log2(uint32_t n) {
    if (n == 0) //throw ...
    {
    }
    uint16_t logValue = -1;
    while (n) {//
        logValue++;
        n >>= 1;
    }
    return logValue;
}

/**
 * perform a log operation
 */
uint32_t log(uint32_t a, uint32_t b) {
    return log2(a) / log2(b);
}

/**
 * perform a log operation with the  correct base
 * @param x log base that you want to use
 * @return
 */
int mylog(double x) {
    return log(x, 2);
}

/**
 * return a correct index using the size
 * @param s size of the data
 * @return
 */
int getindex(size_t s) {
    if (s == 16 || s == 32 || s == 64 || s == 128 || s == 256 || s == 512 || s == 1024 || s == 2048 || s == 2 ||
        s == 4 || s == 8) {
        return mylog(s) - 1;
    }
    return mylog(s);
}

/**
 * spilt a chunk into two chunks
 * @param ptr use this to spilt
 * @param size size that you want to allocate
 */
void spilt(Meta_data *ptr, size_t size) {
    Meta_data *new = ((void *) ptr + size + sizeof(Meta_data));
    new->size = ptr->size - size - sizeof(Meta_data);
    new->free = 1;
    new->prev = ptr;
    new->next = ptr->next;
    ptr->size = size;
    ptr->free = 0;
    ptr->next = new;
}

/**
 * increase the location of the head ,
 * @param size size that you want to allocate
 * return a pointer for the Meta_data
 */
Meta_data *newspilt(size_t size) {
    Meta_data *ptr = head;
    ptr->size = size;
    ptr->free = 0;
    head = ((void *) head + size + Meta_Size);
    memory = memory - size;
    return ptr;
}

/**
 *Find data in the freelist and remove it
 * @param size use he size to get correct index
 * @return
 */
Meta_data *FindMetaData(size_t size) {
    int index = getindex(size);
    Meta_data *header = list[index];
    if (header == NULL)
        return NULL;
    else if (header->next != NULL) {
        header->next->prev = NULL;
        header->free = 0;
        list[index] = header->next;
        header->next = NULL;
    } else {
        list[index] = NULL;
    }
    return header;
}

/*
 * make sure index is 9 w
 */
int checkIndex(int index) {
    if (index >= 9) {
        return 9;
    }
}

/**
 * allocate another 8k if run out of memory
 */
void IncreaseMemory() {
    head = sbrk(HEAP_SIZE);
    memory = HEAP_SIZE;
}

/**
 * use best fit algorithm to find the freeblock
 * @param current  head of the  list[9]
 * @param size size that you want to find
 * @return
 */
Meta_data *findBlock(Meta_data *current, size_t size) {
    Meta_data *ptr = current;
    Meta_data *small = NULL;
    while (ptr != NULL) {
        if (ptr->size == size && ptr->free) {
            return ptr;
        }
        if (ptr->size > size && ptr->free) {
            if (small == NULL)
                small = ptr;
            else if (small->size > ptr->size) {
                small = ptr;
            }
        }
        lastVisited = ptr;
        ptr = ptr->next;

    }
    ptr = small == NULL ? ptr : small;
    return ptr;
}

/**
 * allocate another 8k if list[9] run out of memory
 * @param lastVisitedPtr
 * @param size
 * @return
 */
Meta_data *IncreaseMemoryForLargeBlock(Meta_data *lastVisitedPtr, size_t size) {
    Meta_data *ptr = sbrk(0);
    sbrk(HEAP_SIZE);
    ptr->size = HEAP_SIZE - sizeof(Meta_data);
    ptr->free = 0;
    ptr->next = NULL;
    ptr->prev = lastVisitedPtr;
    lastVisitedPtr->next = ptr;
    if (ptr->size > size + sizeof(Meta_data)) {
        spilt(ptr, size);
    }
    return ptr;
}

/**
 * merge previous data with the data, it's for list[9]
 * @param freed
 */
void mergeNext(Meta_data *freed) {
    Meta_data *next;
    next = freed->next;
    if (next != NULL && next->free == 1) {
        freed->size = freed->size + next->size + sizeof(Meta_data);
        freed->next = next->next;
        if ((next->next) != NULL)
            (next->next)->prev = freed;
    }
}

/**
 * merge next data with the data ,It's for list[9]
 * @param freed
 */
void mergePrev(Meta_data *freed) {
    Meta_data *prev;
    prev = freed->prev;
    if (prev != NULL && prev->free == 1) {
        prev->size = prev->size + freed->size + sizeof(Meta_data);
        prev->next = freed->next;
        if (freed->next != NULL)
            (freed->next)->prev = prev;
    }
}

/**
 * perform various operations for list[9]
 *  return a free memory area form list[9]
 */
void *LargeBlockoperation(size_t size) {
    Meta_data *ptr;
    if (list[ArrayLength - 1] == NULL) {
        list[ArrayLength - 1] = sbrk(0);
        sbrk(HEAP_SIZE);
        ptr = list[ArrayLength - 1];
        ptr->size = HEAP_SIZE - sizeof(Meta_data);
        ptr->free = 0;
        ptr->prev = NULL;
        ptr->next = NULL;
        if (ptr->size > size + sizeof(Meta_data)) {
            spilt(ptr, size);
        }
        return Find_Data(ptr);
    } else {
        Meta_data *FreeBLock = findBlock(list[ArrayLength - 1], size);
        if (FreeBLock == NULL) {
            FreeBLock = IncreaseMemoryForLargeBlock(lastVisited, size);
            if (FreeBLock == NULL) {
                return NULL;
            }
            return Find_Data(FreeBLock);
        } else {
            if (FreeBLock->size > sizeof(Meta_data) + size)
                spilt(FreeBLock, size);
            else if (FreeBLock->size >= size) {
                FreeBLock->free = 0;
            }
        }
        return Find_Data(FreeBLock);
    }
}

/**
 *  allocates the requested memory and returns a pointer to it.
 * @param size size that you want to allocate
 * @return
 */
void *my_malloc(size_t size) {
    if(size>HEAP_SIZE||size==0) {
        return NULL;
    }
    Meta_data *ptr, *free;
    int index;
    void *large;
    index = getindex(size);
    index = checkIndex(index);
    size_t allocated = ipow(2, index + 1);
    allocated = index >= 9 ? size : allocated;
    if (index == 9) {
        large = LargeBlockoperation(allocated);
        return large;
    }
    if (head == NULL) {
        head = sbrk(HEAP_SIZE);
        ptr = newspilt(allocated);
        return Find_Data(ptr);
    }
    free = FindMetaData(allocated);
    if (free == NULL && memory < size + Meta_Size) {
        IncreaseMemory();
        ptr = newspilt(allocated);
        return Find_Data(ptr);
    } else if (free == NULL) {
        ptr = newspilt(allocated);
        return Find_Data(ptr);
    } else {
        return Find_Data(free);
    }
}

/**
 * add the item to a freelist
 * @param new the item
 */
void addItem(Meta_data *new) {
    int index = getindex(new->size);
    Meta_data *curr = list[index];
    if (curr == NULL) {
        list[index] = new;
        new->next = NULL;
        return;
    }
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = new;
    new->prev = curr;
    new->next = NULL;
}

/**
 * deallocates the memory previously allocated by a call to calloc, malloc, or realloc.
 * @param ptr This is the pointer to a memory block previously allocated with malloc
 */
void my_free(void *ptr) {
    Meta_data *free = Find_Meta(ptr);
    int index = getindex(free->size);
    index = checkIndex(index);
    if (index == 9) {
        free->free = 1;
        mergeNext(free);
        mergePrev(free);
    } else {
        free->free = 1;
        addItem(free);
    }
}

/**
 * print infomation about a freelist
 * @param headptr head of the list
 */
void printList(Meta_data *headptr) {
    size_t size = 0;
    int i = 0;
    Meta_data *p = headptr;
    if (p != NULL) {
        while (p != NULL) {
            printf("[%d] p: %p\n", i, p);
            printf("[%d] p->size: %zu\n", i, p->size);
            printf("[%d] p->prev: %p\n", i, p->prev);
            printf("[%d] p->next: %p\n", i, p->next);
            printf("[%d] p->free: %d\n", i, p->free);
            if (p->free)
                size += p->size;
            i++;
            p = p->next;
        }
        printf(" %s %zu \n", "Total size:", size);
        printf("__________________________________________________\n");
    } else {
        printf("__________________________________________________\n");
    }
}

/*
 * print information about all the lists
 */
void printlists() {
    int power = 2;
    for (int i = 0; i < ArrayLength; i++) {
        if (i == 9) {
            printf("%s  %d %s %s \n", "list:", i, "  list size:", "for all larger size");
        } else {
            printf("%s  %d %s %d \n", "list:", i, "  list size:", power);
            power *= 2;
        }
        printList(list[i]);
    }
}

/**
 * Provide a program to test my program
 */
void Operation() {
    size_t size;
    char free;
    void *p;
    void *allocate;
    int number = 1;
    while (1) {
        printf(" m for allocate ,f for \n");
        scanf(" %c", &free);

        if (free == 'f') {
            printf("enter the address of the pointer please\n ");
            scanf("%p", &p);
            my_free(p);
            printlists();
        } else if (free == 'm') {
            printf("enter the size please  \n ");
            scanf("%zu", &size);
            allocate = my_malloc(size);
            printf(" %d %s %p \n", number, " :allocated address: ", allocate);
            number++;
            printlists();
        }
    }
}

int main() {
    Operation();
}




// Max-Heap data structure in C

#include "types.h"
#include "priorityQueue.h"
//int size = 0;



void swapHeapNode(struct heap_p *a, struct heap_p *b) {
  struct heap_p temp = *b;
  *b = *a;
  *a = temp;
}

struct heap_p peekHeap(struct heap_p array[]){
    return array[0];
}

// Function to heapify the tree
void heapify(struct heap_p array[], int size, int i) {
  if (size == 1) {
    return;
  } else {
    // Find the largest among root, left child and right child
    int smallest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;
    if (l < size && array[l].priority < array[smallest].priority)
      smallest = l;
    if (r < size && array[r].priority < array[smallest].priority)
      smallest = r;

    // Swap and continue heapifying if root is not smallest
    if (smallest != i) {
      swapHeapNode(&array[i], &array[smallest]);
      heapify(array, size, smallest);
    }
  }
}

// Function to insert an element into the tree
void insertHeap(struct heap_p array[], struct heap_p newHeap,int *sizePtr) {
  if (*sizePtr == 0) {
    array[0] = newHeap;
    *sizePtr += 1;
  } else {
    array[*sizePtr] = newHeap;
    *sizePtr += 1;
    for (int i = *sizePtr / 2 - 1; i >= 0; i--) {
      heapify(array, *sizePtr, i);
    }
  }
}

// Function to delete an element from the tree
struct heap_p deleteRoot(struct heap_p array[], int index,int *sizePtr) {
    int i;
    for (i = 0; i < *sizePtr; i++) {
        if (index == array[i].index)
        break;
    }


    if(i == *sizePtr)
        return (struct heap_p){-1,-1};

    struct heap_p out = array[i];

    swapHeapNode(&array[i], &array[*sizePtr - 1]);
    *sizePtr -= 1;
    for (int i = *sizePtr / 2 - 1; i >= 0; i--) {
        heapify(array, *sizePtr, i);
    }

    return out;
}

struct heap_p extractMin(struct heap_p array[],int *sizePtr){
    if (*sizePtr == 1) 
    { 
        *sizePtr-=1; 
        return array[0]; 
    } 
  
    // Store the minimum value, and remove it from heap 
    struct heap_p root = array[0]; 
    array[0] = array[*sizePtr-1]; 
    *sizePtr-=1; 
    heapify(array,*sizePtr,0); 
  
    return root; 
}

struct heap_p findInHeap(struct heap_p array[],int pageNum,int *sizePtr){
  struct heap_p temp = deleteRoot(array,pageNum,sizePtr);
  if (temp.index != -1){
    insertHeap(array,temp,sizePtr);
  }
  return temp;
  
}
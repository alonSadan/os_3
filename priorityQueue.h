struct heap_p{
    int index;
    uint priority;
}heap_p;

void heapify(struct heap_p array[], int size, int i);
void insertHeap(struct heap_p array[], struct heap_p newHeap,int *sizePtr);
struct heap_p deleteRoot(struct heap_p array[], int index,int *sizePtr);
struct heap_p extractMin(struct heap_p array[],int *sizePtr);
struct heap_p peekHeap(struct heap_p array[]);
struct heap_p findInHeap(struct heap_p array[],int pageNum,int *sizePtr);

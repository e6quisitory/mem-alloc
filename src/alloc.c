#include <alloc.h>
#include <stdbool.h>
#include <unistd.h>

// Header
typedef struct header blockHeader;
static const size_t blockHeaderSize = sizeof(blockHeader);

// Current chosen algorithm for finding an empty block
static enum algs currChosenAlg;

// Free blocks linked list head
static blockHeader *head = NULL;

// Heap stuff
static size_t heapSize = 0;
static size_t heapLimit = 0;
static void *initialBreak = NULL;
static void *currBreak = NULL;

// First run of alloc/dealloc (or resetted using allocopt())
static bool firstRun = true;

void *PerformAllocation(blockHeader *currBlock, blockHeader *prevBlock,
                        int numBytesRequested) {

  size_t totalNumBytesToAlloc = numBytesRequested + blockHeaderSize;
  size_t currBlockTotalSize = currBlock->size;
  size_t remainingSize = currBlockTotalSize - totalNumBytesToAlloc;

  bool perfectFit = currBlockTotalSize == totalNumBytesToAlloc;
  bool remainingSizeTooSmall = currBlockTotalSize > totalNumBytesToAlloc &&
                               remainingSize <= blockHeaderSize;
  bool remainingSizeLargeEnough = currBlockTotalSize > totalNumBytesToAlloc &&
                                  remainingSize > blockHeaderSize;

  // Case: block is (a perfect fit) or (larger but cannot split)
  if (perfectFit || remainingSizeTooSmall) {

    // Delete node from free list entirely
    if (currBlock == head) { // currBlock is head
      head = currBlock->next;
      currBlock->next = NULL;
    } else if (currBlock->next == NULL) { // currBlock is tail
      prevBlock->next = NULL;
    } else { // currBlock is somewhere in the middle of the list
      prevBlock->next = currBlock->next;
      currBlock->next = NULL;
    }

    return (void *)(currBlock + 1);

    // Case: block is larger than needed & splitting -> remaining size >
    // header size
  } else if (remainingSizeLargeEnough) {

    blockHeader *newFreeBlockHeader =
        (blockHeader *)((char *)currBlock + blockHeaderSize +
                        numBytesRequested);
    newFreeBlockHeader->size =
        currBlockTotalSize - (blockHeaderSize + numBytesRequested);
    currBlock->size = blockHeaderSize + numBytesRequested;

    if (currBlock == head) {
      newFreeBlockHeader->next = currBlock->next;
      head = newFreeBlockHeader;
    } else if (currBlock->next == NULL) {
      prevBlock->next = newFreeBlockHeader;
      newFreeBlockHeader->next = NULL;
    } else {
      prevBlock->next = newFreeBlockHeader;
      newFreeBlockHeader->next = currBlock->next;
    }
    currBlock->next = NULL;

    return (void *)(currBlock + 1);
  }
}

bool IncreaseHeapSize() {
  if (heapSize + INCREMENT > heapLimit) {

    return false;

  } else {

    // If firstRun, initialBreak (base) must be set
    if (firstRun) {
      initialBreak = sbrk(0);
    }

    // Increment program break by INCREMENT
    currBreak = (void *)((char *)sbrk(INCREMENT) + INCREMENT);
    heapSize += INCREMENT;

    // See if coalescing is possible
    blockHeader *currBlock = head;
    while (currBlock != NULL) {
      void *endOfBlock = (void *)((char *)currBlock + currBlock->size);
      if (endOfBlock == (void *)((char *)currBreak - INCREMENT)) {
        currBlock->size += INCREMENT;
        return true;
      }
      currBlock = currBlock->next;
    }

    // If coalescing is not possible (or firstRun), create new node out of heap
    blockHeader *newBlock = (blockHeader *)((char *)currBreak - INCREMENT);
    newBlock->size = INCREMENT;
    newBlock->next = head;
    head = newBlock;
    return true;
  }
}

void allocopt(enum algs algToSet, int heapLimitSpec) {
  currChosenAlg = algToSet;
  heapLimit = heapLimitSpec;

  // Erase heap entirely (if not firstRun)
  if (heapSize > 0) {
    intptr_t toReduceBy = -((intptr_t)heapSize);
    sbrk(toReduceBy);
    heapSize = 0;
  }

  head = NULL;
  firstRun = true;
}

struct allocinfo allocinfo(void) {
  int totalFreeBytes = 0;

  blockHeader *currBlock = head;
  while (currBlock != NULL) {
    totalFreeBytes += currBlock->size - blockHeaderSize;
    currBlock = currBlock->next;
  }

  struct allocinfo toReturn;
  toReturn.free_size = totalFreeBytes;

  return toReturn;
}

void DeleteBlockFromFreeList(blockHeader *blockToDelete,
                             blockHeader *blockToDeletePrev) {
  if (blockToDelete == head) { // head
    head = blockToDelete->next;
  } else if (blockToDelete->next == NULL) { // tail
    blockToDeletePrev->next = NULL;
  } else { // somewhere in middle
    blockToDeletePrev->next = blockToDelete->next;
  }
}

void dealloc(void *block) {

  // Check if coalescing is possible
  // For that, must get the adjacent block(s)
  blockHeader *blockToFree = (blockHeader *)((char *)block - blockHeaderSize);
  void *endOfBlockToFree = (void *)((char *)blockToFree + blockToFree->size);

  bool blockIsLeftmost = blockToFree == initialBreak;
  bool blockIsRightmost = endOfBlockToFree == currBreak;
  bool blockIsInMiddle = !blockIsLeftmost && !blockIsRightmost;

  blockHeader *rightAdj = NULL;
  blockHeader *rightAdjPrev = NULL;
  blockHeader *leftAdj = NULL;
  blockHeader *leftAdjPrev = NULL;

  if (blockIsLeftmost || blockIsInMiddle) {
    void *target = endOfBlockToFree;
    blockHeader *currBlock = head;
    blockHeader *prevBlock = NULL;
    while (currBlock != NULL) {
      if ((void *)currBlock == target) {
        rightAdj = currBlock;
        rightAdjPrev = prevBlock;
        break;
      }
      prevBlock = currBlock;
      currBlock = currBlock->next;
    }
  }

  if (blockIsRightmost || blockIsInMiddle) {
    blockHeader *currBlock = head;
    blockHeader *prevBlock = NULL;
    while (currBlock != NULL) {
      void *endOfCurrBlock = (void *)((char *)currBlock + currBlock->size);
      if (endOfCurrBlock == (void *)blockToFree) {
        leftAdj = currBlock;
        leftAdjPrev = prevBlock;
        break;
      }
      prevBlock = currBlock;
      currBlock = currBlock->next;
    }
  }

  if (blockIsLeftmost) {
    if (rightAdj != NULL) {
      DeleteBlockFromFreeList(rightAdj, rightAdjPrev);
      blockToFree->next = head;
      head = blockToFree;
      blockToFree->size += rightAdj->size;
    } else {
      blockToFree->next = head;
      head = blockToFree;
    }
  }

  if (blockIsRightmost) {
    if (leftAdj != NULL) {
      DeleteBlockFromFreeList(leftAdj, leftAdjPrev);
      leftAdj->next = head;
      head = leftAdj;
      leftAdj->size += blockToFree->size;
    } else {
      blockToFree->next = head;
      head = blockToFree;
    }
  }

  if (blockIsInMiddle) {
    if (leftAdj != NULL && rightAdj != NULL) {
      DeleteBlockFromFreeList(leftAdj, leftAdjPrev);
      DeleteBlockFromFreeList(rightAdj, rightAdjPrev);
      leftAdj->next = head;
      head = leftAdj;
      leftAdj->size += blockToFree->size + rightAdj->size;
    } else if (leftAdj != NULL && rightAdj == NULL) {
      DeleteBlockFromFreeList(leftAdj, leftAdjPrev);
      leftAdj->next = head;
      head = leftAdj;
      leftAdj->size += blockToFree->size;
    } else if (leftAdj == NULL && rightAdj != NULL) {
      DeleteBlockFromFreeList(rightAdj, rightAdjPrev);
      blockToFree->next = head;
      head = blockToFree;
      blockToFree->size += rightAdj->size;
    } else {
      blockToFree->next = head;
      head = blockToFree;
    }
  }
}

void *alloc(int numBytesRequested) {

  if (firstRun) {
    IncreaseHeapSize();
    firstRun = false;
  }

allocationRoutine:;

  size_t totalNumBytesToAlloc = numBytesRequested + blockHeaderSize;
  switch (currChosenAlg) {
  case FIRST_FIT: {
    blockHeader *currBlock = head;
    blockHeader *prevBlock = NULL;
    while (currBlock != NULL) {
      size_t currBlockTotalSize = currBlock->size;
      if (currBlockTotalSize >= totalNumBytesToAlloc) {
        return PerformAllocation(currBlock, prevBlock, numBytesRequested);
      }
      prevBlock = currBlock;
      currBlock = currBlock->next;
    }
    if (IncreaseHeapSize() == false) {
      return NULL;
    } else {
      goto allocationRoutine;
    }
    break;
  }
  case BEST_FIT: {
    blockHeader *smallestBlock = NULL;
    blockHeader *smallestBlockPrev = NULL;
    size_t smallestBlockTotalSize = 0;
    blockHeader *currBlock = head;
    blockHeader *prevBlock = NULL;
    while (currBlock != NULL) {
      size_t currBlockTotalSize = currBlock->size;
      bool currBlockIsBigEnough = currBlockTotalSize >= totalNumBytesToAlloc;
      if (smallestBlock == NULL && currBlockIsBigEnough) {
        smallestBlock = currBlock;
        smallestBlockPrev = prevBlock;
        smallestBlockTotalSize = currBlockTotalSize;
      } else if (currBlockIsBigEnough &&
                 currBlockTotalSize < smallestBlockTotalSize) {
        smallestBlock = currBlock;
        smallestBlockPrev = prevBlock;
        smallestBlockTotalSize = currBlockTotalSize;
      } else {
        // Either curr block is not big enough or it is larger than smallest
        // one found thus far
      }
      prevBlock = currBlock;
      currBlock = currBlock->next;
    }

    if (smallestBlock == NULL) {
      if (IncreaseHeapSize() == false) {
        return NULL;
      } else {
        goto allocationRoutine;
      }
    } else {
      return PerformAllocation(smallestBlock, smallestBlockPrev,
                               numBytesRequested);
    }

    break;
  }
  case WORST_FIT: {
    blockHeader *largestBlock = NULL;
    blockHeader *largestBlockPrev = NULL;
    size_t largestBlockTotalSize = 0;
    blockHeader *currBlock = head;
    blockHeader *prevBlock = NULL;
    while (currBlock != NULL) {
      size_t currBlockTotalSize = currBlock->size;
      bool currBlockIsBigEnough = currBlockTotalSize >= totalNumBytesToAlloc;
      if (largestBlock == NULL && currBlockIsBigEnough) {
        largestBlock = currBlock;
        largestBlockPrev = prevBlock;
        largestBlockTotalSize = currBlockTotalSize;
      } else if (currBlockIsBigEnough &&
                 currBlockTotalSize > largestBlockTotalSize) {
        largestBlock = currBlock;
        largestBlockPrev = prevBlock;
        largestBlockTotalSize = currBlockTotalSize;
      } else {
        // Either curr block is not big enough or it is smaller than largest
        // one found thus far
      }
      prevBlock = currBlock;
      currBlock = currBlock->next;
    }

    if (largestBlock == NULL) {
      if (IncreaseHeapSize() == false) {
        return NULL;
      } else {
        goto allocationRoutine;
      }
    } else {
      return PerformAllocation(largestBlock, largestBlockPrev,
                               numBytesRequested);
    }

    break;
  }
  }
}

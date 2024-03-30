/** Includes **/
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

/** Get size of a word in machine **/
using word_t = intptr_t;

constexpr size_t MAX_SIZE{4096};         // Define max size of the heap
constexpr size_t M_MMAP_THRESHOLD{1024}; // min size to allocate using mmap

struct Block {
  size_t size;
  bool inuse;

  Block *prev{nullptr};
  Block *next{nullptr};
};

static Block *head{nullptr};      // Heap will be implemented as a linked list
std::vector<Block *> free_list{}; // Free list

size_t memorySize() {
  size_t total{};
  Block *curr{head};

  while (curr != nullptr) {
    total += curr->size;
    curr = curr->next;
  }

  return total;
}

size_t align(size_t size) {
  return (size + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

size_t allocSize(size_t size) { return size + sizeof(Block); }

Block *requestFromOS(size_t size) {
  size = allocSize(size); // Prevent SegFault
  void *block{nullptr};

  if (size > M_MMAP_THRESHOLD) {
    block = mmap(nullptr, size, PROT_WRITE | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(errno == 0 && "mmap failed to allocate memory");
  } else {
    block = sbrk(size);
    assert(errno == 0 && "sbrk failed to allocate memory");
  }

  return static_cast<Block *>(block);
}

Block *findBlock(size_t size) {
  if (free_list.size() == 0)
    return nullptr;
  for (const auto &block : free_list) {
    if (block->size >= size) {
      return block;
    }
  }

  return nullptr;
}

bool canCoalesce(Block *block) {
  return (block->next != nullptr && !block->next->inuse) ||
         (block->prev != nullptr && !block->prev->inuse);
}

void coalesce(Block *block) {
  if (block->next != nullptr && !block->next->inuse) {
    block->size += block->next->size;
    block->next = block->next->next;
  }

  if (block->prev != nullptr && !block->prev->inuse) {
    block->size += block->prev->size;
    block->prev = block->prev->prev;
    block->prev->next = block;
  }
}

bool canSplit(Block *block, size_t size) { return (block->size > size); }

Block *split(Block *block, size_t size) {
  Block *newBlock{block + allocSize(size)}; // get splitting pointer
  newBlock->inuse = false;
  newBlock->size = block->size - size;
  newBlock->next = block->next;

  block->next = newBlock;
  block->size = size;
  newBlock->prev = block;

  return block;
}

void *alloc(size_t size) {
  size = align(size);

  if (memorySize() + size > MAX_SIZE) {
    return nullptr;
  }

  Block *block{findBlock(size)};
  free_list.erase(std::remove(free_list.begin(), free_list.end(), block),
                  free_list.end());

  if (block == nullptr) {
    block = requestFromOS(size);

    if (head == nullptr) {
      head = block;
    } else {
      Block *curr{head};
      while (curr->next != nullptr) // O(n)
      {
        curr = curr->next;
      }

      curr->next = block;
      block->prev = curr;
    }

    block->size = size;
    block->inuse = true;

    return block;
  }

  block->inuse = true;

  if (canSplit(block, size)) {
    block = split(block, size);
  }

  return block;
}

void free(void *ptr) {
  Block *curr{head};

  while (curr != nullptr) {
    if (curr == ptr) {
      curr->inuse = false;

      if (canCoalesce(curr)) {
        coalesce(curr);
      }

      free_list.push_back(curr);

      return;
    }

    curr = curr->next;
  }

  return;
}

void printMemory() {
  Block *curr{head};
  while (curr != nullptr) {
    std::cout << '[' << curr->size << ", " << curr->inuse << "] -> ";
    curr = curr->next;
  }

  std::cout << "nullptr\n";
}

int blocksAvailable() {
  Block *curr{head};
  int n_blocks{};
  while (curr != nullptr) {
    n_blocks++;
    curr = curr->next;
  }

  return n_blocks;
}

void resetHeap() {
  if (head == nullptr)
    return; // already reset

  brk(head);

  head = nullptr;
  return;
}

int main() {
  Block *b1{static_cast<Block *>(alloc(15))}; // [16, 1]
  assert(b1->size == 16);
  assert(b1->inuse);
  free(b1); // [16, 0]
  assert(!b1->inuse);

  Block *b2{static_cast<Block *>(alloc(8))}; // [8, 1] -> [8, 0]
  assert(b2->size == 8);
  assert(blocksAvailable() == 2);
  assert(b1 == b2);

  Block *b3{static_cast<Block *>(alloc(12))}; // [8, 1] -> [8, 0] -> [16, 1]
  free(b3);                                   // [8, 1] -> [24, 0]
  free(b2);                                   // [32, 0]
  assert(b2->size == 32);
  assert(blocksAvailable() == 1);

  Block *b4{static_cast<Block *>(alloc(4097))}; // Out of memory
  assert(b4 == nullptr);

  Block *b5{static_cast<Block *>(alloc(2034))}; // Allocates using mmap
  assert(b5 != nullptr);

  std::cout << "\nAll assertions passed\n\n";

  return 0;
}

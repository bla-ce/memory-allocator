#include <cassert>
#include <cstdint>
#include <iostream>
#include <unistd.h>

using word_t = intptr_t;


struct Block
{
    size_t size;
    bool inuse;

    Block* next { nullptr };
};

static Block* head { nullptr };

size_t align(size_t size)
{
    return (size + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
} 

size_t allocSize(size_t size)
{
    return size + sizeof(Block);
}

Block* requestFromOS(size_t size)
{
    size = allocSize(size); // Prevent SegFault
    Block* block { static_cast<Block*>(sbrk(size)) };

    return block;
}

Block* findBlock(size_t size)
{
    Block* curr { head };

    while(curr != nullptr)
    {
        if(curr->size >= size && !curr->inuse)
        {
            return curr;
        }

        curr = curr->next;
    }

    return nullptr;
}

bool canSplit(Block* block, size_t size)
{
    return (block->size > size);
}

Block* split(Block* block, size_t size)
{
    Block* newBlock { block + allocSize(size) }; //get splitting pointer
    newBlock->inuse = false;
    newBlock->size = block->size - size;
    newBlock->next = block->next;

    block->next = newBlock;
    block->size = size;

    return block;
}

void* alloc(size_t size)
{
    size = align(size);

    Block* block { findBlock(size) };
    
    if(block == nullptr)
    {
        block = requestFromOS(size);

        if(head == nullptr)
        {
            head = block;
        } else {
            Block* curr { head };
            while(curr->next != nullptr) // O(n)
            {
                curr = curr->next;
            }

            curr->next = block;
        }

        block->size = size;
        block->inuse = true;

        return block;
    }

    block->inuse = true;

    if(canSplit(block, size))
    {
        block = split(block, size);
    }

    return block;
}

void free(void* ptr)
{
    Block* curr { head };

    while(curr != nullptr)
    {
        if(curr == ptr)
        {
            curr->inuse = false;
            return;
        }

        curr = curr->next;
    }

    return;
}

void printMemory()
{
    Block* curr { head };
    while(curr != nullptr)
    {
        std::cout << '[' << curr->size << ", " << curr->inuse << "], ";
        curr = curr->next;
    }

    std::cout << "nullptr\n";
}

int main()
{
    /* Test Case 1: Data alignment */
    assert( align(8) == sizeof(word_t) );
    assert( align(4) == sizeof(word_t) );
    assert( align(15) == 16 );
    assert( align(17) == 24 );

    Block* b1 { static_cast<Block*>(alloc(12)) }; 
    assert( b1 != nullptr );
    assert( head != nullptr );

    printMemory();
   
    Block* b2 { static_cast<Block*>(alloc(22)) }; 
    assert( b1->next == b2 );
    assert( b2 != nullptr );

    printMemory();

    Block* b3 { static_cast<Block*>(alloc(10)) };
    printMemory();
    free(b3);
    assert(!b3->inuse);

    printMemory();
    Block* b4 { static_cast<Block*>(alloc(8)) };
    assert(b3 == b4);
    assert(b3->size == 8);
    assert(b4->size == 8);
    assert(b4->next->next == nullptr);

    printMemory();

    free(b1);
    assert(!head->inuse);

    printMemory();
    
    Block* b5 { static_cast<Block*>(alloc(14)) };
    assert(head == b5);
    assert(b1 == b5);

    printMemory();
    std::cout << "All assertions passed!\n\n";

    return 0;
}

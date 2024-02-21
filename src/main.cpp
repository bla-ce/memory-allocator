#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <pstl/glue_algorithm_defs.h>
#include <unistd.h>

using word_t = intptr_t;

constexpr size_t MAX_SIZE { 4096 };

struct Block
{
    size_t size;
    bool inuse;

    Block* next { nullptr };
};

static Block* head { nullptr };

size_t memorySize()
{
    size_t total{};
    Block* curr { head };

    while( curr != nullptr )
    {
        total += curr->size;
        curr = curr->next;
    }

    return total;
}

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
    Block* best { nullptr };
    Block* curr { head };

    while(curr != nullptr)
    {
        if(curr->size >= size && !curr->inuse)
        {
            if( best == nullptr )
            {
                best = curr;
                continue;
            }

            if( curr->size < best->size )
            {
                best = curr;
            }
        }

        curr = curr->next;
    }

    return best;
}

bool canCoalesce(Block* block)
{
    return (block->next != nullptr && !block->next->inuse);
}

void coalesce(Block* block)
{
    block->size += block->next->size;
    block->next = block->next->next;
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

    if(memorySize() + size > MAX_SIZE)
    {
        std::cout << "Out of Memory\n\n";
        return nullptr;
    }

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

            if(canCoalesce(curr))
            {
                coalesce(curr);
            }

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
        std::cout << '[' << curr->size << ", " << curr->inuse << "] -> ";
        curr = curr->next;
    }

    std::cout << "nullptr\n";
}

void freeAll()
{
    Block* curr { head };

    while(curr != nullptr)
    {
        free(curr);
        curr = curr->next;
    }
}

int main()
{
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

    Block* b6 { static_cast<Block*>(alloc(17)) };
    Block* b7 { static_cast<Block*>(alloc(14)) };
    printMemory();
    free(b7);
    free(b6);
    assert(b6->size == 40);
    printMemory();

    Block* b8 { static_cast<Block*>(alloc(14)) };
    printMemory();
    Block* b9 { static_cast<Block*>(alloc(12)) };
    printMemory();

    free(b6);
    free(b4);
    printMemory();
    Block* b10 { static_cast<Block*>(alloc(4)) };
    printMemory();
    assert(b10 == b9->next);

    freeAll();
    printMemory();

    assert( alloc(4097) == nullptr );

    std::cout << "All assertions passed!\n\n";

    return 0;
}

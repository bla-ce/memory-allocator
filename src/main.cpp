#include <cassert>
#include <cstdint>
#include <iostream>

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

void* alloc(size_t size)
{
    size = align(size);

    Block* block { static_cast<Block*>( malloc(size) ) };
    block->inuse = true;
    block->size = size;

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
            break;
        }

        curr = curr->next;
    }
}

void printMemory()
{
    Block* curr { head };
    while(curr != nullptr)
    {
        std::cout << '[' << curr->size << ", " << curr->inuse << "], ";
        curr = curr->next;
    }

    std::cout << '\n';
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
    free(b3);
    assert(!b3->inuse);

    printMemory();

    Block* b4 { static_cast<Block*>(alloc(8)) };
    assert(b3->next == b4);

    printMemory();
 
    std::cout << "All assertions passed!\n\n";

    return 0;
}

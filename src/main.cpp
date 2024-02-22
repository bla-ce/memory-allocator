#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

using word_t = intptr_t;

constexpr size_t MAX_SIZE { 4096 };

struct Block
{
    size_t size;
    bool inuse;

    Block* prev { nullptr };
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
    return (block->next != nullptr && !block->next->inuse) || (block->prev != nullptr && !block->prev->inuse);
}

void coalesce(Block* block)
{
    if(block->next != nullptr && !block->next->inuse)
    {
        block->size += block->next->size;
        block->next = block->next->next;
    }

    if(block->prev != nullptr && !block->prev->inuse)
    {
        block->size += block->prev->size;
        block->prev = block->prev->prev;
        block->prev->next = block;
    }
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
    newBlock->prev = block;

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
            block->prev = curr;
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

void resetHeap()
{
    if(head == nullptr) return; // already reset

    brk(head);

    head = nullptr;
    return;
}

int main()
{
    char choice{};

    do
    {
        std::cout << "\nMemory Allocator:\n";
        std::cout << "\t1. Allocate Memory\n";
        std::cout << "\t2. Free Memory\n";
        std::cout << "\t3. Reset Heap\n";
        std::cout << "\t4. Print Heap\n";
        std::cout << "\t5. Get Heap Size\n";
        std::cout << "\t6. Quit\n\n";

        std::cout << "Enter your choice: ";
        std::cin >> choice;

        if( std::clamp( static_cast<int>(choice - '0'), 1, 5) != choice ) continue; // if choice not in range [1, 5]

        switch (choice) {
            case '1': 
                //allocate memory
            case '2':
                //free memory
            case '3':
                resetHeap();
            case '4':
                printMemory();
            default:
                std::cout << "Memory Size: " << memorySize() << '\n';
        }
    } while(choice != '6');

    return 0;
}

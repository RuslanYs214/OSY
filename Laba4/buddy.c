#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define NUM_LISTS 32

typedef struct BlockHeader {
    size_t size;
    struct BlockHeader* next;
    struct BlockHeader* prev;
    int is_free;
} BlockHeader;

typedef struct Allocator {
    void* memory;
    size_t size;
    BlockHeader* free_lists[NUM_LISTS];
} Allocator;

static int get_free_list_index(size_t size) {
    int index = 0;
    size_t current_size = 1;
    while (current_size < size) {
        current_size <<= 1;
        index++;
        if (index >= NUM_LISTS - 1) {
            return NUM_LISTS - 1;
        }
    }
    return index;
}

static void add_to_free_list(Allocator* allocator, BlockHeader* block) {
    int index = get_free_list_index(block->size);
    block->is_free = 1;
    block->next = allocator->free_lists[index];
    block->prev = NULL;
    if (allocator->free_lists[index] != NULL) {
        allocator->free_lists[index]->prev = block;
    }
    allocator->free_lists[index] = block;
}

static void remove_from_free_list(Allocator* allocator, BlockHeader* block) {
    int index = get_free_list_index(block->size);
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        allocator->free_lists[index] = block->next;
    }
    if (block->next != NULL) {
        block->next->prev = block->prev;
    }
    block->next = block->prev = NULL;
    block->is_free = 0;
}

static BlockHeader* split_block(BlockHeader* block, size_t size) {
    size_t new_size = block->size / 2;
    BlockHeader* buddy = (BlockHeader*)((char*)block + new_size);
    buddy->size = new_size;
    buddy->next = NULL;
    buddy->prev = NULL;
    buddy->is_free = 1;
    block->size = new_size;
    return buddy;
}

static void merge_blocks(Allocator* allocator, BlockHeader* block) {
    size_t block_addr = (size_t)block;
    size_t memory_addr = (size_t)allocator->memory;
    size_t block_offset = block_addr - memory_addr;
    size_t buddy_offset = block_offset ^ block->size;
    BlockHeader* buddy = (BlockHeader*)(memory_addr + buddy_offset);
    if (buddy != NULL && buddy->is_free && buddy->size == block->size) {
        remove_from_free_list(allocator, buddy);
        if (block < buddy) {
            block->size *= 2;
        } else {
            buddy->size *= 2;
            block = buddy;
        }
        merge_blocks(allocator, block);
    } else {
        add_to_free_list(allocator, block);
    }
}

void* allocator_create(void* mem, size_t size) {
    if (mem == NULL || size <= sizeof(Allocator) + sizeof(BlockHeader)) {
        write(STDERR_FILENO, "Error: Insufficient memory for allocator initialization.\n", 58);
        return NULL;
    }
    
    if ((size & (size - 1)) != 0) {
        write(STDERR_FILENO, "Error: Memory size must be a power of two.\n", 43);
        return NULL;
    }

    Allocator* allocator = (Allocator*)mem;
    allocator->memory = (char*)mem + sizeof(Allocator);
    allocator->size = size - sizeof(Allocator);

    for (int i = 0; i < NUM_LISTS; i++) {
        allocator->free_lists[i] = NULL;
    }

    BlockHeader* initial_block = (BlockHeader*)allocator->memory;
    initial_block->size = allocator->size;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    initial_block->is_free = 1;

    add_to_free_list(allocator, initial_block);
    return allocator;
}


void* my_malloc(void* allocator_ptr, size_t size) {
    if (size == 0 || allocator_ptr == NULL) {
        return NULL;
    }
    Allocator* allocator = (Allocator*)allocator_ptr;
    size += sizeof(BlockHeader);
    int index = get_free_list_index(size);
    for (int i = index; i < NUM_LISTS; i++) {
        BlockHeader* block = allocator->free_lists[i];
        if (block == NULL) continue;
        while (block != NULL) {
            remove_from_free_list(allocator, block);
            while (block->size > size) {
                BlockHeader* buddy = split_block(block, size);
                add_to_free_list(allocator, buddy);
            }
            block->is_free = 0;
            return (char*)block + sizeof(BlockHeader);
        }
    }
    return NULL;
}

void my_free(void* allocator_ptr, void* ptr) {
    if (ptr == NULL || allocator_ptr == NULL) {
        return;
    }
    Allocator* allocator = (Allocator*)allocator_ptr;
    BlockHeader* block = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    if (block < (BlockHeader*)allocator->memory || 
        (char*)block > (char*)allocator->memory + allocator->size) {
        write(STDERR_FILENO, "Error: Invalid free request.\n", 29);
        return;
    }
    block->is_free = 1;
    merge_blocks(allocator, block);
}

void allocator_destroy(void* allocator_ptr, size_t size) {
    if (!allocator_ptr) {
        return;
    }
    Allocator* allocator = (Allocator*)allocator_ptr;
    void* full_memory = (char*)allocator->memory - sizeof(Allocator);
    if (munmap(full_memory, size) == -1) {
        const char msg[] = "Error: munmap failed.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(EXIT_FAILURE);
    }
}

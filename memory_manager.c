#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>


#define BLOCK_SIZE sizeof(struct block)
#define PADDED_SIZE(size) ((size + 7) & ~7)

struct block {
    struct block *next;
    int size;
    int in_use;
};

static struct block *head = NULL;
static void *heap_end = NULL;

void *myalloc(int size) {
    if (size <= 0) {
        return NULL;
    }

    // If this is the first call, mmap() to get some space and build the linked list node
    if (head == NULL) {
        void *heap = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if (heap == MAP_FAILED) {
            return NULL;
        }

        head = (struct block*)heap;
        head->next = NULL;
        head->size = 1024 - BLOCK_SIZE;
        head->in_use = 0;
        heap_end = heap + 1024;
    }

    // Find the first block that is not in-use and has enough space
    struct block *curr = head;
    while (curr != NULL) {
        if (!curr->in_use && (curr->size >= PADDED_SIZE(size))) {
            // Mark the block as in-use and return a pointer to the user data
            curr->in_use = 1;
            void *data = (void*)((char*)curr + BLOCK_SIZE);
            return data;
        }
        curr = curr->next;
    }

    // No block found, return NULL
    return NULL;
}

size_t GET_PADDING(size_t size) {
    const size_t padding = 16;  // padding needs to be a multiple of 16 bytes
    size_t remainder = size % padding;
    if (remainder == 0) {
        return 0;
    } else {
        return padding - remainder;
    }
}

void* Find_Space(size_t bytes) {
    bytes = bytes + GET_PADDING(bytes);
    struct block *current = head;
    while (current != NULL) {
        if (current->in_use == 0 && current->size >= bytes) {
            size_t required_space = GET_PADDING(bytes) + sizeof(struct block) + 16;
            if (current->size >= required_space) {
                Split_Space(current, bytes);
            }
            current->in_use = 1;
            return (void*)current + sizeof(struct block);
        }
        current = current->next;
    }
    return NULL;
}

void Split_Space(struct block *current_node, size_t requested_size) {
    size_t remaining_size = current_node->size - GET_PADDING(requested_size) - sizeof(struct block);
    struct block *new_node = (void*)current_node + sizeof(struct block) + GET_PADDING(requested_size);
    new_node->size = remaining_size;
    new_node->in_use = 0;
    new_node->next = current_node->next;
    current_node->size = GET_PADDING(requested_size);
    current_node->next = new_node;
}

void myfree(void *ptr) {
    struct block *current = (struct block*)((char*)ptr - sizeof(struct block));
    current->in_use = 0;
}

void print_data(void)
{
    struct block *b = head;

    if (b == NULL) {
        printf("[empty]\n");
        return;
    }

    while (b != NULL) {
        // Uncomment the following line if you want to see the pointer values
        //printf("[%p:%d,%s]", b, b->size, b->in_use? "used": "free");
        printf("[%d,%s]", b->size, b->in_use? "used": "free");
        if (b->next != NULL) {
            printf(" -> ");
        }

        b = b->next;
    }

    printf("\n");
}

void main(){
    void *p;

    p = myalloc(512);
    print_data();

    myfree(p);
    print_data();
}
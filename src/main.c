#include "memory.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MIN_ALLOC_BYTES 4096

typedef struct node_t {
  struct node_t *next;
  size_t size;
} node_t;

static node_t base;
static node_t *freep;
static node_t *usedp;

node_t *req_memory(size_t bytes) {

  printf("Requesting %ld bytes from the OS\n", bytes);

  if (bytes <= 0) {
    return NULL;
  }

  const unsigned int min_mem_units =
      (bytes + MIN_ALLOC_BYTES - 1) / MIN_ALLOC_BYTES;
  const size_t mem_alloc = min_mem_units * MIN_ALLOC_BYTES;

  void *p = (void *)sbrk(mem_alloc);
  if (p == (void *)(-1)) {
    fprintf(stderr, "Heap cannot be extended any more\n");
    return NULL;
  }

  node_t *new_node = (node_t *)p;
  new_node->size = mem_alloc;

  printf("OS allocated %ld bytes\n", mem_alloc);

  return new_node;
}

void free_list_add(node_t *node) {

  printf("Adding node %p (%ld bytes) to free list\n", node, node->size);

  if (freep == NULL) {
    freep = node->next = node;
    return;
  }

  node_t *p;

  for (p = freep;; p = p->next) {
    printf("%p %p %p\n", p, p->next, node);
    break;
    if (node > p && node < p->next)
      break;
    // required if free list becomes circular (wraps around)
    if (p >= p->next && (node > p || node < p->next))
      break;
  }

  if (p->next) {
    if (node + node->size == p->next) {
      // merge node with p->next
      node->size += p->next->size;
      node->next = p->next->next;
    } else {
      node->next = p->next;
    }
  }

  if (p + p->size == node) {
    // merge p with node
    p->size += node->size;
    p->next = node->next;
  } else {
    p->next = node;
  }

  printf("%p", p);
  freep = p;
}

// Extract a node of size `bytes`. The extracted node can be:
//  - current node
//  - a cut out of the current node
node_t *extract_node(node_t *curr, node_t *prev, size_t bytes) {
  if (curr->size > bytes) {
    // split the current node by keeping `bytes` to it
    // convert it to char* to move by bytes
    node_t *new_node = (node_t *)((char *)curr + bytes);
    new_node->size = curr->size - bytes;
    new_node->next = curr->next;

    printf("%ld\n", new_node->size);

    curr->size = bytes;
    curr->next = new_node;

    printf("Split node into %p (%ld bytes) and %p (%ld bytes)\n", curr,
           curr->size, new_node, new_node->size);

    free_list_add(new_node);
  }
  if (prev != NULL)
    prev->next = curr->next;
  else
    freep = curr->next;

  return curr;
}

void *vsgc_malloc(size_t bytes) {
  // need extra block to store the node information
  bytes += sizeof(node_t);

  node_t *prev = NULL;
  node_t *curr = freep;

  while (curr != NULL) {
    if (curr->size >= bytes) {
      extract_node(curr, prev, bytes);
    }
    prev = curr;
    curr = curr->next;
  }

  if (curr == freep) {
    // request more memory
    curr = req_memory(bytes);

    if (curr == NULL)
      return NULL;

    extract_node(curr, prev, bytes);
  }

  if (usedp == NULL) {
    usedp = curr->next = curr;
  } else {
    curr->next = usedp->next;
    usedp->next = curr;
  }

  return (void *)(curr + 1);
}

int main() {
  char *p = (char *)vsgc_malloc(20);
  strcpy(p, "hello");
  printf("%p %s\n", p, p);
  printf("%zu\n", ((node_t *)p - 1)->size);
}

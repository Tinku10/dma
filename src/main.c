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

static node_t *freep = NULL;
static node_t *usedp = NULL;


size_t aligned(size_t bytes) {
  // round it off by closest multiple of word size not less than bytes
  return (bytes + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t) - 1);
}

node_t *req_memory(size_t bytes) {

  // printf("Requesting %ld bytes from the OS\n", bytes);

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

  // printf("OS allocated %ld bytes\n", mem_alloc);

  return new_node;
}

void free_list_add(node_t *node) {

  // printf("Adding node %p (%ld bytes) to free list\n", node, node->size);

  if (freep == NULL) {
    freep = node;
    return;
  }

  node_t *prev = NULL;
  node_t *curr = freep;

  if (node < curr) {
    if ((char*) node + node->size == (char*)curr) {
      node->size += curr->size;
      node->next = curr->next;
    } else {
      node->next = curr;
    }

    freep = node;
    return;
  }

  for (; curr != NULL; prev = curr, curr = curr->next) {
    if (node > curr)
      break;
  }

  if (curr == NULL) {
    prev->next = node;
    return;
  }

  if (curr->next) {
    if ((char*) node + node->size == (char*) curr->next) {
      // merge node with p->next
      node->size += curr->next->size;
      node->next = curr->next->next;
    } else {
      node->next = curr->next;
    }
  }

  if ((char*) curr + curr->size == (char*) node) {
    // merge p with node
    curr->size += node->size;
    curr->next = node->next;
  } else {
    curr->next = node;
  }
}

void used_list_add(node_t *node) {
  // printf("Adding %p (%zu bytes) to used list\n", node, node->size);
  // update used list pointer
  if (usedp == NULL) {
    node->next = NULL;
    usedp = node;
  } else {
    node->next = usedp->next;
    usedp->next = node;
  }
}

void extract_node(node_t *curr, node_t *prev, size_t bytes) {
  if (curr->size - bytes > sizeof(node_t)) {
    // only split the node if the new_node has enough bytes
    // to hold itself and the data
    // otherwise, extract the large node without splitting
    node_t *new_node = (node_t *)((char *)curr + bytes);
    new_node->size = curr->size - bytes;
    new_node->next = curr->next;

    curr->size = bytes;

    // printf("Split node into %p (%ld bytes) and %p (%ld bytes)\n", curr,
    //        curr->size, new_node, new_node->size);

    if (prev != NULL)
      prev->next = new_node;
    else
      freep = new_node;
  }

  if (curr == freep) {
    freep = freep->next;
  }

  used_list_add(curr);
}

void free_list_view() {
  printf("FREE LIST VIEW===========\n");
  for (node_t *p = freep; p != NULL; p = p->next) {
    printf("(%p - %zu bytes) ", p, p->size);
  }
  printf("\n");
}

void used_list_view() {
  printf("USED LIST VIEW===========\n");
  for (node_t *p = usedp; p != NULL; p = p->next) {
    printf("(%p - %zu bytes) ", p, p->size);
  }
  printf("\n");
}

void *vsgc_malloc(size_t bytes) {
  // need extra block to store the node information
  printf("\nUser requested %zu bytes\n", bytes);

  bytes += sizeof(node_t);

  node_t *prev = NULL;
  node_t *curr = freep;

  while (curr != NULL) {
    if (curr->size >= bytes) {
      extract_node(curr, prev, bytes);
      break;
    }
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL) {
    // request more memory
    curr = req_memory(bytes);

    if (curr == NULL)
      return NULL;

    extract_node(curr, prev, bytes);
  }

  free_list_view();
  used_list_view();

  return (void *)(curr + 1);
}

int main() {
  char *p = (char *)vsgc_malloc(20);
  // strcpy(p, "hello");
  // printf("%p %s\n", p, p);
  // printf("%zu\n", ((node_t *)p - 1)->size);

  char *q = (char *)vsgc_malloc(5);
  // strcpy(q, "hi");
  // printf("%p %s\n", q, q);

  char *a = (char *)vsgc_malloc(1);
  // strcpy(a, "h");
  // printf("%p %s\n", a, a);

  char *b = (char *)vsgc_malloc(16);
  // strcpy(b, "hisomeone");
  // printf("%p %s\n", b, b);
  //
  char *c = (char *)vsgc_malloc(1000);
  char *d = (char *)vsgc_malloc(10);
  char *e = (char *)vsgc_malloc(5000);
  char *f = (char *)vsgc_malloc(2500);
}

/*
Copyright (c) 2011 J. David Lee. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.  */

/*

This implements an ordered doubly-linked list, ordered from smallest to largest
so peeking at the head is min and tail is max in O(1) after insertion.

We add a remove operation that will traverses in order and will remove
based on the death value (matches the death value of an index
entry)

 */



#include "Python.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef PANDAS_INLINE
  #if defined(__GNUC__)
    #define PANDAS_INLINE static __inline__
  #elif defined(_MSC_VER)
    #define PANDAS_INLINE static __inline
  #elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define PANDAS_INLINE static inline
  #else
    #define PANDAS_INLINE
  #endif
#endif

struct pl_node {
  double val;                   // The node's value.
  int death;                    // When this node should die
  struct pl_node *larger_node;  // The next larger node sorted by value.
  struct pl_node *smaller_node; // The next smaller node sorted by value.
};

struct pl_list {
  int len;                  // The length of the list.
  int is_max;               // Flag if we are a max priority list
  struct pl_node *head;     // The head node by index.
  struct pl_node *tail;     // The tail node by index.
};

struct pl_node *pl_new_node(double val, int death) {
  struct pl_node *n = (struct pl_node*)malloc(sizeof(struct pl_node));
  n->val = val;
  n->death = death;
  n->larger_node = 0;
  n->smaller_node = 0;
  return n;
}

struct pl_list *pl_new(int len, int is_max) {
struct pl_list *pl = (struct pl_list*)malloc(sizeof(struct pl_list));
  pl->len = len;
  pl->is_max = is_max;
  pl->head = 0;
  pl->tail = 0;
  return pl;
}

static void pl_insert_after(struct pl_node *n, struct pl_node *n_new) {
  n_new->smaller_node = n;
  n_new->larger_node = n->larger_node;
  n->larger_node = n_new;
  if(n_new->larger_node != 0) {
    n_new->larger_node->smaller_node = n_new;
  }
}


static void pl_insert_init(struct pl_list *pl, double val, int death) {

  struct pl_node *n_new = pl_new_node(val, death);

  // If this is the first node.
  if(pl->tail == 0) {
    pl->head = pl->tail = n_new;
    // printf("first node: val: %f, death: %d\n", val, death);
    return;
  }

  struct pl_node *n = pl->head;

  // New smallest node?
  // Reset head
  if (n_new->val < n->val) {
    // printf("new smallest node: val: %f, death: %d\n", val, death);
    n_new->larger_node = n;
    n->smaller_node = n_new;
    pl->head = n_new;
    return;
  }

  // new tail node
  if (n_new->val > pl->tail->val) {
     pl_insert_after(pl->tail, n_new);
     pl->tail = n_new;
     return;
  }


  // search

  // Find node to insert after.
  while(n->larger_node != 0 && n_new->val > n->larger_node->val) {
    n = n->larger_node;
  }

  // Insert after this node.
  pl_insert_after(n, n_new);

  // we possibly have a new tail
  if (pl->tail == n) {
    pl->tail = n_new;
  }
}

struct pl_node *pl_get_value(struct pl_list *pl) {
  // Retun the min (head) or max (tail) value
  struct pl_node *n;
  if (pl->is_max) {
     n = pl->tail;
  }
  else {
     n = pl->head;
  }
  return n;
}

static int pl_remove(struct pl_list *pl, int *starti, int curval) {
  // Remove the node indicated by index
  // if the death is <= than the index
  // Return the count of removed entries
  struct pl_node *n = pl->head;
  struct pl_node *ln, *sn;
  struct pl_node *tmp;
  int count = 0, death, deathval;

  // printf("  check for remove: curval: %d\n", curval);
  while(n != 0) {

    death = n->death;
    deathval = starti[death];
    // printf("  remove: death: %d, deathval: %d\n", death, deathval);
    if (n->death != -1 & curval >= deathval) {

      // printf("  remove: death: %d, deathval: %d\n", death, deathval);
      ln = n->larger_node;
      sn = n->smaller_node;

      if (ln != 0) {
        // printf("  larger node: val: %f, death: %d\n", ln->val, ln->death);
        ln->smaller_node = sn;
      }
      if (sn != 0)  {
        // printf("  smaller node: val: %f, death: %d\n", sn->val, sn->death);
        sn->larger_node = ln;
      }

      if (pl->head == n) {
        // printf("  head to larger\n");
        pl->head = ln;
      }
      if (pl->tail == n) {
        // printf("  tail to smaller\n");
        pl->tail = sn;
      }
      count++;

      tmp = n->larger_node;
      free(n);
      n = tmp;

     // this optimizes death checking
     // as we don't need to iterate the entire list each time we are checking
     if (count > 5) {
        break;
      }
    }
    else {
      n = n->larger_node;
    }
  }
  return count;
}

static void pl_print(struct pl_list *pl) {
  // debug method for printint nodes in order
  struct pl_node *n = pl->head;
  int i = 0;
  while (n != 0) {
    printf("[%d] val: %f, death: %d\n", i, n->val, n->death);
    n = n->larger_node;
    i++;
  }
}

static void pl_free(struct pl_list *pl) {
  struct pl_node *n = pl->head;
  struct pl_node *tmp;
  while(n != 0) {
    tmp = n->larger_node;
    free(n);
    n = tmp;
  }
  free(pl);
}

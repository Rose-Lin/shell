#ifndef DLIST_H_
#define DLIST_H_

#include "job_node.h"
// An abstract type for sequences
typedef struct dlist_record* dlist;

// return an empty dlist. Make sure to call dlist_free when you're done
// with it.
dlist dlist_new();


job_node* get_head(dlist);

job_node* get_tail(dlist);



// returns the number of elements in the dlist. Runs in constant time.
int dlist_size(dlist l);

// appends a new element to the beginning of the dlist. Runs in constant time.
// void dlist_push(dlist l, int elt);

// removes and returns the first element in the dlist. Runs in constant time.
// precondition: there is at least one element in the dlist.
// int dlist_pop(dlist l);

// returns the first element in the dlist, without changing the dlist.
// Runs in constant time.
// precondition: there is at least one element in the dlist.
// int dlist_peek(dlist l);

// appends a new element to the end of the dlist. Runs in constant time.
void dlist_push_end(dlist l, job_node* n);

// removes and returns the last element in the dlist. Runs in constant time.
// precondition: there is at least one element in the dlist.
void dlist_pop_end(dlist l);

// returns the last element in the dlist, without changing the dlist.
// Runs in constant time.
// precondition: there is at least one element in the dlist.
job_node* dlist_peek_end(dlist l);

// adds a new element into the dlist, after n existing elements.
// Traverses from whichever side of the dlist is closer.
// precondition: the list has at least n elements
void dlist_insert(dlist l, int index, job_node* n);

// retrieves the nth element of the dlist.
// Traverses from whichever side of the dlist is closer.
// precondition: the list has at least (index+1) elements
job_node* dlist_get(dlist l, int index);

// sets the nth element of the dlist to a new value.
// Traverses from whichever side of the dlist is closer.
// precondition: the list has at least (n+1) elements
// postcondition: returns the old value of the element that was set
// int dlist_set(dlist l, int n, int new_elt);

// removes the nth element of the dlist.
// Traverses from whichever side of the dlist is closer.
// precondition: the list has at least (n+1) elements
// postcondition: returns the removed element
void dlist_remove(dlist l, int n);

//remove the jobnode according to pid from the dlist
void dlist_remove_bypid(dlist l, pid_t pid);

//return the job_node found by pid
job_node* dlist_get_bypid(dlist l, pid_t pid);

// frees an dlist. Takes O(size(l)) steps.
void dlist_free(dlist l);


#endif

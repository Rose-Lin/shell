#ifndef DLIST_NODE_H_
#define DLIST_NODE_H_
#include <termios.h>

typedef struct job_node {
  int index;  // the index stored in this node
  int status; // the status of the job: fg/bg/suspended
  pid_t pid;  //pid of the job
  pid_t gpid;  //gpid of the job
  struct termios jmode;
  char* original_input; //input from command line
  struct job_node* next; // pointer to next node
  struct job_node* prev; // pointer to previous node
} job_node;

// create (i.e., malloc) a new node
job_node* new_node(int index, int status, pid_t pid, pid_t gpid, char* original_input, job_node* next, job_node* prev);

// insert a new node after the given one
// Precondition: Supplied node is not NULL.
void insert_after(job_node* n, job_node* new_n);

// insert a new node before the given one
// Precondition: Supplied node is not NULL.
void insert_before(job_node* n, job_node* new_n);

// delete the given node
// Precondition: Supplied node is not NULL.
void delete_node(job_node* n);

// return the jobnode found by pid;
// If not found: return NULL
job_node* get_jobnode_bypid(job_node* head, pid_t pid);

job_node* jobnode_deepcopy(job_node* n);

// return a pointer to the indexth node in the list. If index is
// the length of the list, this returns NULL, but does not error.
// Precondition: the list has at least index number of nodes
job_node* nth_job(job_node* head, int index);

// return a pointer to the nth previous node in the list. (That is,
// this uses `prev` pointers, not `next` pointers.) If index is
// the length of the list, this returns NULL, but does not error.
// Precondition: the list has at least index number of nodes
job_node* nth_job_prev(job_node* tail, int index);

char* get_input(job_node*);

// free an entire linked list. The list might be empty.
void free_joblist(job_node* head);

// create a linked list that stores the same elements as the given array.
// Postcondition: returns the head element
// job_node* from_array(int n, int a[n]);

// fill in the given array with the elements from the list.
// Returns the number of elements filled in.
// Postcondition: if n is returned, the first n elements of the array
// have been copied from the linked list
//int to_array(job_node* head, int n, int a[n]);

// gets the length of a list
int length(job_node* head);

#endif

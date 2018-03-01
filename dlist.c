#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "dlist.h"
#include "job_node.h"

#define FALSE 0
#define TRUE 1

typedef struct dlist_record{
  job_node* head;
  job_node* tail;
  int size;
}dlist_record;

dlist dlist_new(){
  dlist l = (dlist)malloc(sizeof(dlist_record));
  l->head = NULL;
  l->tail = NULL;
  l->size =0;
  return l;
}

int dlist_size(dlist l){
  return l->size;
}

job_node* get_head(dlist dl) {
  return dl->head;
}

job_node* get_tail(dlist dl) {
  return dl->tail;
}


void dlist_push_end(dlist l, job_node* n){
  if (l->tail){
    job_node* old_tail = l->tail;
    old_tail->next = n;
    n->prev = old_tail;
    l->tail = n;
  }else{
    l->head = n;
    l->tail = n;
  }
  l->size++;
}

void dlist_pop_end(dlist l){
  job_node* n = l->tail;
  if(n->prev){
    l->tail =  l->tail->prev;
  }
  l->size --;
  delete_node(n);
}

job_node* dlist_peek_end(dlist l){
  return l->tail;
}

void dlist_insert(dlist l, int index, job_node* n){
  job_node* h = l-> head;
  job_node* t = l-> tail;
  bool cut_in_line = false;
  if(index > (l->size)/2){
    for(int i=l->size; i>index; i--){
      t = t->prev;
      cut_in_line=true;
    }
    insert_after(t,n);
    if(!cut_in_line){
      l->tail = t->next;
    }
  }else{
    for(int i=0; i<index; i++){
      h = h->next;
      cut_in_line =true;
    }
    if(h){
      insert_before(h,n);
      if(!cut_in_line){
        l->head = h->prev;
      }
    }else{
      l->head = n;
      l->tail = l->head;
    }
  }
  l->size++;
}

job_node* dlist_get(dlist l, int index) {
  if(l == NULL) {
    return NULL;
  }
  job_node* t = l->head;
  while(t != NULL) {
    if(t->index == index) {
      return t;
    }
    t = t->next;
  }
  return NULL;
}

void dlist_remove(dlist l, int n){
  if(n <= (l->size)/2){
    delete_node(nth_job(l->head,n));
  }else{
    delete_node(nth_job_prev(l->head,l->size-n-1));
  }
  l->size--;
}

int dlist_remove_bypid(dlist l, pid_t pid) {
  if(l->size == 0) {
    return FALSE;
  }
  job_node* t = l->head;
  while(t != NULL) {
    if(t->pid == pid) {
      job_node* tprev = t->prev;
      job_node* tnext = t->next;
      if(tprev != NULL && tnext != NULL) {
        tprev->next = tnext;
        tnext->prev = tprev;
      } else if (tprev == NULL && tnext == NULL) {
        l->head = NULL;
        l->tail = NULL;
        l->size = 0;
        return TRUE;
      } else if(tprev == NULL) {
        tnext->prev = NULL;
        l->head = tnext;
      } else if (tnext == NULL) {
        tprev->next = NULL;
        l->tail = tprev;
      }
      l->size--;
      free(t->original_input);
      free(t);
      return TRUE;
    }
    t = t->next;
  }
  return FALSE;
}

job_node* dlist_get_bypid(dlist l, pid_t pid){
  job_node* h = l->head;
  return get_jobnode_bypid(h, pid);
}

void dlist_free(dlist l){
  if(l == NULL) {
    return;
  } else if(l->size == 0) {
    free(l);
    return;
  }
  free_joblist(l->head);
  free(l);
}

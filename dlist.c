#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "dlist.h"
#include "job_node.h"

typedef struct dlist_record{
  job_node* head;
  job_node* tail;
  int size;
}dlist_record;

dlist dlist_new(){
  dlist l = malloc(sizeof(dlist_record));
  l->head = NULL;
  l->tail = NULL;
  l->size =0;
  return l;
}

int dlist_size(dlist l){
  return l->size;
}

// void dlist_push(dlist l, int elt){
//   l->head = new_node(elt,l->head,NULL);
//   if(l->head->next){
//     insert_before(l->head,elt);
//   }
//   l->tail = nth_node(l->head,l->size);
//   l->size++;
// }

// int dlist_pop(dlist l){
//   int element = 0;
//   job_node* n = l->head;
//   element = n->data;
//   l->head = n->next;
//   delete_node(n);
//   l->size--;
//   return element;
// }

// int dlist_peek(dlist l){
//   return(l->head->data);
// }

void dlist_push_end(dlist l, job_node* n){
  if (l->tail){
    printf("%s\n", "----");
    insert_after(l->tail, n);
    l->tail = l->tail->next;
  }else{
    printf("%s\n", "----");
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

job_node* dlist_get(dlist l, int index){
  job_node* t = l->tail;
  if(index < (l->size)/2){
    //traverse from head
    return nth_job(l->head, index);
  }else{
    //traverse from tail
    return nth_job_prev(t, l->size-index-1);
  }
}

// int dlist_set(dlist l, int n, int new_elt){
//   int old = 0;
//   if(n <= (l->size)/2){
//     old =nth_node(l->head,n)->data;
//     nth_node(l->head,n)->data= new_elt;
//   }else{
//     old = nth_node(l->tail,n)->data;
//     nth_node(l->tail,n)->data = new_elt;
//   }
//   return old;
// }

void dlist_remove(dlist l, int n){
  int old = 0;
  if(n <= (l->size)/2){
    delete_node(nth_job(l->head,n));
  }else{
    delete_node(nth_job_prev(l->head,l->size-n-1));
  }
  l->size--;
}

void dlist_free(dlist l){
  free_joblist(l->head);
  free(l);
}

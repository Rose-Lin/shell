#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "job_node.h"

job_node* new_node(int index, int status, pid_t pid, pid_t gpid, char* original_input, job_node* next, job_node* prev){
  job_node* n = malloc(sizeof(job_node));
  n->index = index;
  n->status = status;
  n->pid = pid;
  n->gpid = gpid;
  n->original_input = original_input;
  n->next = next;
  n->prev = prev;
  return n;
}

char* get_input(job_node* node) {
  return node->original_input;
}

void insert_after(job_node* n, job_node* new_n){
  if(n->next){
    n->next->prev = new_n;
    new_n->next = n->next;
  }
  n->next = new_n;
  new_n->prev = n;
}

void insert_before(job_node* n, job_node* new_n){
  if(n->prev){
    n->prev->next = new_n;
    new_n->prev = n->prev;
  }
  n->prev = new_n;
  new_n->next = n;
}

void delete_node(job_node* n){
  if(n->prev && n->next){
    n->prev->next = n->next;
    n->next->prev = n->prev;
  }else if(n->prev){
    n->prev->next = NULL;
  }else if(n->next){
    n->next->prev = NULL;
  }
  free(n);
}

job_node* get_jobnode_bypid(job_node* head, pid_t pid){
  job_node* p = head;
  if (p){
    while (p){
      if (p->pid == pid){
        return p;
      }
      p = p->next;
    }
  }
  return NULL;
}

job_node* nth_job(job_node* head, int index){
  job_node* p = head;
  if(index ==length(head)){
    return NULL;
  }else{
    for(int i=0; i<index; p = p->next,i++)
      ;
  }
  return p;
}

job_node* nth_job_prev(job_node* tail, int index){
  int length =0;
  job_node* t = tail;
  for(;t;length++, t = t->prev){
    ;
  }
  t = tail;
  if(index==length){
    return NULL;
  }else{
    for(int i=0; i<index; t = t->prev, i++){
      ;
    }
  }
  return t;
}

void free_joblist(job_node* head){
  job_node* l = head;
  while(l){
    head = l;
    free(l->original_input);
    l = l->next;
    free(head);
  }
}

// job_node* from_array(int n, int a[n]){
//   if(n==0){
//     return NULL;
//   }
//   job_node* head = new_node(a[0],NULL,NULL);
//   job_node* temp = head;
//   for(int i=1;i<n;i++){
//     insert_after(temp,a[i]);
//     temp = temp->next;
//   }
//   return head;
// }

// int to_array(job_node* head, int n, int a[n]){
//   job_node* num = head;
//   int i=0;
//   for(; i<n; num=num->next){
//     if(num){
//       a[i] = num->index;
//       i++;
//     }
//   }
//   return i;
// }

int length(job_node* head){
  job_node* n = head;
  int length = 0;
  while(n){
    length++;
    n = n->next;
  }
  return length;
}

#include <stdlib.h>
#include <stdio.h>

#include "ADTQueue.h"

struct queue {
	List list;	
};

Queue q_create(void (*destroy)(void* a)){
    Queue q = malloc(sizeof(*q));
    q->list = list_create(destroy);

    return q;
}

int q_size(Queue q){ return list_size(q->list); }

void* q_first(Queue q){ return list_node_value(q->list, list_first(q->list)); }

void q_insert(Queue q, void* value){
    int size = q_size(q);

    if(size == 0){
        list_insert_next(q->list, LIST_BOF, value);
    }
    else{
        ListNode before = list_first(q->list);
        for(ListNode node = list_first(q->list); node != LIST_EOF; node=list_next(q->list, node)){    
            before = node;
            if(list_next(q->list, node)==LIST_EOF){
                list_insert_next(q->list, before, value);
                break;
            }
            before = node;
        }
        
    }
}

void q_print(Queue q){
    int counter = 1;
    for(ListNode node = list_first(q->list); node != LIST_EOF; node = list_next(q->list, node)){
        printf("%d: %d \n", counter, *(int*)list_node_value(q->list, node));
        counter++;
    }
}

void q_remove_first(Queue q){
    ListNode node = LIST_BOF;
    list_remove_next(q->list, node);
}

void q_destroy(Queue q){
    list_destroy(q->list);
    free(q);
}
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "ADTList.h"

typedef struct queue* Queue;

Queue q_create(void (*destroy)());

int q_size(Queue q);

void* q_first(Queue q);

void q_insert(Queue q, void* value);

void q_remove_first(Queue q);

void q_destroy(Queue q);

void q_print(Queue q);
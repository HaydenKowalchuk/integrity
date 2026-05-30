#pragma once

#include <integrity/common/common.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../scene/scene.h"

// A structure to represent a stack
struct StackNode {
  struct scene data;
  struct StackNode* next;
};

struct StackNode* newNode(scene data);

bool isEmpty(struct StackNode* root);
void push(struct StackNode** root, scene data);
scene pop(struct StackNode** root);
scene* peek(struct StackNode** root);

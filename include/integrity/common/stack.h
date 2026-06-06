#pragma once

#include <integrity/common/common.h>
#include <integrity/scene/scene.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct StackNode {
  struct IntegrityScene data;
  struct StackNode* next;
};

struct StackNode* newNode(IntegrityScene data);

bool isEmpty(struct StackNode* root);
void push(struct StackNode** root, IntegrityScene data);
IntegrityScene pop(struct StackNode** root);
IntegrityScene* peek(struct StackNode** root);

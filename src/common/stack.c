#include <integrity/common/resource_manager.h>
#include <integrity/common/stack.h>
#include <string.h>

static int skip_init = 0;

IntegrityScene empty = {.flags = INT_MIN};

struct StackNode* newNode(IntegrityScene data) {
  struct StackNode* stackNode = (struct StackNode*)malloc(sizeof(struct StackNode));
  stackNode->data = data;
  stackNode->next = NULL;
  return stackNode;
}

bool isEmpty(struct StackNode* root) {
  return !root;
}

void push(struct StackNode** root, IntegrityScene data) {
  struct StackNode* stackNode = newNode(data);
  stackNode->next = *root;
  *root = stackNode;
  struct IntegrityScene* current = &(*root)->data;

  /* Call Init */
  if (current->init && !skip_init) {
    (current->init)();
  }
}

IntegrityScene pop(struct StackNode** root) {
  if (isEmpty(*root)) {
    return (IntegrityScene){.flags = INT_MIN};
  }

  struct StackNode* temp = *root;
  *root = (*root)->next;
  struct IntegrityScene* popped = &temp->data;
  struct IntegrityScene ret = *popped;

  /* Call Exit */
  if (popped->exit) {
    (popped->exit)();
  }

  skip_init = 0;
  if (temp->data.flags & SCENE_NO_REINIT_CHILD) {
    skip_init = 1;
  }

  free(temp);

  return ret;
}

IntegrityScene* peek(struct StackNode** root) {
  if (isEmpty(*root)) {
    return &empty;
  }
  return &(*root)->data;
}

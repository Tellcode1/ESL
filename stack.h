#ifndef E_STACK_H
#define E_STACK_H

#include "stdafx.h"
#include "var.h"

#include <stddef.h>
#include <stdlib.h>

typedef struct e_stack_frame {
  u32 variable_offset;
  u32 stack_size;
} e_stack_frame;

typedef struct e_var_offset {
  u32 offset_index;
  u32 id; /* Hashed name */
} e_var_offset;

typedef struct e_stack {
  u32    size;
  u32    capacity;
  e_var* stack;

  u32            depth;
  u32            frame_capacity;
  e_stack_frame* frames;

  u32           nvariables;
  u32           variable_capacity;
  e_var_offset* variables;
} e_stack;

static inline int
e_stack_init(u32 capacity, u32 frame_capacity, u32 variable_capacity, e_stack* stack)
{
  capacity        = MAX(capacity, 8);
  stack->capacity = capacity;
  stack->size     = 0;
  stack->stack    = (e_var*)malloc(sizeof(e_var) * capacity);
  if (stack->stack == nullptr) return -1;

  stack->depth          = 0;
  stack->frame_capacity = frame_capacity;
  stack->frames         = (e_stack_frame*)malloc(sizeof(e_stack_frame) * frame_capacity);
  if (stack->frames == nullptr) return -1;

  stack->variable_capacity = variable_capacity;
  stack->nvariables        = 0;
  stack->variables         = (e_var_offset*)malloc(sizeof(e_var_offset) * variable_capacity);

  return 0;
}

static inline void
e_stack_free(e_stack* stack)
{
  for (u32 i = 0; i < stack->size; i++) e_var_release(&stack->stack[i]);
  free(stack->frames);
  free(stack->stack);
  free(stack->variables);
}

static inline int
e_stack_push_frame(e_stack* stack)
{
  if (stack->depth >= stack->frame_capacity) {
    stack->frame_capacity *= 2;
    stack->frames = (e_stack_frame*)realloc(stack->frames, stack->frame_capacity * sizeof(e_stack_frame));
    if (stack->frames == nullptr) return -1;
  }

  stack->frames[stack->depth++] = (e_stack_frame){
    .variable_offset = stack->nvariables,
    .stack_size      = stack->size,
  };

  return 0;
}

static inline int
e_stack_pop_frame(e_stack* stack)
{
  const e_stack_frame* frame = &stack->frames[--stack->depth];

  /* Release the variables we're popping. */
  for (size_t i = frame->stack_size; i < stack->size; i++) e_var_release(&stack->stack[i]);

  stack->size       = frame->stack_size;
  stack->nvariables = frame->variable_offset;

  return 0;
}

static inline int
e_stack_push(e_stack* stack, e_var v)
{
  {
    if (stack->size >= stack->capacity) {
      u32    new_cap   = stack->capacity * 2;
      e_var* new_stack = (e_var*)realloc(stack->stack, new_cap * sizeof(e_var));
      if (new_stack == nullptr) return -1;

      stack->capacity = new_cap;
      stack->stack    = new_stack;
    }

    stack->stack[stack->size] = v;
    stack->size++;
  }
  return 0;
}

static inline void
e_stack_pop(e_stack* stack)
{
  e_var_release(&stack->stack[stack->size - 1]);
  stack->size--;
}

static inline e_var*
e_stack_top(e_stack* stack)
{ return &stack->stack[stack->size - 1]; }

/**
 * Push a variable.
 * All variables are popped on frame pop.
 */
static inline e_var*
e_stack_push_variable(u32 id, e_stack* stack)
{
  if (stack->nvariables >= stack->variable_capacity) {
    u32           new_c         = stack->variable_capacity * 2;
    e_var_offset* new_variables = (e_var_offset*)realloc(stack->variables, new_c * sizeof(e_var_offset));
    if (new_variables == nullptr) exit(-1);

    // memset new region to 0
    memset(new_variables + stack->nvariables, 0, (new_c - stack->nvariables) * sizeof(e_var_offset));

    stack->variables         = new_variables;
    stack->variable_capacity = new_c;
  }

  e_var_offset* var = &stack->variables[stack->nvariables];
  var->offset_index = stack->size;
  var->id           = id;
  stack->nvariables++;

  e_stack_push(stack, (e_var){ .type = E_VARTYPE_VOID });

  return e_stack_top(stack);
}

#endif // E_STACK_H
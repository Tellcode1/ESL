/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef E_STACK_H
#define E_STACK_H

#include "bfunc.h"
#include "perr.h"
#include "pool.h"
#include "stdafx.h"
#include "var.h"

#include <stddef.h>
#include <stdlib.h>

/**
 * Stack frame.
 * Stack will automatically release
 * the variables on frame pop.
 */
typedef struct e_stack_frame {
  u32 variable_offset;
  u32 stack_size;
} e_stack_frame;

/**
 * The position of a variable in the stack.
 *
 * Initializing a variable pushes an entry
 * in to the variables array of the stack.
 *
 * There can be multiple entries with the same
 * ID! This is how we allow overshadowing of variables.
 */
typedef struct e_var_offset {
  u32 id; /* Hashed name */
  u32 offset_index;
  u32 depth;
} e_var_entry;

/**
 * Stack.
 */
typedef struct e_stack {
  u32    size;
  u32    capacity;
  e_var* stack;

  u32            depth; // nframes
  u32            frame_capacity;
  e_stack_frame* frames;

  u32          nvariables;
  u32          variable_capacity;
  e_var_entry* variables;
} e_stack;

static inline int
e_stack_init(u32 capacity, u32 frame_capacity, u32 variable_capacity, e_stack* stack)
{
  capacity        = MAX(capacity, 8);
  stack->capacity = capacity;
  stack->size     = 0;
  stack->stack    = (e_var*)malloc(sizeof(e_var) * capacity);
  if (stack->stack == nullptr) return E_EMALLOC;

  stack->depth          = 0;
  stack->frame_capacity = frame_capacity;
  stack->frames         = (e_stack_frame*)malloc(sizeof(e_stack_frame) * frame_capacity);
  if (stack->frames == nullptr) return E_EMALLOC;

  stack->variable_capacity = variable_capacity;
  stack->nvariables        = 0;
  stack->variables         = (e_var_entry*)malloc(sizeof(e_var_entry) * variable_capacity);
  if (stack->variables == nullptr) return E_EMALLOC;

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
    if (stack->frames == nullptr) return E_EMALLOC;
  }

  stack->frames[stack->depth] = (e_stack_frame){
    .variable_offset = stack->nvariables,
    .stack_size      = stack->size,
  };
  stack->depth++;

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
e_stack_push(e_stack* stack, const e_var* v)
{
  {
    if (stack->size >= stack->capacity) {
      u32    new_cap   = stack->capacity * 2;
      e_var* new_stack = (e_var*)realloc(stack->stack, new_cap * sizeof(e_var));
      if (new_stack == nullptr) return E_EMALLOC;

      stack->capacity = new_cap;
      stack->stack    = new_stack;
    }

    stack->stack[stack->size] = *v;
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
    u32          new_c         = stack->variable_capacity * 2;
    e_var_entry* new_variables = (e_var_entry*)realloc(stack->variables, new_c * sizeof(e_var_entry));
    if (new_variables == nullptr) return nullptr;

    // memset new region to 0
    memset(new_variables + stack->nvariables, 0, (new_c - stack->nvariables) * sizeof(e_var_entry));

    stack->variables         = new_variables;
    stack->variable_capacity = new_c;
  }

  /* Push entry in variables table */
  e_var_entry* var  = &stack->variables[stack->nvariables];
  var->offset_index = stack->size;
  var->depth        = stack->depth;
  var->id           = id;
  stack->nvariables++;

  /* Initialized to VOID */
  e_var void_var = (e_var){ .type = E_VARTYPE_VOID };
  e_stack_push(stack, &void_var);

  return e_stack_top(stack);
}

/**
 * Find a variable. NULL if doesn't exist.
 */

static inline e_var*
e_stack_find(const e_stack* stack, u32 hash)
{
  u32 i = stack->nvariables - 1;
  while (i < stack->nvariables) {
    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
    i--;
  }
  return nullptr;
}

/**
 * Find a variable in the current depth of the stack.
 * NULL if not in stack.
 */
static inline e_var*
e_stack_find_in_current_scope(const e_stack* stack, u32 hash)
{
  u32 i     = stack->nvariables - 1;
  u32 depth = stack->depth;
  while (i < stack->nvariables) {
    if (stack->variables[i].depth != depth) break;

    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
    i--;
  }
  return nullptr;
}

#endif // E_STACK_H
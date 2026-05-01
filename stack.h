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

#include <stdio.h>
#include <string.h>
#define DEBUG_PRINT_STACK 1

#include "perr.h"
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

  u32 max_usage;
} e_stack;

static inline int
e_stack_init(u32 capacity, u32 frame_capacity, u32 variable_capacity, e_stack* stack)
{
  const u32 stack_alignment = 16;

  capacity        = MAX(capacity, 8);
  stack->capacity = capacity;
  stack->size     = 0;
  stack->stack    = (e_var*)e_aligned_malloc(sizeof(e_var) * capacity, stack_alignment);
  if (stack->stack == nullptr) return E_EMALLOC;

  stack->depth          = 0;
  stack->frame_capacity = frame_capacity;
  stack->frames         = (e_stack_frame*)calloc(frame_capacity, sizeof(e_stack_frame));
  if (stack->frames == nullptr) return E_EMALLOC;

  stack->variable_capacity = variable_capacity;
  stack->nvariables        = 0;
  stack->variables         = (e_var_entry*)calloc(variable_capacity, sizeof(e_var_entry));
  if (stack->variables == nullptr) return E_EMALLOC;

  stack->max_usage = 0;

  return 0;
}

static inline void
e_stack_free(e_stack* stack)
{
  for (u32 i = 0; i < stack->size; i++) e_var_release(&stack->stack[i]);
  e_xfree((void**)&stack->frames);
  e_xfree((void**)&stack->variables);
  e_aligned_free(stack->stack);
  memset(stack, 0, sizeof *stack);
}

static inline int
e_stack_push_frame(e_stack* stack)
{
  if (stack->depth >= stack->frame_capacity) {
    u32            frames_capacity = stack->frame_capacity * 2;
    e_stack_frame* frames          = (e_stack_frame*)realloc(stack->frames, frames_capacity * sizeof(e_stack_frame));
    if (frames == nullptr) return E_EMALLOC;

    stack->frames         = frames;
    stack->frame_capacity = frames_capacity;
  }

  stack->frames[stack->depth] = (e_stack_frame){
    .variable_offset = stack->nvariables,
    .stack_size      = stack->size,
  };
  // printf("Pushed frame [depth=%u]\n", stack->depth);
  stack->depth++;

  return 0;
}

static inline void
e_stack_pop_frame(e_stack* stack)
{
  // printf("Popped frame [depth=%u]\n", stack->depth - 1);
  const e_stack_frame* frame = &stack->frames[--stack->depth];

  /* Release the variables we're popping. */
  for (size_t i = frame->stack_size; i < stack->size; i++) { e_var_release(&stack->stack[i]); }

  stack->size       = frame->stack_size;
  stack->nvariables = frame->variable_offset;
}

static inline int
e_stack_push(e_stack* stack, const e_var* v)
{
  const u32 alignment = 16;
  if (stack->size >= stack->capacity) {
    u32    new_cap   = stack->capacity * 2;
    e_var* new_stack = (e_var*)e_aligned_realloc(stack->stack, stack->capacity * sizeof(e_var), new_cap * sizeof(e_var), alignment);
    if (new_stack == nullptr) return E_EMALLOC;

    stack->capacity = new_cap;
    stack->stack    = new_stack;
  }

  // printf("Pushed: ");
  // eb_println((e_var*)v, 1);

  e_var* slot = &stack->stack[stack->size];
  e_var_shallow_cpy(v, slot);
  e_var_acquire(slot);

  stack->size++;
  if (stack->size > stack->max_usage) stack->max_usage = stack->size;

  return 0;
}

static inline void
e_stack_pop(e_stack* stack)
{
  e_var* top = &stack->stack[stack->size - 1];
  // printf("Popped: ");
  // eb_println((e_var*)top, 1);
  e_var_release(top);

  *top = E_NULLVAR;
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

    // set new variables to null
    for (u32 i = stack->nvariables; i < new_c; i++) {}
    memset(new_variables + stack->nvariables, 0, (new_c - stack->nvariables) * sizeof(e_var_entry));

    stack->variables         = new_variables;
    stack->variable_capacity = new_c;
  }

  // fprintf(stderr, "Variable with ID %u pushed to stack top %u\n", id, stack->size);

  /* Push entry in variables table */
  e_var_entry* var  = &stack->variables[stack->nvariables];
  var->offset_index = stack->size;
  var->depth        = stack->depth;
  var->id           = id;
  stack->nvariables++;

  /* Initialized to VOID */
  e_var void_var = (e_var){ .type = E_VARTYPE_NULL };
  e_stack_push(stack, &void_var);

  return e_stack_top(stack);
}

/**
 * Find a variable. NULL if doesn't exist.
 */

static inline e_var*
e_stack_find(const e_stack* stack, u32 hash)
{
  // Old ver broke for stack->nvariables = 0
  for (u32 i = stack->nvariables; i-- > 0;) {
    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
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
  u32 depth = stack->depth;
  // Old ver broke for stack->nvariables = 0
  for (u32 i = stack->nvariables; i-- > 0;) {
    if (stack->variables[i].depth != depth) break;

    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
  }
  return nullptr;
}

#endif // E_STACK_H
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

#ifndef E_BYTECODE_H
#define E_BYTECODE_H

/**
 * TODO: Revise the entire documentation.
 * All of it is severly outdated!
 */

#include "stdafx.h"

typedef u16 e_ins;

typedef enum e_opcode_bck {
  /**
   * NOOP. Does nothing. Used for alignment sometimes.
   * Since the size of an instruction is 2 bytes, instructions can really be only aligned with
   * 2 byte offsets (or through other methods.)
   * Usage(noattr): NOOP
   */
  E_OPCODE_NOOP,

  /**
   * Operate two (variables from the stack) and push result to the stack (or optionally to a variable);
   * If the COMPOUND attr is added, first operand is the destination, where the result of [2] and [3] will be stored.
   * If the MULTIPLE attr is used, the first operand is unconditionally the number of arguments, followed by the usual arguments.
   * Usage(noattr): ADD/SUB/MUL/DIV/EXP, Stack contains operands
   * Usage(MULTIPLE): ADD/SUB/MUL/DIV/EXP [NumArgs], Stack contains operands
   * Usage(COMPOUND|MULTIPLE): ADD/SUB/MUL/DIV/EXP [NumArgs] [LeftID], Stack contains right operand
   * Usage(VARIABLE): ADD/SUB/MUL/DIV/EXP [LeftID] [RightID], Result is pushed to the stack
   * Usage(VARIABLE|MULTIPLE): ADD/SUB/MUL/DIV/EXP [NumArgs] [LeftID] [RightID1] [RightID2] [RightID3] ..., Result is pushed to the stack
   * Usage(VARIABLE|COMPOUND|MULTIPLE): ADD/SUB/MUL/DIV/EXP [NumArgs] [LeftID] [RightID1] [RightID2] [RightID3] ..., Result is stored to [Left].
   */
  E_OPCODE_ADD,
  E_OPCODE_SUB,
  E_OPCODE_MUL,
  E_OPCODE_DIV,
  E_OPCODE_MOD,
  E_OPCODE_EXP,
  E_OPCODE_AND,  // Boolean AND
  E_OPCODE_OR,   // Boolean OR
  E_OPCODE_NOT,  // Boolean NOT
  E_OPCODE_BAND, // Bitwise AND
  E_OPCODE_BOR,  // Bitwise OR
  E_OPCODE_XOR,  // Bitwise XOR (no boolean equivalent!)
  E_OPCODE_BNOT, // Bitwise NOT
  E_OPCODE_EQL,  // Equality
  E_OPCODE_NEQ,  // Inequality
  E_OPCODE_LT,
  E_OPCODE_LTE,
  E_OPCODE_GT,
  E_OPCODE_GTE,
  E_OPCODE_NEG, // Negate (-x)

  /**
   * Increment or Decrement the top of the stack (or optionally a variable).
   * Usage(noattr): INC/DEC , Stack contains operand
   * Usage(VARIABLE): INC/DEC [var ID:u32]
   */
  E_OPCODE_INC,
  E_OPCODE_DEC,

  /**
   * Call a function.
   * arguments will be popped from the stack on function return.
   * Arguments are compiled in reverse order, so they can be popped linearly.
   * (Compilation is: for (i64 i = nargs - 1; i >= 0; i--) compile(args[i]), popping in VM is for (u32 i = 0; i < nargs; i++) push(args[i]))
   * If the INLINE attr is used, the function will not push a stack frame.
   * Builtin methods never push a stack frame, so the inline attr is implied and not necessary.
   * If the COMPOUND and VARIABLE attr is used, the functions return value is assigned to the variable ID in 3rd operand.
   * Usage(noattr/INLINE): CALL [nargs:u16] [function ID:u32]
   */
  E_OPCODE_CALL,

  /**
   * Load the argument list provided through the command line, empty list if not
   * executed from the command line or no arguments are present.
   */
  E_OPCODE_LOAD_ARG_LIST,

  /**
   * Return to the caller (with a value optionally).
   * Usage(noattr): RET [has_value], has_value is 1 byte boolean.
   * If has_value, the top of the stack is returned to the caller.
   * Else, void is returned.
   */
  E_OPCODE_RETURN,

  /**
   * Push a literal from the literal table with the index specified as the first operand.
   * Usage(noattr): LOADCONST [idx : e_operand]
   */
  E_OPCODE_LITERAL,

  /**
   * Load a variable from the map using the 1st operand as the ID.
   * If the MEMBER_LOAD attr is used, the 1st operand is the member index, and the 2nd operand is the variable ID.
   * If the MULTIPLE attr is used, the 1st operand is the number of variables to load, in the order that they are specified.
   * !!! The MULTIPLE attr cannot be used with the MEMBER_LOAD attr.
   * Usage (noattr): LOAD [var ID]
   * Usage (MEMBER_ACCESS attr): LOAD [member index] [struct variable ID]
   * Usage (MULTIPLE attr): LOAD [NumArgs] [ID1] [ID2] [ID3]...
   */
  E_OPCODE_LOAD,

  /**
   * Pop the top of the stack.
   */
  E_OPCODE_POP,

  /**
   * Duplicate the top of the stack.
   * If object is refcounted, produce a shallow copy.
   */
  E_OPCODE_DUP,

  /**
   * Load a reference to the variable using the 1st operand as the ID.
   * If MEMBER_ACCESS attr is used, the 1st operand is the member index, and the 2nd operand is the variable ID.
   * Usage(noattr): LOAD_REFERENCE [var ID]
   * Usage(MEMBER_ACCESS): LOAD_REFERENCE [member index] [struct var ID]
   */
  // E_OPCODE_LOAD_REFERENCE,
  // I'm removing references.
  // Stop me if you can.

  /**
   * Assign the top of the stack to the variable on the 1st operand.
   * The assigned value will be left on the stack.
   * If the MEMBER_ACCESS attr is used, the 1st operand will be the member index, and the 2nd operand is the variable ID of the struct.
   * If the CLEAN attr is used, assigned value will be popped off the stack.
   * Usage(noattr/CLEAN): ASSIGN [var ID]
   * Usage(MEMBER_ACCESS): ASSIGN [member index] [struct variable ID]
   */
  E_OPCODE_ASSIGN,

  /**
   * Assign to a member of a struct/container.
   * The base struct/container and index will be popped.
   * assigned value is kept.
   * Usage(noattr): INDEX_ASSIGN [var ID : u32], Stack is Top=Value, Top-1=Index, Top-2=Base struct/container
   */
  E_OPCODE_INDEX_ASSIGN,

  /**
   * Assign to variable on the stack.
   * Used for direct variable assignments
   * (vectors, generally.)
   * INDEX_ASSIGN_VAR [varID : u32]
   */
  E_OPCODE_INDEX_ASSIGN_VAR,

  /**
   * Push a variable to the stack, and set its ID on the variable table.
   * The initial value of the variable will be NULL.
   * If an explicit type for the variable is defined, but no initializer was specified, the initial value of the variable will be zeroed.
   * If the COMPOUND attr is used, the top of the stack is assigned to the variable.
   * If the VARIABLE attr is used, the value of the variable ID operand is SHALLOW copied.
   * If the MULTIPLE attr is used, 1st operand specifies the number of variables to push. Each variable will be initialized to VOID.
   * If both the MULTIPLE and COMPOUND attres are used, the values are pushed off of the stack and stored to the variable in reverse order (First
   * variable gets top, second gets top - 1, etc).
   * !!! The MULTIPLE attr cannot be used with the VARIABLE attr.
   */
  E_OPCODE_INIT,

  /**
   * Pack elements into a single list.
   * The list is pushed to the stack.
   * nelems is allowed to be 0.
   * If the CLEAN attr is used, the elements are popped from the stack on copy.
   * Usage(noattr): MK_LIST [num_elems:u32]
   */
  E_OPCODE_MK_LIST,

  /**
   * Pack key value pairs into a map.
   * The map is pushed to the stack.
   * npairs is allowed to be 0.
   * All key value pairs are released from the stack.
   * Key/Value pairs are pushed in forward order:
   * for (pair) {
   *   compile(pair.key)
   *   compile(pair.value)
   * }
   * And is popped in reverse order (value first, key next).
   * Order of KV pairs does not matter in maps, as they
   * do not have any particular order.
   * Usage(noattr): MK_MAP [npairs]
   */
  E_OPCODE_MK_MAP,

  /**
   * Index into an list, string or structure.
   * The returned value is not an lvalue, you can not modify it and expect
   * modifications to propogate across all copies.
   * If index is out of range, program HALTs with E_EOUTOFRANGE.
   * Base (structure/string/list) will NOT be popped off the stack.
   * Usage(noattr): INDEX
   * Stack is: Top=Index, Top-1=Base
   */
  E_OPCODE_INDEX,

  /**
   * A label. Removed from the instruction stream on optimization levels >= 2
   * because it requires walking through the entire bytestream for each label,
   * modifying each jump to point to the correct indices.
   * Should serve as NOOP.
   * One operand is used, as the label ID. All jumps to this label must have the
   * same label ID.
   * Usage(noattr): LABEL [labelID : u32]
   */
  E_OPCODE_LABEL,

  /**
   * Jump to the specified label.
   * Usage(noattr): JMP [labelID : u32]
   */
  E_OPCODE_JMP,

  /**
   * Jump if equal
   * Usage(noattr): JE [labelID : u32], Stack is [top], [top - 1]
   */
  E_OPCODE_JE,

  /**
   * Jump if not equal
   * Usage(noattr): JE [labelID : u32], Stack is [top], [top - 1]
   */
  E_OPCODE_JNE,

  /**
   * Jump if zero.
   * Usage(noattr): JZ [labelID : u32]
   */
  E_OPCODE_JZ,

  /**
   * Jump if not zero.
   * Usage(noattr): JNZ [labelID : u32]
   */
  E_OPCODE_JNZ,

  /**
   * Push the variable state to a custom location on the VM.
   * The state can be restored by calling E_OPCODE_POP_VARIABLES
   * No operands are used.
   * Usage(noattr): PUSH_VARIABLES/POP_VARIABLES
   */
  E_OPCODE_PUSH_VARIABLES,
  E_OPCODE_POP_VARIABLES,

  /**
   * Make a struct with a set number of members
   * MK_STRUCT [nMembers : u32]
   */
  E_OPCODE_MK_STRUCT,

  /**
   * Access a member by name from a variable.
   * Usage(noattr) MEMBER_ACCESS [MemberNameHashed : u32], Top = Namespace/Struct
   */
  E_OPCODE_MEMBER_ACCESS,

  /**
   * Assign to a member of a struct by name.
   * * Usage(noattr) MEMBER_ACCESS [MemberNameHashed : u32], Top=Value, Top-1 = Namespace/Struct
   */
  E_OPCODE_MEMBER_ASSIGN,

  /**
   * Exit the program with the code specified in 1st operand.
   * Usage: HALT [exit code : u32]
   */
  E_OPCODE_HALT,

  /* E_OPCODE num */
  E_OPCODE_COUNT,
} e_opcode_bck;
typedef u8 e_opcode;

static const u8 __e_opcode_must_have_less_than_256_entries__[E_OPCODE_COUNT <= 256 ? 1 : -1] = { 0 };

typedef struct e_ins_packed {
  u8 opcode;
  union {
    u32  init;
    u32  load;
    u32  halt;
    u32  jmp, jz, jnz, je, jne;
    u32  assign;
    u32  mk_list, mk_map;
    u32  index_assign;
    u32  label;
    u16  literal;
    bool has_return_value;
    struct {
      u16 nargs;
      u32 function_id;
    } call;
  } v;
} e_ins_packed;

#endif // E_IR_H

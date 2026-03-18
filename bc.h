#ifndef E_BYTECODE_H
#define E_BYTECODE_H

#include "stdafx.h"

typedef u16 e_operand;
typedef u32 e_varID;
typedef u32 e_fnID;    // Resolved at compile time.
typedef u32 e_labelID; // Resolved at compile time.

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
  E_OPCODE_EQL,
  E_OPCODE_NEQ,
  E_OPCODE_LT,
  E_OPCODE_LTE,
  E_OPCODE_GT,
  E_OPCODE_GTE,

  /**
   * Increment or Decrement the top of the stack (or optionally a variable).
   * Usage(noattr): INC/DEC , Stack contains operand
   * Usage(VARIABLE): INC/DEC [var id]
   */
  E_OPCODE_INC,
  E_OPCODE_DEC,

  /**
   * Call a function.
   * arguments will be popped from the stack on function return.
   * If the INLINE attr is used, the function will not push a stack frame.
   * Builtin methods never push a stack frame, so the inline attr is implied and not necessary.
   * If the COMPOUND and VARIABLE attr is used, the functions return value is assigned to the variable ID in 3rd operand.
   * Usage(noattr/INLINE): CALL [function ID] [nargs]
   * Usage(COMPOUND|VARIABLE): CALL [function ID] [nargs] [var ID], Result is stored to var ID
   * Where, function ID = u32, nargs = u16, [var ID = u32]
   */
  E_OPCODE_CALL,

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
   * The assigned value will be left on the stack (can be disabled using CLEAN attr)
   * If the MEMBER_ACCESS attr is used, the 1st operand will be the member index, and the 2nd operand is the variable ID of the struct.
   * If the CLEAN attr is used, assigned value will be popped off the stack.
   * Usage(noattr/CLEAN): ASSIGN [var ID]
   * Usage(MEMBER_ACCESS): ASSIGN [member index] [struct variable ID]
   */
  E_OPCODE_ASSIGN,

  /**
   * Push a variable to the stack, and set its ID on the variable table.
   * The initial value of the variable will be VOID.
   * If an explicit type for the variable is defined, but no initializer was specified, the initial value of the variable will be zeroed.
   * If the COMPOUND attr is used, the top of the stack is assigned to the variable.
   * If the VARIABLE attr is used, the value of the variable ID operand is SHALLOW copied.
   * If the MULTIPLE attr is used, 1st operand specifies the number of variables to push. Each variable will be initialized to VOID.
   * If both the MULTIPLE and COMPOUND attres are used, the values are pushed off of the stack and stored to the variable in reverse order (First variable gets top, second
   * gets top - 1, etc).
   * !!! The MULTIPLE attr cannot be used with the VARIABLE attr.
   */
  E_OPCODE_INIT,

  /**
   * A label. Removed from the instruction stream on optimization levels >= 2.
   * Requires walking through the entire bytestream for each label.
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
   */
  E_OPCODE_PUSH_VARIABLES,
  E_OPCODE_POP_VARIABLES,

  /**
   * Exit the program with the code specified in 1st operand.
   * Usage: HALT [exit code]
   */
  E_OPCODE_HALT,

  /* E_OPCODE num */
  E_OPCODE_COUNT,
} e_opcode_bck;
typedef u8 e_opcode;

typedef enum e_attr_bits {
  E_ATTR_NONE          = 0,
  E_ATTR_COMPOUND      = 1 << 0,
  E_ATTR_MEMBER_ACCESS = 1 << 1,
  E_ATTR_INLINE        = 1 << 2,
  E_ATTR_VARIABLE      = 1 << 3,
  E_ATTR_MULTIPLE      = 1 << 4,
  E_ATTR_CLEAN         = 1 << 5,
} e_attr_bits;
typedef u8 e_attr;

static inline e_ins
e_pack_ins(e_opcode opcode, e_attr tags)
{
  const int bits = sizeof(e_opcode) * 8;
  return (e_ins)opcode | ((e_ins)tags << bits);
}

static inline u64
e_pack_label_ins(u32 target)
{
  const int bits = sizeof(e_opcode) * 8;
  return E_OPCODE_LABEL | ((u64)target << bits);
}

static inline void
e_unpack_ins(e_ins ins, e_opcode* opcode, e_attr* tags)
{
  const int bits = sizeof(e_opcode) * 8;

  *opcode = (e_opcode)(ins & 0xFF);
  *tags   = (e_attr)((ins >> bits) & 0xFF);
}

#endif // E_IR_H

#ifndef E_PROGRAM_ERROR_H
#define E_PROGRAM_ERROR_H

typedef enum e_ecode {
  E_OK           = 0,
  E_EMALLOC      = -1,
  E_EMALFORM     = -2, // Malformed input
  E_EILLINS      = -3, // Illegal instruction
  E_EOUTOFRANGE  = -4, // JMP to hell / Structure index too big
  E_ENONEXISTENT = -5, // Non existent variable / structure.
  E_EUNKNOWN     = -5,
  E_EPANIC       = -6,
} e_ecode;

#endif // E_PROGRAM_ERROR_H

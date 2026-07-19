#ifndef KIT_COMPILER_ASSEMBLY_DUMP_H
#define KIT_COMPILER_ASSEMBLY_DUMP_H

#include "../../inc/kit.cc.h"
#include "../../inc/kit.ir.h"

#include <stdio.h>

void kit_dump_asm(const kit_compilation_result* r);
void kit_print_instruction(kit_ins i, const kit_compilation_result* r, FILE* f);
void kit_print_instruction_stream(const kit_compilation_result* r, const kit_ins* ins, u32 nins, int indent, FILE* f);

#endif // KIT_COMPILER_ASSEMBLY_DUMP_H
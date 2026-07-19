#ifndef KIT_COMPILER_TABLES_H
#define KIT_COMPILER_TABLES_H

#include "../../inc/kit.cc.h"

RETURNS_ERRCODE int append_defer_entry(kit_compiler* cc, int* exprs, u32 nexprs);
RETURNS_ERRCODE int append_function_entry(kit_arena* a, kitc_function_table* funcs, const kitc_function* func);
RETURNS_ERRCODE int append_struct_decleration(kit_arena* a, const char* name, kitc_struct_information* deposit);
RETURNS_ERRCODE int append_literal_variable(kitc_literal_table* literals, const kit_var* v);
RETURNS_ERRCODE int append_struct_info(kit_compiler* cc, const kitc_struct_information* data);
RETURNS_ERRCODE int add_literal_to_track(kit_compiler* cc, const kit_var* v);

RETURNS_ERRCODE int ns_push(kit_compiler* cc, const char* name);
void                ns_pop(kit_compiler* cc);

#endif // KIT_COMPILER_TABLES_H
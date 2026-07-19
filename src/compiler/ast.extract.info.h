#ifndef KIT_COMPILER_AST_INFORMATION_H
#define KIT_COMPILER_AST_INFORMATION_H

#include "../../inc/kit.cc.h"
#include "../../inc/kit.stdafx.h"

bool                is_literal_value(const struct kit_ast* ast, int node);
RETURNS_ERRCODE int convert_node_to_literal(kit_compiler* cc, int node, kit_var* o);

#endif
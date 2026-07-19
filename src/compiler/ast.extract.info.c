#include "ast.extract.info.h"

#include "../../inc/kit.arena.h"
#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.var.h"

static inline RETURNS_ERRCODE int
make_string_variable(kit_arena* a, char* s, kit_var* v) // s will be onwed by variable after this
{
  kit_refdobj* obj = kit_arnalloc(a, sizeof(kit_refdobj));
  if (!obj) return -1;

  obj->refc                 = 1;
  KIT_OBJ_AS_STRING(obj)->s = s;

  *v = (kit_var){ .type = KIT_VARTYPE_STRING, .val.s = obj };
  return 0;
}

bool
is_literal_value(const kit_ast* ast, int node)
{
  kit_ast_node_type type = KIT_GET_NODE(ast, node)->type;

  // if (type == KIT_AST_NODE_CALL) {
  //   const char* function_name = KIT_GET_NODE(ast, node)->call.function_name;
  //   const int*  args          = KIT_GET_NODE(ast, node)->call.args;
  //   u32         nargs         = KIT_GET_NODE(ast, node)->call.nargs;

  //   if (strcmp(function_name, "vec2") == 0 || strcmp(function_name, "vec3") == 0 || strcmp(function_name, "vec4") == 0) {
  //     bool constant_vector = true;
  //     for (u32 i = 0; i < nargs; i++) {
  //       if (!is_literal_value(ast, args[i])) {
  //         constant_vector = false;
  //         break;
  //       }
  //     }

  //     return constant_vector;
  //   }
  // }
  return type == KIT_AST_NODE_INT || type == KIT_AST_NODE_CHAR || type == KIT_AST_NODE_BOOL || type == KIT_AST_NODE_STRING
      || type == KIT_AST_NODE_FLOAT;
}

RETURNS_ERRCODE int
convert_node_to_literal(kit_compiler* cc, int node, kit_var* o)
{
  kit_ast* p = cc->ast;
  switch (KIT_GET_NODE(p, node)->type) {
    case KIT_AST_NODE_INT: *o = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_GET_NODE(p, node)->i.i }; return 0;
    case KIT_AST_NODE_FLOAT: *o = (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = KIT_GET_NODE(p, node)->f.f }; return 0;
    case KIT_AST_NODE_CHAR: *o = (kit_var){ .type = KIT_VARTYPE_CHAR, .val.c = KIT_GET_NODE(p, node)->c.c }; return 0;
    case KIT_AST_NODE_BOOL: *o = (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = KIT_GET_NODE(p, node)->b.b }; return 0;
    case KIT_AST_NODE_STRING: return make_string_variable(cc->arena, kit_arnstrdup(cc->arena, KIT_GET_NODE(p, node)->s.s), o);

    case KIT_AST_NODE_CALL: {
      // const char* function_name = KIT_GET_NODE(cc->ast, node)->call.function_name;
      // const int*  args          = KIT_GET_NODE(cc->ast, node)->call.args;
      // u32         nargs         = KIT_GET_NODE(cc->ast, node)->call.nargs;

      // if (strcmp(function_name, "vec2") == 0 || strcmp(function_name, "vec3") == 0 || strcmp(function_name, "vec4") == 0) {
      //   return fold_vector(cc, args, nargs);
      // }
    }

    case KIT_AST_NODE_LIST:
    case KIT_AST_NODE_MAP: /* TODO: Implement */ abort(); break;

    default: return 1;
  }
  return 1;
}
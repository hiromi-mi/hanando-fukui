// analysis.c
/*
Copyright 2020 hiromi-mi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "main.h"
#include "selfhost.h"
#include <string.h>

extern Env *env;
extern int lang;
extern Map *typedb;
extern Map *struct_typedb;
extern Map *current_local_typedb;
extern int neg_float_generate;
extern int neg_double_generate;

void update_rsp_offset(int size) {
   env->rsp_offset += size;
   if (*env->rsp_offset_max < env->rsp_offset) {
      *env->rsp_offset_max = env->rsp_offset;
   }
}


Node *analyzing(Node *node) {
   int j;
   Env *prev_env = env;
   if (node->ty == ND_BLOCK || node->ty == ND_FDEF) {
      env = node->env;
      LocalVariable *local_variable = NULL;
      // generate all size of nodes
      // preview all variables and setup offsets.
      if (prev_env) {
         // This is to update rsp_offset based on (it's based system).
         // Previously, this is called by new_env() but no longer supported.
         env->rsp_offset += prev_env->rsp_offset;
      }
      for (j = 0; j < env->idents->keys->len; j++) {
         local_variable = (LocalVariable *)env->idents->vals->data[j];
         if (local_variable->lvar_offset != 0) {
            // For Example: STRUCT Passed Argumenets (use rsp+24 or so on)
            continue;
         }
         int size = cnt_size(local_variable->type);
         // should aligned as x86_64
         if (size % 8 != 0) {
            size += (8 - size % 8);
         }
         update_rsp_offset(size);
         local_variable->lvar_offset = env->rsp_offset;
      }
   }

   if (node->lhs) {
      node->lhs = analyzing(node->lhs);
   }
   if (node->rhs) {
      node->rhs = analyzing(node->rhs);
   }
   if (node->is_omiited) {
      node->is_omiited = analyzing(node->is_omiited);
   }
   for (j = 0; j < 3; j++) {
      if (node->conds[j]) {
         node->conds[j] = analyzing(node->conds[j]);
      }
   }
   if (node->argc > 0) {
      for (j = 0; j < node->argc; j++) {
         if (node->args[j]) {
            node->args[j] = analyzing(node->args[j]);
         }
      }
   }
   if (node->code) {
      for (j = 0; j < node->code->len && node->code->data[j]; j++) {
         node->code->data[j] = (Token *)analyzing((Node *)node->code->data[j]);
      }
   }

   switch (node->ty) {
      case ND_RETURN:
         if (env->ret == NULL) {
            error("try to Return to NULL: Unexpected behaviour\n");
         }

         // treat TY_AUTO
         if (env->ret->ty == TY_AUTO) {
            copy_type(node->lhs->type, env->ret);
         }
         if (node->lhs) {
            node->type = node->lhs->type;
         } // if return to VOID, there is no lhs
         break;
      case ND_SIZEOF:
         if (node->conds[0]) {
            // evaluate node->conds[0] and return its type
            node = new_long_num_node(cnt_size(node->conds[0]->type));

         } else {
            // ND_SIZEOF should generate its type information.
            // This is because: on parse, no (structured) type size are
            // avaliable
            node = new_long_num_node(cnt_size(node->sizeof_type));
         }
         break;
      case ND_ADD:
      case ND_SUB:
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->rhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->rhs, NULL);
            node->rhs->type = node->rhs->lhs->type;
            node->rhs->num_val = cnt_size(node->lhs->type->ptrof);
            node->type = node->lhs->type;
         } else if (node->rhs->type->ty == TY_PTR ||
                    node->rhs->type->ty == TY_ARRAY) {
            node->lhs = new_node(ND_MULTIPLY_IMMUTABLE_VALUE, node->lhs, NULL);
            node->lhs->type = node->rhs->lhs->type;
            node->lhs->num_val = cnt_size(node->rhs->type->ptrof);
            node->type = node->rhs->type;
         } else {
            node = implicit_althemic_type_conversion(node);
            node->type = node->lhs->type;
            break;
         }
         if (node->rhs->type->ty == TY_PTR || node->rhs->type->ty == TY_ARRAY) {
            node->type = node->rhs->type;
            // TY_PTR no matter when node->lhs->node->lhs is INT or PTR
         } else if (node->lhs->type->ty == TY_PTR ||
                    node->lhs->type->ty == TY_ARRAY) {
            node->type = node->lhs->type;
            // TY_PTR no matter when node->lhs->node->lhs is INT or PTR
         }
         break;
      case ND_MUL:
         node = implicit_althemic_type_conversion(node);
         node->type = node->lhs->type;
         break;
      case ND_ISEQ:
      case ND_ISNOTEQ:
      case ND_ISLESSEQ:
      case ND_ISMOREEQ:
      case ND_LESS:
      case ND_GREATER:
         node->type = find_typed_db("int", typedb);
         node = implicit_althemic_type_conversion(node);
         break;
      case ND_FPLUSPLUS:
      case ND_FSUBSUB:
         // Moved to analyzing process.
         if (node->lhs->type->ty == TY_PTR || node->lhs->type->ty == TY_ARRAY) {
            node->num_val = type2size(node->lhs->type->ptrof);
         }
         node->type = node->lhs->type;
         break;
      case ND_ADDRESS:
         node->type = new_type();
         node->type->ty = TY_PTR;
         node->type->ptrof = node->lhs->type;
         break;
      case ND_DEREF:
         node->type = node->lhs->type->ptrof;
         if (!node->type) {
            error("Error: Dereference on NOT pointered.");
         }
         break;
      case ND_ASSIGN:
         if (node->lhs->type->is_const) {
            error("Error: Assign to constant value.\n");
         }
         if (node->lhs->type->ty != node->rhs->type->ty) {
            node->rhs = new_node(ND_CAST, node->rhs, NULL);
            node->rhs->type = node->lhs->type;
         }
         node->type = node->lhs->type;
         break;
      case ND_DOT:
         if (node->lhs->type->ty != TY_STRUCT) {
            error("Error: dot operator to NOT struct\n");
         }
         node->type = (Type *)map_get(node->lhs->type->structure, node->name);
         if (!node->type) {
            error("Error: structure not found: %s\n", node->name);
         }
         // Member Accesss Control p.231
         if ((lang & 1) && (node->type->memaccess == HIDED)) {
            error("Error: access to private item: %s\n", node->name);
         }
         if ((lang & 1) && ((node->type->memaccess == PRIVATE) ||
                            (node->type->memaccess == PROTECTED))) {
            if (!env->current_class || !env->current_class->var_name ||
                (strcmp(env->current_class->var_name, "this"))) {
               // TODO
               //(strcmp(env->current_class->type_name,
               // node->type->type_name)))) {
               error("Error: access to private item: %s\n", node->name);
            }
         }
         break;
      case ND_NEG:
         node->type = node->lhs->type;
         if (node->type->ty == TY_DOUBLE) {
            neg_double_generate = 1;
         }
         if (node->type->ty == TY_FLOAT) {
            neg_float_generate = 1;
         }
         break;
      case ND_MULTIPLY_IMMUTABLE_VALUE:
         node->type = node->lhs->type;
         break;
      case ND_COMMA:
         node->type = node->rhs->type;
         break;
      case ND_FUNC:
         if (node->funcdef) {
            // there are new typedefs. {
            node->type = node->funcdef->type->ret;
         } else {
            node->type = find_typed_db("int", typedb);
         }
         break;
      case ND_VASTART:
         node->num_val = env->current_func->argc;
         node->type = node->lhs->type;
         break;
      case '?':
         /* arithmetic, arithmetic -> arithmetic
          * structure, structure -> structure
          * void, void -> void
          * pointer, pointer/NULL -> pointer
          * (differently qualified) compatible types
          * null or not -> other
          * coid -> pointer to qualified void
          */
         /* TODO: Support NULL pointer & void */
         node->type = node->lhs->type;
         if (node->type->ty != TY_STRUCT || node->type->ty != TY_PTR || node->type->ty != TY_ARRAY || node->type->ty != TY_VOID) {
            node = implicit_althemic_type_conversion(node);
         }
         break;
      default:
         if (node->type == NULL && node->lhs && node->lhs->type) {
            node->type = node->lhs->type;
         }
         break;
   }

   env = prev_env;
   return node;
}


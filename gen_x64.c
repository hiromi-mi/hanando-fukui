// gen_x64.c
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
#include <stdio.h>
#include <stdlib.h>
extern Vector *globalcode;
extern Vector *strs;
extern Vector *floats;
extern Vector *float_doubles;
extern Map *global_vars;
extern Map *funcdefs_generated_template;
extern Map *funcdefs;
extern Map *consts;
extern Map *typedb;
extern Map *struct_typedb;
extern Map *current_local_typedb;

int reg_table[6];
int float_reg_table[8];
char *registers8[5];
char *registers16[5];
char *registers32[5];
char *registers64[5];
char *float_registers[8];
char *float_arg_registers[6];
char arg_registers[6][4];

int if_cnt = 0;
int for_while_cnt = 0;
int neg_float_generate = 0;
int neg_double_generate = 0;

int env_for_while = 0;
int env_for_while_switch = 0;



#define NO_REGISTER NULL

Register *gen_register_rightval(Node *node, int unused_eval);

void globalvar_gen(void) {
   puts(".data");
   int j;
   for (j = 0; j < global_vars->keys->len; j++) {
      Type *valdataj = (Type *)global_vars->vals->data[j];
      char *keydataj = (char *)global_vars->keys->data[j];
      if (valdataj->is_extern) {
         // Do not write externed values.
         continue;
      }
      if (valdataj->initstr) {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".size %s, %d\n", keydataj, cnt_size(valdataj));
         printf("%s:\n", keydataj);
         printf(".quad %s\n", valdataj->initstr);
         puts(".text");
      } else if (valdataj->initval == 0) {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".comm %s, %d\n", keydataj, cnt_size(valdataj));
      } else {
         printf(".type %s,@object\n", keydataj);
         printf(".global %s\n", keydataj);
         printf(".size %s, %d\n", keydataj, cnt_size(valdataj));
         printf("%s:\n", keydataj);
         printf(".long %d\n", valdataj->initval);
         puts(".text");
      }
   }
   for (j = 0; j < strs->len; j++) {
      printf(".LC%d:\n", j);
      printf(".string \"%s\"\n", (char *)strs->data[j]);
   }
   for (j = 0; j < floats->len; j++) {
      printf(".LCF%d:\n", j);
      int *int_repr = (int *)floats->data[j];
      printf(".long %d\n", *int_repr);
   }
   for (j = 0; j < float_doubles->len; j++) {
      printf(".LCD%d:\n", j);
      long *repr = (long *)float_doubles->data[j];
      printf(".long %ld\n", *repr & 0xFFFFFFFF);
      printf(".long %ld\n", *repr >> 32);
   }

   // using in ND_NEG
   if (neg_double_generate) {
      printf(".LCDNEGDOUBLE:\n");
      printf(".long 0\n");
      printf(".long -2147483648\n");
   }
   if (neg_float_generate) {
      printf(".LCDNEGFLOAT:\n");
      printf(".long 2147483648\n");
   }
}

void gen_register_top(void) {
   init_reg_table();
   init_reg_registers();
   int j;
   for (j = 0; j < globalcode->len; j++) {
      gen_register_rightval((Node *)globalcode->data[j], 1);
   }
}

// These registers will be used to map into registers
void init_reg_registers(void) {
   // This code is valid (and safe) because RHS is const ptr. lreg[7] -> on top
   // of "r10b"
   // "r15" will be used as rsp alignment register.
   strcpy(arg_registers[0], "rdi");
   strcpy(arg_registers[1], "rsi");
   strcpy(arg_registers[2], "rdx");
   strcpy(arg_registers[3], "rcx");
   strcpy(arg_registers[4], "r8");
   strcpy(arg_registers[5], "r9");

   registers8[0] = "r12b";
   registers8[1] = "r13b";
   registers8[2] = "r14b";
   registers8[3] = "r11b";
   registers8[4] = "r10b";
   registers16[0] = "r12w";
   registers16[1] = "r13w";
   registers16[2] = "r14w";
   registers16[3] = "r11w";
   registers16[4] = "r10w";
   registers32[0] = "r12d";
   registers32[1] = "r13d";
   registers32[2] = "r14d";
   registers32[3] = "r11d";
   registers32[4] = "r10d";
   registers64[0] = "r12";
   registers64[1] = "r13";
   registers64[2] = "r14";
   registers64[3] = "r11";
   registers64[4] = "r10";
   float_arg_registers[0] = "xmm0";
   float_arg_registers[1] = "xmm1";
   float_arg_registers[2] = "xmm2";
   float_arg_registers[3] = "xmm3";
   float_arg_registers[4] = "xmm4";
   float_arg_registers[5] = "xmm5";
   float_registers[0] = "xmm6";
   float_registers[1] = "xmm7";
   float_registers[2] = "xmm8";
   float_registers[3] = "xmm9";
   float_registers[4] = "xmm10";
   float_registers[5] = "xmm11";
   float_registers[6] = "xmm12";
   float_registers[7] = "xmm13";
}

char *id2reg8(int id) { return registers8[id]; }

char *id2reg32(int id) { return registers32[id]; }

char *id2reg64(int id) { return registers64[id]; }

void init_reg_table(void) {
   int j;
   for (j = 0; j < 6; j++) { // j = 6 means r15
      reg_table[j] = -1;     // NEVER_USED REGISTERS
   }
}

char *gvar_size2reg(int size, char *name) {
   char *_str = malloc(sizeof(char) * 256);
   if (size == 1) {
      snprintf(_str, 255, "byte ptr %s[rip]", name);
   } else if (size == 4) {
      snprintf(_str, 255, "dword ptr %s[rip]", name);
   } else {
      snprintf(_str, 255, "qword ptr %s[rip]", name);
   }
   return _str;
}

char *node2specifier(Node *node) {
   int size;
   size = type2size(node->type);
   if (size == 1) {
      return "byte ptr";
   } else if (size == 4) {
      return "dword ptr";
   } else {
      return "qword ptr";
   }
}

char *type2mov(Type *type) {
   switch (type->ty) {
      case TY_FLOAT:
         return "movss";
      case TY_DOUBLE:
         return "movsd";
      /*case TY_CHAR:
         return "movzx";*/
      default:
         return "mov";
   }
}

char *gvar_node2reg(Node *node, char *name) {
   return gvar_size2reg(type2size(node->type), name);
}

char *size2reg(int size, Register *reg) {
   if (reg->kind == R_REGISTER) {
      if (size == 1) {
         return id2reg8(reg->id);
      } else if (size == 4) {
         return id2reg32(reg->id);
      } else {
         return id2reg64(reg->id);
      }
      // with type specifier.
   } else if (reg->kind == R_LVAR || reg->kind == R_REVERSED_LVAR) {
      char *_str = malloc(sizeof(char) * 256);
      if (size == 1) {
         snprintf(_str, 255, "byte ptr [rbp-%d]", reg->id);
      } else if (size == 4) {
         snprintf(_str, 255, "dword ptr [rbp-%d]", reg->id);
      } else {
         snprintf(_str, 255, "qword ptr [rbp-%d]", reg->id);
      }
      return _str;
   } else if (reg->kind == R_GVAR) {
      return gvar_size2reg(size, reg->name);
   } else if (reg->kind == R_XMM) {
      return float_registers[reg->id];
   }
   error("Error: Cannot Have Register\n");
}
char *node2reg(Node *node, Register *reg) {
   if (reg->kind == R_LVAR || reg->kind == R_REVERSED_LVAR) {
      return size2reg(reg->size, reg);
   }
   return size2reg(type2size(node->type), reg);
}

Register *float_retain_reg(void) {
   int j;
   for (j = 0; j < 8; j++) {
      if (float_reg_table[j] > 0)
         continue;
      float_reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_XMM;
      return reg;
   }
   error("No more float registers are avaliable\n");
}

Register *retain_reg(void) {
   int j;
   for (j = 0; j < 5; j++) {
      if (reg_table[j] > 0)
         continue;
      if (reg_table[j] < 0 && j < 3) {
      }
      reg_table[j] = 1;

      Register *reg = malloc(sizeof(Register));
      reg->id = j;
      reg->name = NULL;
      reg->kind = R_REGISTER;
      reg->size = -1;
      return reg;
   }
   error("No more registers are avaliable\n");
}

void release_reg(Register *reg) {
   if (!reg) {
      return;
   }
   if (reg->kind == R_REGISTER) {
      reg_table[reg->id] = 0; // USED REGISTER
   } else if (reg->kind == R_XMM) {
      float_reg_table[reg->id] = 0;
   }
   free(reg);
}

void release_all_reg(void) {
   int j;
   for (j = 0; j < 6; j++) {
      // j = 5 means r15
      reg_table[j] = -1;
   }
   for (j = 0; j < 8; j++) {
      float_reg_table[j] = -1;
   }
}

void secure_mutable_with_type(Register *reg, Type *type) {
   Register *new_reg;
   // to enable to change
   if (reg->kind != R_REGISTER && reg->kind != R_XMM) {
      if (type->ty == TY_FLOAT || type->ty == TY_DOUBLE) {
         new_reg = float_retain_reg();
         printf("%s %s, %s\n", type2mov(type),
                size2reg(type2size(type), new_reg), size2reg(reg->size, reg));
      } else {
         new_reg = retain_reg();
         if (reg->size <= 0) {
            printf("mov %s, %s\n", size2reg(8, new_reg), size2reg(8, reg));
         } else if (reg->size == 1) {
            printf("movzx %s, %s\n", size2reg(8, new_reg),
                   size2reg(reg->size, reg));
         } else {
            printf("mov %s, %s\n", size2reg(reg->size, new_reg),
                   size2reg(reg->size, reg));
         }
      }
      reg->id = new_reg->id;
      reg->kind = new_reg->kind;
      reg->name = new_reg->name;
      reg->size = new_reg->size;
   }
}

Register *gen_register_leftval(Node *node) {
   Register *temp_reg;
   Register *lhs_reg;
   // Treat as lvalue.
   if (!node) {
      return NO_REGISTER;
   }
   switch (node->ty) {
      case ND_IDENT:
         temp_reg = retain_reg();
         if (node->local_variable->lvar_offset >= 0) {
            printf("lea %s, [rbp-%d]\n", id2reg64(temp_reg->id),
                   node->local_variable->lvar_offset);
         } else {
            printf("lea %s, [rbp-%d]\n", id2reg64(temp_reg->id),
                   -node->local_variable->lvar_offset);
         }
         return temp_reg;

      case ND_DEREF:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         if (lhs_reg->kind != R_REGISTER && lhs_reg->kind != R_XMM) {
            temp_reg = retain_reg();
            printf("mov %s, %s\n", size2reg(8, temp_reg),
                   node2reg(node->lhs, lhs_reg));
            release_reg(lhs_reg);
            lhs_reg = temp_reg;
         }
         return lhs_reg;

      case ND_GLOBAL_IDENT:
      case ND_STRING:
      case ND_SYMBOL:
         temp_reg = retain_reg();
         printf("lea %s, %s\n", id2reg64(temp_reg->id),
                gvar_node2reg(node, node->name));
         return temp_reg;

      case ND_DOT:
         lhs_reg = gen_register_leftval(node->lhs);
         if (node->type->offset > 0) {
            printf("add %s, %d\n", size2reg(8, lhs_reg), node->type->offset);
         }
         return lhs_reg;

      default:
         error("Error: NOT lvalue");
         return NO_REGISTER;
   }
}

void extend_al_ifneeded(Node *node, Register *reg) {
   if (type2size(node->type) > 1) {
      printf("movzx %s, al\n", node2reg(node, reg));
   } else {
      printf("mov %s, al\n", node2reg(node, reg));
   }
}

void restore_callee_reg(void) {
   // stored in retain_reg() or
   // puts("mov r15, [rbp-24]");in ND_FUNC
   int j;
   // because r12,...are callee-savevd
   for (j = 2; j >= 0; j--) {
      // only registers already stored will be restoed
      if (reg_table[j] >= 0) { // used (>= 1)
         printf("mov %s, qword ptr [rbp-%d]\n", registers64[j], j * 8 + 8);
      }
   }
   if (reg_table[5] > 0) { // treat as r15
      printf("mov rsp, r15\n");
      puts("mov r15, qword ptr [rbp-32]");
   }
}

int save_reg(void) {
   int j;
   int stored_cnt = 0;
   // because only r10 and r11 are caller-saved
   for (j = 3; j < 5; j++) {
      if (reg_table[j] <= 0)
         continue;
      printf("push %s\n", registers64[j]);
      stored_cnt++;
   }
   return stored_cnt;
}

int restore_reg(void) {
   int j;
   int restored_cnt = 0;
   // 4 because only r10 and r11 are caller-saved
   for (j = 4; j >= 3; j--) {
      if (reg_table[j] <= 0)
         continue;
      printf("pop %s\n", registers64[j]);
      restored_cnt++;
   }
   return restored_cnt;
}

Register *gen_register_rightval(Node *node, int unused_eval) {
   Register *temp_reg;
   Register *lhs_reg;
   Register *rhs_reg;
   int j = 0;
   int cur_if_cnt;
   int func_call_float_cnt = 0;
   int func_call_should_sub_rsp = 0;

   if (!node) {
      return NO_REGISTER;
   }

   // Write down line number
   if (node->pline >= 0) {
      // 1 means the file number
      printf(".loc 1 %d\n", node->pline);
   }
   if (unused_eval && (node->ty == ND_NUM || node->ty == ND_FLOAT || node->ty == ND_STRING)) {
      // 未使用な式なので0 を返す
      return NO_REGISTER;
   }
   switch (node->ty) {
      case ND_NUM:
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            // Delete previous value because of mov al, %ld will not delete all
            // value in rax
            printf("xor %s, %s\n", size2reg(8, temp_reg),
                   size2reg(8, temp_reg));
         }
         printf("mov %s, %ld\n", node2reg(node, temp_reg), node->num_val);
         return temp_reg;

      case ND_FLOAT:
         temp_reg = float_retain_reg();
         printf("%s %s, %s[rip]\n", type2mov(node->type),
                node2reg(node, temp_reg), node->name);
         return temp_reg;

      case ND_STRING:
      case ND_SYMBOL:
         temp_reg = retain_reg();
         printf("lea %s, qword ptr %s[rip]\n", size2reg(8, temp_reg),
                node->name);
         return temp_reg;
         // return with toplevel char ptr.

      case ND_EXTERN_SYMBOL:
         // TODO should delete
         temp_reg = retain_reg();
         printf("mov %s, qword ptr [rip + %s@GOTPCREL]\n",
                size2reg(8, temp_reg), node->name);
         printf("mov %s, qword ptr [%s]\n", size2reg(8, temp_reg),
                size2reg(8, temp_reg));
         return temp_reg;

      case ND_IDENT:
         if (node->type->ty == TY_ARRAY) {
            return gen_register_leftval(node);
         } else {
            temp_reg = malloc(sizeof(Register));
            if (node->local_variable->lvar_offset < 0) {
               temp_reg->id = -node->local_variable->lvar_offset;
               temp_reg->kind = R_REVERSED_LVAR;
            } else {
               temp_reg->id = node->local_variable->lvar_offset;
               temp_reg->kind = R_LVAR;
            }
            temp_reg->name = NULL;
            temp_reg->size = type2size(node->type);
            return temp_reg;
         }

      case ND_GLOBAL_IDENT:
         if (node->type->ty == TY_ARRAY) {
            return gen_register_leftval(node);
         } else {
            temp_reg = malloc(sizeof(Register));
            temp_reg->id = 0;
            temp_reg->kind = R_GVAR;
            temp_reg->name = node->name;
            temp_reg->size = type2size(node->type);
            return temp_reg;
         }

      case ND_DOT:
         lhs_reg = gen_register_leftval(node->lhs);
         switch (node->type->ty) {
            case TY_ARRAY:
               if (node->type->offset > 0) {
                  printf("add %s, %d\n", size2reg(8, lhs_reg),
                         node->type->offset);
               }
               break;
            case TY_FLOAT:
            case TY_DOUBLE:
               temp_reg = float_retain_reg();
               printf("%s %s, [%s+%d]\n", type2mov(node->type),
                      node2reg(node, temp_reg), id2reg64(lhs_reg->id),
                      node->type->offset);
               release_reg(lhs_reg);
               return temp_reg;
            default:
               printf("mov %s, [%s+%d]\n", node2reg(node, lhs_reg),
                      id2reg64(lhs_reg->id), node->type->offset);
               break;
         }
         return lhs_reg;
      case ND_ASSIGN:
         // This behaivour can be revised. like [rbp-8+2]
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            secure_mutable_with_type(rhs_reg, node->lhs->type);
            if (node->lhs->type->ty == TY_FLOAT ||
                node->lhs->type->ty == TY_DOUBLE) {
               printf("%s %s, %s\n", type2mov(node->rhs->type),
                      node2reg(node->lhs, lhs_reg),
                      node2reg(node->lhs, rhs_reg));
            } else {
               // TODO
               printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg),
                      node2reg(node->lhs, rhs_reg));
            }
            release_reg(rhs_reg);
            if (unused_eval) {
               release_reg(lhs_reg);
            }
            return lhs_reg;
         } else if (node->lhs->ty == ND_DOT && node->lhs->type->offset > 0) {
            /*
            add r12, 152
            mov qword ptr [r12], r13
            */
            // can be written as
            /*
            mov qword ptr [r12+152], r13
            */
            lhs_reg = gen_register_leftval(node->lhs->lhs);
            if (node->rhs->ty == ND_NUM) {
               rhs_reg = NULL;
               // can be written as immutable value
               printf("%s %s [%s+%d], %ld\n", type2mov(node->type),
                      node2specifier(node), id2reg64(lhs_reg->id),
                      node->lhs->type->offset, node->rhs->num_val);
            } else {
               rhs_reg = gen_register_rightval(node->rhs, 0);
               secure_mutable_with_type(rhs_reg, node->rhs->type);
               printf("%s %s [%s+%d], %s\n", type2mov(node->type),
                      node2specifier(node), id2reg64(lhs_reg->id),
                      node->lhs->type->offset, node2reg(node->lhs, rhs_reg));
            }
            release_reg(lhs_reg);
            if (unused_eval) {
               release_reg(rhs_reg);
            }
            return rhs_reg;
         } else {
            lhs_reg = gen_register_leftval(node->lhs);
            rhs_reg = gen_register_rightval(node->rhs, 0);
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            printf("%s %s [%s], %s\n", type2mov(node->type),
                   node2specifier(node), id2reg64(lhs_reg->id),
                   node2reg(node->lhs, rhs_reg));
            release_reg(lhs_reg);
            if (unused_eval) {
               release_reg(rhs_reg);
            }
            return rhs_reg;
         }

      case ND_CAST:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         if (node->type->ty == TY_FLOAT) {
            switch (node->lhs->type->ty) {
               case TY_INT:
                  temp_reg = float_retain_reg();
                  printf("cvtsi2ss %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               case TY_DOUBLE:
                  temp_reg = float_retain_reg();
                  printf("cvtsd2ss %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               default:
                  break;
            }
            return lhs_reg;
         }
         if (node->type->ty == TY_DOUBLE) {
            switch (node->lhs->type->ty) {
               case TY_INT:
                  temp_reg = float_retain_reg();
                  printf("cvtsi2sd %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               case TY_FLOAT:
                  temp_reg = float_retain_reg();
                  printf("cvtss2sd %s, %s\n", node2reg(node, temp_reg),
                         node2reg(node->lhs, lhs_reg));
                  release_reg(lhs_reg);
                  return temp_reg;
               default:
                  break;
            }
            return lhs_reg;
         }
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (node->type->ty == node->lhs->type->ty) {
            return lhs_reg;
         }
         switch (node->lhs->type->ty) {
            case TY_CHAR:
               // TODO treat as unsigned char.
               // for signed char, use movsx instead of.
               printf("movzx %s, %s\n", node2reg(node, lhs_reg),
                      id2reg8(lhs_reg->id));
               break;
            case TY_FLOAT:
               temp_reg = retain_reg();
               if (node->type->ty == TY_INT) {
                  // TODO It it not supported yet: to convert float -> char is
                  // not supported yet
                  printf("cvttss2si %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else if (node->type->ty == TY_LONG) {
                  printf("cvttss2siq %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else {
                  error("Error: float -> unknown type convert: %d\n",
                        node->type->ty);
               }
               release_reg(lhs_reg);
               return temp_reg;
            case TY_DOUBLE:
               temp_reg = retain_reg();
               if (node->type->ty == TY_INT) {
                  // TODO It it not supported yet: to convert float -> char is
                  // not supported yet
                  printf("cvttsd2si %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else if (node->type->ty == TY_LONG) {
                  printf("cvttsd2siq %s, %s\n", node2reg(node->lhs, temp_reg),
                         node2reg(node->lhs, lhs_reg));
               } else {
                  error("Error: double -> unknown type convert: %d\n",
                        node->type->ty);
               }
               release_reg(lhs_reg);
               return temp_reg;
            default:
               break;
         }
         return lhs_reg;

      case ND_QUESTION:
         // TODO Support double
         temp_reg = gen_register_rightval(node->conds[0], 0);
         printf("cmp %s, 0\n", node2reg(node->conds[0], temp_reg));
         release_reg(temp_reg);

         cur_if_cnt = if_cnt++;
         printf("je .Lendif%d\n", cur_if_cnt);
         temp_reg = gen_register_rightval(node->lhs, 0);
         printf("jmp .Lelseend%d\n", cur_if_cnt);
         printf(".Lendif%d:\n", cur_if_cnt);
         // There are else
         // TODO この挙動は, retain_reg() 直後に同一レジスタが確保されることに依存している
         release_reg(temp_reg);
         temp_reg = gen_register_rightval(node->rhs, 0);
         printf(".Lelseend%d:\n", cur_if_cnt);
         return temp_reg;

      case ND_COMMA:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         release_reg(lhs_reg);
         return rhs_reg;

      case ND_ADD:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("addss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("addsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else {
            printf("add %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_SUB:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("subss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("subsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else {
            printf("sub %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_MULTIPLY_IMMUTABLE_VALUE:
         // after optimizing, this will be deleted.
         lhs_reg = gen_register_rightval(node->lhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("imul %s, %ld\n", node2reg(node, lhs_reg), node->num_val);
         return lhs_reg;

      case ND_MUL:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         if (node->lhs->type->ty == TY_FLOAT) {
            printf("mulss %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            printf("mulsd %s, %s\n", node2reg(node, lhs_reg),
                   node2reg(node, rhs_reg));
         } else {
            printf("imul %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_DIV:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         if (node->lhs->type->ty == TY_FLOAT) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("divss %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
         } else if (node->lhs->type->ty == TY_DOUBLE) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("divsd %s, %s\n", node2reg(node->lhs, lhs_reg),
                   node2reg(node->rhs, rhs_reg));
         } else {
            // TODO should support char
            printf("mov %s, %s\n", _rax(node->lhs),
                   node2reg(node->lhs, lhs_reg));
            puts("cqo");
            printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("mov %s, %s\n", node2reg(node->lhs, lhs_reg),
                   _rax(node->lhs));
         }
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_MOD:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // TODO should support char: idiv only support r32 or r64
         printf("mov %s, %s\n", _rax(node->lhs), node2reg(node->lhs, lhs_reg));
         puts("cqo");
         printf("idiv %s\n", node2reg(node->rhs, rhs_reg));
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("mov %s, %s\n", node2reg(node, lhs_reg), _rdx(node->lhs));
         release_reg(rhs_reg);
         if (unused_eval) {
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            return lhs_reg;
         }

      case ND_XOR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("xor %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;

      case ND_AND:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("and %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;
         break;

      case ND_OR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("or %s, %s\n", node2reg(node, lhs_reg),
                node2reg(node, rhs_reg));
         release_reg(rhs_reg);
         return lhs_reg;

      case ND_RSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         puts("xor rcx, rcx");
         printf("mov cl, %s\n", size2reg(1, rhs_reg));
         release_reg(rhs_reg);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("sar %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_LSHIFT:
         // SHOULD be int
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         // FIXME: for signed int (Arthmetric)
         // mov rdi[8] -> rax
         puts("xor rcx, rcx");
         printf("mov cl, %s\n", size2reg(1, rhs_reg));
         release_reg(rhs_reg);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         printf("sal %s, cl\n", node2reg(node->lhs, lhs_reg));
         return lhs_reg;

      case ND_ISEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("sete al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISNOTEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         cmp_regs(node, lhs_reg, rhs_reg);
         puts("setne al");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_GREATER:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         cmp_regs(node, lhs_reg, rhs_reg);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            puts("seta al");
         } else {
            puts("setg al");
         }
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_LESS:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);

         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            // lhs_reg should be xmm, but rhs should be xmm or memory
            // lhs_reg and rhs_reg are reversed because of comisd
            cmp_regs(node, rhs_reg, lhs_reg);
            puts("seta al");
         } else {
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            cmp_regs(node, lhs_reg, rhs_reg);
            puts("setl al");
         }

         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISMOREEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         cmp_regs(node, lhs_reg, rhs_reg);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            puts("setnb al");
         } else {
            puts("setge al");
         }
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_ISLESSEQ:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         if (node->lhs->type->ty == TY_FLOAT ||
             node->lhs->type->ty == TY_DOUBLE) {
            // lhs_reg should be xmm, but rhs should be xmm or memory
            // lhs_reg and rhs_reg are reversed because of comisd
            secure_mutable_with_type(rhs_reg, node->rhs->type);
            cmp_regs(node, rhs_reg, lhs_reg);
            puts("setnb al");
         } else {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            cmp_regs(node, lhs_reg, rhs_reg);
            puts("setle al");
         }
         puts("and al, 1");
         release_reg(lhs_reg);
         release_reg(rhs_reg);

         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_RETURN:
         if (node->lhs) {
            lhs_reg = gen_register_rightval(node->lhs, 0);
            if (node->lhs->type->ty == TY_FLOAT ||
                node->lhs->type->ty == TY_DOUBLE) {
               printf("%s xmm0, %s\n", type2mov(node->lhs->type),
                      node2reg(node->lhs, lhs_reg));
            } else {
               printf("mov %s, %s\n", _rax(node->lhs),
                      node2reg(node->lhs, lhs_reg));
            }
            release_reg(lhs_reg);
         }
         restore_callee_reg();
         puts("mov rsp, rbp");
         puts("pop rbp");
         puts("ret");
         return NO_REGISTER;

      case ND_ADDRESS:
         if (node->lhs->ty == ND_IDENT || node->lhs->ty == ND_GLOBAL_IDENT) {
            temp_reg = retain_reg();
            lhs_reg = gen_register_rightval(node->lhs, 0);
            printf("lea %s, %s\n", id2reg64(temp_reg->id),
                   node2reg(node, lhs_reg));
            release_reg(lhs_reg);
            return temp_reg;
         } else {
            return gen_register_leftval(node->lhs);
         }

      case ND_NEG:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         secure_mutable_with_type(lhs_reg, node->type);
         switch (node->type->ty) {
            case TY_FLOAT:
               temp_reg = float_retain_reg();
               printf("movq %s, qword ptr .LCDNEGFLOAT[rip]\n",
                      node2reg(node, temp_reg));
               printf("xorps %s, %s\n", node2reg(node, lhs_reg),
                      node2reg(node, temp_reg));
               release_reg(temp_reg);
               break;
            case TY_DOUBLE:
               temp_reg = float_retain_reg();
               printf("movq %s, qword ptr .LCDNEGDOUBLE[rip]\n",
                      node2reg(node, temp_reg));
               printf("xorpd %s, %s\n", node2reg(node, lhs_reg),
                      node2reg(node, temp_reg));
               release_reg(temp_reg);
               break;
            default:
               printf("neg %s\n", node2reg(node, lhs_reg));
               break;
         }
         return lhs_reg;

      case ND_FPLUSPLUS:
         lhs_reg = gen_register_leftval(node->lhs);
         if (unused_eval) {
            secure_mutable_with_type(lhs_reg, node->lhs->type);
            printf("add %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();
            secure_mutable_with_type(lhs_reg, node->lhs->type);

            if (type2size(node->type) == 1) {
               printf("movzx %s, [%s]\n", size2reg(4, temp_reg),
                      id2reg64(lhs_reg->id));
            } else {
               printf("mov %s, [%s]\n", node2reg(node, temp_reg),
                      id2reg64(lhs_reg->id));
            }
            printf("add %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return temp_reg;
         }

      case ND_FSUBSUB:
         lhs_reg = gen_register_leftval(node->lhs);
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (unused_eval) {
            printf("sub %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return NO_REGISTER;
         } else {
            temp_reg = retain_reg();

            printf("mov %s, [%s]\n", node2reg(node, temp_reg),
                   id2reg64(lhs_reg->id));
            printf("sub %s [%s], %ld\n", node2specifier(node),
                   id2reg64(lhs_reg->id), node->num_val);
            release_reg(lhs_reg);
            return temp_reg;
         }

      case ND_APOS:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         puts("sete al");
         release_reg(lhs_reg);
         temp_reg = retain_reg();
         extend_al_ifneeded(node, temp_reg);
         return temp_reg;

      case ND_DEREF:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         // this is because we cannot [[lhs_reg]] (float deref)
         secure_mutable_with_type(lhs_reg, node->lhs->type);
         if (type2size(node->type) == 1) {
            // when reading char, we should read just 1 byte
            printf("movzx %s, byte ptr [%s]\n", size2reg(4, lhs_reg),
                   size2reg(8, lhs_reg));
         } else if (node->type->ty == TY_FLOAT || node->type->ty == TY_DOUBLE) {
            temp_reg = float_retain_reg();
            printf("%s %s, [%s]\n", type2mov(node->type),
                   node2reg(node, temp_reg), size2reg(8, lhs_reg));
            release_reg(lhs_reg);
            return temp_reg;
         } else if (node->type->ty != TY_ARRAY) {
            printf("mov %s, [%s]\n", node2reg(node, lhs_reg),
                   size2reg(8, lhs_reg));
         }
         return lhs_reg;

      case ND_FDEF:
         printf(".type %s,@function\n", node->gen_name);
         printf(".global %s\n", node->gen_name); // to visible for ld
         printf("%s:\n", node->gen_name);
         puts("push rbp");
         puts("mov rbp, rsp");
         printf("sub rsp, %d\n", *node->env->rsp_offset_max);
         int is_entrypoint = !(strncmp(node->gen_name, "main", 5));
         int fdef_int_arguments = 0;
         int fdef_float_arguments = 0;
         j = 0;
         for (fdef_int_arguments = 0;
              (fdef_int_arguments + fdef_float_arguments) < node->argc; j++) {
            if (node->args[j]->type->ty == TY_FLOAT ||
                node->args[j]->type->ty == TY_DOUBLE) {
               temp_reg = gen_register_rightval(node->args[j], 0);
               printf("%s %s, %s\n", type2mov(node->args[j]->type),
                      node2reg(node->args[j], temp_reg),
                      float_arg_registers[fdef_float_arguments]);
               fdef_float_arguments++;
            } else {
               temp_reg = gen_register_rightval(node->args[j], 0);
               printf("mov %s, %s\n", size2reg(8, temp_reg),
                      arg_registers[fdef_int_arguments]);
               fdef_int_arguments++;
            }
         }
         if (node->is_omiited) {
            for (j = 0; j < 6; j++) {
               printf("mov [rbp-%d], %s\n",
                      node->is_omiited->local_variable->lvar_offset - j * 8,
                      arg_registers[j]);
            }
            for (j = 0; j < 8; j++) {
               printf("movaps [rbp-%d], xmm%d\n",
                      node->is_omiited->local_variable->lvar_offset - 8 * 6 -
                          j * 16,
                      j);
            }
         }
         printf("jmp .LFD%s\n", node->gen_name);
         printf(".LFB%s:\n", node->gen_name);
         for (j = 0; j < node->code->len; j++) {
            // read inside functions.
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         if (node->type->ret->ty == TY_VOID || is_entrypoint) {
            if (!is_entrypoint) {
               restore_callee_reg();
            } else {
               puts("mov rax, 0"); // main() function will return 0
            }
            puts("mov rsp, rbp");
            puts("pop rbp");
            puts("ret");
         }

         printf(".LFD%s:\n", node->gen_name);
         // r10, r11 are caller-saved so no need to consider
         // store callee saved registers. will be restored in
         // restore_callee_reg() r12 will be stored in [rbp-0] r13 will be
         // stored in [rbp-8] r14 will be stored in [rbp-16]
         for (j = 0; j < 3; j++) {
            if (reg_table[j] >= 0) { // used (>= 1)
               printf("mov qword ptr [rbp-%d], %s\n", j * 8 + 8,
                      registers64[j]);
            }
         }
         if (reg_table[5] > 0) {                 // treat as r15
            puts("mov qword ptr [rbp-32], r15"); // store r15 to [rbp-32]
         }
         printf("jmp .LFB%s\n", node->gen_name);
         // Release All Registers.
         release_all_reg();
         return NO_REGISTER;

      case ND_BLOCK:
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         return NO_REGISTER;

      case ND_EXPRESSION_BLOCK:
         if (node->argc > 0) {
            puts("mov rbp, rsp");
            printf("sub rsp, %d\n", *node->env->rsp_offset_max);
            puts("#test begin expansion");
            for (j = 0; j < node->argc; j++) {
               // temp_reg = gen_register_rightval(node->args[j], 0);
               gen_register_rightval(node->args[j], 1);
               puts("#test end expansion of args");
            }
         }
         for (j = 0; j < node->code->len && node->code->data[j]; j++) {
            gen_register_rightval((Node *)node->code->data[j], 1);
         }
         if (node->argc > 0) {
            puts("mov rsp, rbp");
            puts("#test end expansion ");
         }
         return NO_REGISTER;

      case ND_FUNC:
         for (j = 0; j < node->argc; j++) {
            temp_reg = gen_register_rightval(node->args[j], 0);
            if (node->args[j]->type->ty == TY_FLOAT ||
                node->args[j]->type->ty == TY_DOUBLE) {
               printf("%s %s, %s\n", type2mov(node->args[j]->type),
                      float_arg_registers[func_call_float_cnt],
                      node2reg(node->args[j], temp_reg));
               func_call_float_cnt++;
            } else if (node->args[j]->type->ty == TY_STRUCT) {
               int k = 0;
               switch (temp_reg->kind) {
                  case R_GVAR:
                     func_call_should_sub_rsp += type2size(node->args[j]->type);
                     for (k = type2size(node->args[j]->type) - 8; k >= 0;
                          k -= 8) {
                        printf("push qword ptr %s[rip+%d]\n", temp_reg->name,
                               k);
                     }
                     break;
                  case R_LVAR:
                     func_call_should_sub_rsp += type2size(node->args[j]->type);
                     for (k = type2size(node->args[j]->type) - 8; k >= 0;
                          k -= 8) {
                        printf("push qword ptr [rbp-%d]\n", temp_reg->id - k);
                     }
                     break;
                  default:
                     error("Error: Not IMplemented On ND_FUNC w/ TY_STRUCT\n");
               }
               // TODO This will create "mov eax, 1" even though no float
               // arugments
               func_call_float_cnt++;
            } else {
               secure_mutable_with_type(temp_reg, node->type);
               printf("push %s\n", id2reg64(temp_reg->id));
            }
            release_reg(temp_reg);
         }
         for (j = node->argc - 1 - func_call_float_cnt; j >= 0; j--) {
            // because of function call will break these registers
            printf("pop %s\n", arg_registers[j]);
         }

         save_reg();
         printf("mov eax, %d\n", func_call_float_cnt);

         if (reg_table[5] < 0) {
            reg_table[5] = 1;
         }
         puts("mov r15, rsp");
         puts("and rsp, -16");
         if (node->gen_name) { // ND_GLOBAL_IDENT, called from local vars.
            printf("call %s\n",
                   node->gen_name); // rax should be aligned with the size
         } else if (node->type->context->is_previous_class) {
            char *buf = malloc(sizeof(char) * 256);
            snprintf(buf, 255, "%s::%s", node->type->context->is_previous_class,
                     node->type->context->method_name);
            printf("call %s\n", mangle_func_name(buf));
         } else {
            temp_reg = gen_register_rightval(node->lhs, 0);
            secure_mutable_with_type(temp_reg, node->lhs->type);
            printf("call %s\n", size2reg(8, temp_reg));
            release_reg(temp_reg);
         }
         puts("mov rsp, r15");
         if (func_call_should_sub_rsp > 0) {
            printf("sub rsp, -%d\n", func_call_should_sub_rsp);
         }
         restore_reg();

         if (node->type->ty == TY_VOID || unused_eval == 1) {
            return NO_REGISTER;
         } else if (node->type->ty == TY_FLOAT || node->type->ty == TY_DOUBLE) {
            temp_reg = float_retain_reg();
            // movaps just move all of xmmx
            printf("movaps %s, xmm0\n", node2reg(node, temp_reg));
            return temp_reg;
         } else {
            temp_reg = retain_reg();
            printf("mov %s, %s\n", node2reg(node, temp_reg), _rax(node));
            return temp_reg;
         }

      case ND_VAARG:
         // SEE in AMD64 ABI Draft 0.99.7 p.53 (uglibc.org )
         lhs_reg = gen_register_leftval(node->lhs); // meaning ap
         temp_reg = retain_reg();

         // gp_offset
         printf("mov eax, [%s]\n", id2reg64(lhs_reg->id)); // get gp_offset
         printf("mov edx, eax\n");
         printf("add rdx, 8\n");
         printf("add rax, [%s+16]\n", id2reg64(lhs_reg->id));
         printf("mov [%s], edx\n", id2reg64(lhs_reg->id));
         // only supported register
         printf("mov %s, [rax]\n",
                node2reg(node, temp_reg)); // 1->reg_saved_area
         release_reg(lhs_reg);

         return temp_reg;

      case ND_VASTART:
         lhs_reg = gen_register_leftval(node->lhs); // meaning ap's address
         // node->num_val means the first undefined argument No.
         // from lval_offset:
         // rbp-80 : r0
         // rbp-72 : r1
         // ...
         // rbp-56: r6
         printf("mov dword ptr [%s], %ld\n", id2reg64(lhs_reg->id),
                // omitted_argc * 8
                node->num_val * 8);
         printf("mov dword ptr [%s+4], 304\n", id2reg64(lhs_reg->id));
         printf("lea rax, [rbp-%d]\n", node->rhs->local_variable->lvar_offset);
         printf("mov qword ptr [%s+16], rax\n", id2reg64(lhs_reg->id));
         return NO_REGISTER;

      case ND_VAEND:
         return NO_REGISTER;

      case ND_GOTO:
         printf("jmp %s\n", node->name);
         return NO_REGISTER;

      case ND_LOR:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         release_reg(lhs_reg);
         cur_if_cnt = if_cnt++;
         printf("jne .Lorend%d\n", cur_if_cnt);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         printf("cmp %s, 0\n", node2reg(node->rhs, rhs_reg));
         release_reg(rhs_reg);
         printf(".Lorend%d:\n", cur_if_cnt);
         puts("setne al");
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            printf("mov %s, al\n", node2reg(node, temp_reg));
         } else {
            printf("movzx %s, al\n", node2reg(node, temp_reg));
         }
         return temp_reg;

      case ND_LAND:
         lhs_reg = gen_register_rightval(node->lhs, 0);
         printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
         release_reg(lhs_reg);
         cur_if_cnt = if_cnt++;
         printf("je .Lorend%d\n", cur_if_cnt);
         rhs_reg = gen_register_rightval(node->rhs, 0);
         printf("cmp %s, 0\n", node2reg(node->rhs, rhs_reg));
         release_reg(rhs_reg);
         printf(".Lorend%d:\n", cur_if_cnt);
         puts("setne al");
         temp_reg = retain_reg();
         if (type2size(node->type) == 1) {
            printf("mov %s, al\n", node2reg(node, temp_reg));
         } else {
            printf("movzx %s, al\n", node2reg(node, temp_reg));
         }
         return temp_reg;

      case ND_IF:
         temp_reg = gen_register_rightval(node->args[0], 0);
         printf("cmp %s, 0\n", node2reg(node->args[0], temp_reg));
         release_reg(temp_reg);
         cur_if_cnt = if_cnt++;
         printf("je .Lendif%d\n", cur_if_cnt);
         gen_register_rightval(node->lhs, 1);
         if (node->rhs) {
            printf("jmp .Lelseend%d\n", cur_if_cnt);
         }
         printf(".Lendif%d:\n", cur_if_cnt);
         if (node->rhs) {
            // There are else
            gen_register_rightval(node->rhs, 1);
            printf(".Lelseend%d:\n", cur_if_cnt);
         }
         return NO_REGISTER;

      case ND_SWITCH:
         // TODO: quite dirty
         cur_if_cnt = for_while_cnt++;
         int prev_env_for_while_switch = env_for_while_switch;
         env_for_while_switch = cur_if_cnt;

         lhs_reg = gen_register_rightval(node->lhs, 0);
         // find CASE Labels and lookup into args[0]->code->data
         for (j = 0; node->rhs->code->data[j]; j++) {
            Node *curnode = (Node *)node->rhs->code->data[j];
            if (curnode->ty == ND_CASE) {
               char *input = malloc(sizeof(char) * 256);
               snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
               curnode->name = input; // assign unique ID

               if (type2size(node->lhs->type) !=
                   type2size(curnode->lhs->type)) {
                  curnode->lhs = new_node(ND_CAST, curnode->lhs, NULL);
                  curnode->lhs->type = node->lhs->type;
               }
               temp_reg = gen_register_rightval(curnode->lhs, 0);
               printf("cmp %s, %s\n", node2reg(node->lhs, lhs_reg),
                      node2reg(curnode->lhs, temp_reg));
               release_reg(temp_reg);
               printf("je %s\n", input);
            }
            if (curnode->ty == ND_DEFAULT) {
               char *input = malloc(sizeof(char) * 256);
               snprintf(input, 255, ".L%dC%d", cur_if_cnt, j);
               curnode->name = input;
               printf("jmp %s\n", input);
            }
         }
         release_reg(lhs_reg);
         // content
         gen_register_rightval(node->rhs, 1);
         printf(".Lend%d:\n", cur_if_cnt);
         env_for_while_switch = prev_env_for_while_switch;
         return NO_REGISTER;

      case ND_CASE:
      case ND_DEFAULT:
         // just an def. of goto
         // saved with input
         if (!node->name) {
            error("Error: case statement without switch\n");
         }
         printf("%s:\n", node->name);
         return NO_REGISTER;

      case ND_FOR:
      case ND_WHILE:
      case ND_DOWHILE:
         cur_if_cnt = for_while_cnt++;
         int prev_env_for_while = env_for_while;
         prev_env_for_while_switch = env_for_while_switch;
         env_for_while = cur_if_cnt;
         env_for_while_switch = cur_if_cnt;

         switch (node->ty) {
            case ND_FOR:
               gen_register_rightval(node->conds[0], 1);
               printf("jmp .Lcondition%d\n", cur_if_cnt);
               printf(".Lbeginwork%d:\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               printf(".Lbegin%d:\n", cur_if_cnt);
               gen_register_rightval(node->conds[2], 1);
               printf(".Lcondition%d:\n", cur_if_cnt);
               temp_reg = gen_register_rightval(node->conds[1], 0);

               printf("cmp %s, 0\n", node2reg(node->conds[1], temp_reg));
               release_reg(temp_reg);
               printf("jne .Lbeginwork%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;

            case ND_WHILE:
               printf(".Lbegin%d:\n", cur_if_cnt);
               lhs_reg = gen_register_rightval(node->lhs, 0);
               printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
               release_reg(lhs_reg);
               printf("je .Lend%d\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               printf("jmp .Lbegin%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;

            case ND_DOWHILE:
               printf(".Lbegin%d:\n", cur_if_cnt);
               gen_register_rightval(node->rhs, 1);
               lhs_reg = gen_register_rightval(node->lhs, 0);
               printf("cmp %s, 0\n", node2reg(node->lhs, lhs_reg));
               release_reg(lhs_reg);
               printf("jne .Lbegin%d\n", cur_if_cnt);
               printf(".Lend%d:\n", cur_if_cnt);
               break;
            default:
               break;
         }

         env_for_while = prev_env_for_while;
         env_for_while_switch = prev_env_for_while_switch;
         return NO_REGISTER;

      case ND_BREAK:
         printf("jmp .Lend%d #break\n", env_for_while_switch);
         return NO_REGISTER;

      case ND_CONTINUE:
         printf("jmp .Lbegin%d\n", env_for_while);
         return NO_REGISTER;

      default:
         error("Error: Incorrect Registers %d.\n", node->ty);
   }
   return NO_REGISTER;
}

Node *node_expression(void) {
   Node *node = node_assignment_expression();
   while (1) {
      if (consume_token(',')) {
         node = new_node(',', node, node_assignment_expression());
      } else {
         return node;
      }
   }
   return node;
}

Node *node_conditional_expression(void) {
   Node *node = node_lor();
   Node *conditional_node = NULL;
   if (consume_token('?')) {
      conditional_node = new_node('?', node_expression(), NULL);
      conditional_node->conds[0] = node;
      expect_token(':');
      conditional_node->rhs = node_conditional_expression();
      node = conditional_node;
   }
   return node;
}


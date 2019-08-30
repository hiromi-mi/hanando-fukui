typedef struct {
   long __val[16];
} __sigset_t;

typedef long __jmp_buf[8];

typedef struct  __jmp_buf_tag {
   __jmp_buf __jmpbuf;
   int __mask_was_saved;
   __sigset_t __saved_mask;
} jmp_buf;

void longjmp(jmp_buf env, int val);
int setjmp(jmp_buf env);

jmp_buf jbuf;
/*
int func(jmp_buf buf) {
   buf.__jmpbuf[0] = 3;
   return 0;
}*/

int main() {
   /*
   int value;
   func(jbuf);
   value = setjmp(jbuf);
   if (value == 0) {
      longjmp(jbuf, 1);
   } else {
      printf("Throw, %d\n", value);
      return 0;
   }
   */
   __sigset_t t;
   t.__val[0] = 10;
   t.__val[3] = 40;
   /*
   jbuf.__jmpbuf[1] = 3;
   jbuf.__mask_was_saved = 20;
   jbuf.__saved_mask.__val[0] = 30;
   */
   return 0;
}

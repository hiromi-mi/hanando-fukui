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

int func(__sigset_t buf) {
   printf("%d\n", buf.__val[13]);
   return 0;
}

int main() {
   __sigset_t t;
   t.__val[13] = 2;
   func(t);
   printf("%d\n", t.__val[13]);
   return 0;
}

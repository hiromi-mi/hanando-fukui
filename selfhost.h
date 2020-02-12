// selfhost.h
// This file is required for self-hosting.
#ifdef __HANANDO_FUKUI__
FILE *fopen(char *name, char *type);
void *malloc(int size);
void *realloc(void *ptr, int size);
float strtof(char *nptr, char **endptr);
double strtod(char *nptr, char **endptr);
char *dirname(char *path);
char *basename(char *path);

int isspace(int c);
#endif

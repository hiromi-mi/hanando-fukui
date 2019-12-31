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

#define SEEK_END 2
#define SEEK_SET 0
char *strdup(const char *s);

int isspace(int c);
#endif


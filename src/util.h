#include <stdint.h>

typedef uintptr_t OBJECT_PTR;
typedef int BOOLEAN;

char *convert_to_lower_case(char *);
char *convert_identifier(char *);
char *replace_hyphens(char *);
BOOLEAN is_gensym(OBJECT_PTR);

void sort(char *);
char *substring(const char*, size_t, size_t); 
char *strip_last_colon(char *);

#include <stdint.h>

#define TWO_RAISED_TO_SYMBOL_BITS_MINUS_1 0x3FFFFF

#define BIT_MASK 15

#define SYMBOL_BITS         22
#define OBJECT_SHIFT         4

#define SYMBOL_TAG           1
#define CHARACTER_TAG        2
#define STRING_TAG           3
#define INTEGER_TAG          4
#define FLOAT_TAG            5
#define CONS_TAG             6
#define OBJECT_TAG           7
#define CLASS_OBJECT_TAG     8
#define ARRAY_TAG            9
#define NATIVE_FN_TAG       11
#define CLOSURE_TAG         12
#define TRUE_TAG            13
#define FALSE_TAG           14

typedef int BOOLEAN;

#define true 1
#define false 0

typedef uintptr_t OBJECT_PTR;

//#if __aarch64__
//typedef OBJECT_PTR (*nativefn)();
//#else
typedef OBJECT_PTR (*nativefn)(OBJECT_PTR, ...);
//#endif

typedef struct package
{
  char *name;
  int nof_symbols;
  char ** symbols;
} package_t;

typedef struct binding
{
  OBJECT_PTR key;
  OBJECT_PTR val;
} binding_t;

typedef struct binding_env
{
  unsigned int count;
  binding_t *bindings;
} binding_env_t;

typedef struct
{
  unsigned int nof_elements;
  OBJECT_PTR *elements;
} array_object_t;

typedef struct
{
  OBJECT_PTR class_object;
  unsigned int nof_instance_vars;
  binding_t *instance_vars; //instance var name, value
} object_t;

typedef struct
{
  OBJECT_PTR parent_class_object;
  char *name;
  
  unsigned int nof_instances;
  OBJECT_PTR *instances;
  
  unsigned int nof_instance_vars;
  OBJECT_PTR *inst_vars;
  
  unsigned int nof_shared_vars;
  binding_t *shared_vars;
  
  unsigned int nof_instance_methods;
  binding_t *instance_methods; //method selector and CONS of native function object and free vars 
  
  unsigned int nof_class_methods;
  binding_t *class_methods;  //method selector and CONS of native function object and free vars
  
  //note: if we are going to store the method source too,
  //the val component of the dictionary wiil be a CONS cell with the native funtion object
  //and the method source Lisp object
} class_object_t;

//to avoid passing function pointers
//between manual code and compiler-generated code
typedef struct
{
  nativefn nf;
} native_fn_obj_t;

OBJECT_PTR list(int, ...);
OBJECT_PTR reverse(OBJECT_PTR);
OBJECT_PTR concat(unsigned int, ...);
OBJECT_PTR cons(OBJECT_PTR, OBJECT_PTR);
void print_object(OBJECT_PTR);

void error(const char *, ...);

BOOLEAN IS_SYMBOL_OBJECT(OBJECT_PTR);
BOOLEAN IS_CONS_OBJECT(OBJECT_PTR );
BOOLEAN IS_INTEGER_OBJECT(OBJECT_PTR );
BOOLEAN IS_TRUE_OBJECT(OBJECT_PTR);
BOOLEAN IS_FALSE_OBJECT(OBJECT_PTR);
BOOLEAN IS_CLOSURE_OBJECT(OBJECT_PTR);

int cons_length(OBJECT_PTR);

BOOLEAN is_atom(OBJECT_PTR);

OBJECT_PTR build_symbol_object(int);

OBJECT_PTR car(OBJECT_PTR);
OBJECT_PTR cdr(OBJECT_PTR);
OBJECT_PTR CDDR(OBJECT_PTR);
OBJECT_PTR CADR(OBJECT_PTR);
OBJECT_PTR CAAR(OBJECT_PTR);
OBJECT_PTR CDDDR(OBJECT_PTR);

OBJECT_PTR first(OBJECT_PTR);
OBJECT_PTR second(OBJECT_PTR);
OBJECT_PTR third(OBJECT_PTR);
OBJECT_PTR fourth(OBJECT_PTR);

OBJECT_PTR gensym();
uintptr_t extract_ptr(OBJECT_PTR);
OBJECT_PTR clone_object(OBJECT_PTR);
OBJECT_PTR last_cell(OBJECT_PTR);
void set_heap(uintptr_t, unsigned int, OBJECT_PTR);

int get_int_value(OBJECT_PTR);
int allocate_memory(void **, size_t);

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "gc.h"

#include "util.h"
#include "smalltalk.h"

#define MAX_C_SOURCE_SIZE 524288

void *compile_functions(OBJECT_PTR);
void *compile_functions_from_string(char *);
unsigned int build_fn_prototypes(char *, unsigned int);
unsigned int build_c_string(OBJECT_PTR, char *, BOOLEAN);
unsigned int build_c_fragment(OBJECT_PTR, char *, BOOLEAN, BOOLEAN);
char *extract_variable_string(OBJECT_PTR, BOOLEAN);

OBJECT_PTR CADR(OBJECT_PTR);
OBJECT_PTR third(OBJECT_PTR);
BOOLEAN primop(OBJECT_PTR);

char *get_symbol_name(OBJECT_PTR);
int get_int_value(OBJECT_PTR);

BOOLEAN exists(OBJECT_PTR, OBJECT_PTR);

extern OBJECT_PTR message_selectors;

extern OBJECT_PTR NIL;
extern OBJECT_PTR LET;
extern OBJECT_PTR MESSAGE_SEND;
extern OBJECT_PTR LET1;
extern OBJECT_PTR NTH;
extern OBJECT_PTR SAVE_CONTINUATION;
extern OBJECT_PTR GET_CONTINUATION;
extern OBJECT_PTR SAVE_CONTINUATION_TO_RESUME;
extern OBJECT_PTR CREATE_CLOSURE;
extern OBJECT_PTR EXTRACT_NATIVE_FN;
extern OBJECT_PTR METHOD_LOOKUP;
extern OBJECT_PTR CAR;
extern OBJECT_PTR SETCAR;
extern OBJECT_PTR CONS;

/* input should be the result of lift_transform()) */
void *compile_to_c(OBJECT_PTR input)
{
  OBJECT_PTR lambdas = reverse(cdr(input));

  //TODO: error checking for compile_functions
  
  return compile_functions(lambdas);
}

void *compile_functions(OBJECT_PTR lambda_forms)
{
  char str[MAX_C_SOURCE_SIZE];
  memset(str, '\0', MAX_C_SOURCE_SIZE);

  unsigned int len = 0;

  len += build_fn_prototypes(str+len, len);
  
  OBJECT_PTR rest = lambda_forms;

  while(rest != NIL)
  {
    len += build_c_string(car(rest), str+len, false);
    rest = cdr(rest);
  }

  assert(len <= MAX_C_SOURCE_SIZE);

#ifdef DEBUG
  FILE *out = fopen("debug.c", "a");
  fprintf(out, "%s\n", str);
  fclose(out);
#endif

  return compile_functions_from_string(str);
}

unsigned int build_fn_prototypes(char *buf, unsigned int offset)
{
  unsigned int len = offset;

#ifdef __APPLE__
  len += sprintf(buf+len, "typedef unsigned long uintptr_t;\n");
#else
  len += sprintf(buf+len, "#include <stdint.h>\n");
#endif

//#if __aarch64__
//  len += sprintf(buf+len, "typedef uintptr_t (*nativefn)();\n");
//#else  
  len += sprintf(buf+len, "typedef uintptr_t (*nativefn)(uintptr_t, ...);\n");
//#endif
  
  //len += sprintf(buf+len, "uintptr_t nth(uintptr_t, uintptr_t);\n");
  len += sprintf(buf+len, "uintptr_t nth_closed_val(uintptr_t, uintptr_t);\n");
  len += sprintf(buf+len, "void save_continuation(uintptr_t);\n");
  len += sprintf(buf+len, "void set_most_recent_closure(uintptr_t);\n");
  len += sprintf(buf+len, "nativefn extract_native_fn(uintptr_t);\n");

  len += sprintf(buf+len, "uintptr_t create_closure(uintptr_t, uintptr_t, nativefn, ...);\n");

  //len += sprintf(buf+len, "uintptr_t message_send(uintptr_t, uintptr_t, uintptr_t, ...);\n");  
  //len += sprintf(buf+len, "uintptr_t method_lookup(uintptr_t, uintptr_t);\n");

  len += sprintf(buf+len, "uintptr_t car(uintptr_t);\n");
  len += sprintf(buf+len, "uintptr_t setcar(uintptr_t, uintptr_t);\n");
  len += sprintf(buf+len, "uintptr_t cons(uintptr_t, uintptr_t);\n");
  
  len += sprintf(buf+len, "int in_error_condition();\n");
  len += sprintf(buf+len, "uintptr_t get_continuation(uintptr_t);\n");

  len += sprintf(buf+len, "uintptr_t save_cont_to_resume(uintptr_t);\n");

  len += sprintf(buf+len, "uintptr_t handle_exception();\n");  

  //since integers are not boxed objects (unlike in pLisp),
  //we can pass them directly, there is no need for convert_int_to_object().
  //uncomment this once the compiler has been tested thoroughly.
  len += sprintf(buf+len, "uintptr_t convert_int_to_object(int);\n");

  //len += sprintf(buf+len, "uintptr_t convert_native_fn_to_object(nativefn);\n");

  return len;
}

unsigned int build_c_string(OBJECT_PTR lambda_form, char *buf, BOOLEAN serialize_flag)
{
  char *fname = extract_variable_string(car(lambda_form), serialize_flag);

  unsigned int len = 0;

  len += sprintf(buf+len, "uintptr_t %s(", fname);

  OBJECT_PTR params = second(second(lambda_form));

  OBJECT_PTR rest = params;

  BOOLEAN first_time = true;

  while(rest != NIL)
  {
    char *pname = extract_variable_string(car(rest), serialize_flag);

    if(!first_time)
      len += sprintf(buf+len, ", ");

    len += sprintf(buf+len, "uintptr_t %s", pname);

    rest = cdr(rest);
    first_time = false;

  }

  len += sprintf(buf+len, ")\n{\n");

#ifdef DEBUG
  len += sprintf(buf+len, "printf(\"%s\\n\");\n", fname);
#endif
  
  //uncomment for debugging
  /* rest = params; */

  /* while(rest != NIL) */
  /* { */
  /*   char *pname = extract_variable_string(car(rest), serialize_flag); */

  /*   len += sprintf(buf+len, "printf(\"%%p\\n\", %s);\n", pname); */
    
  /*   rest = cdr(rest); */
  /*   free(pname); */
  /* } */
  //end debugging code

  char *closure_name = extract_variable_string(first(params), serialize_flag);

  len += sprintf(buf+len, "set_most_recent_closure(%s);\n", closure_name);

  len += sprintf(buf+len, "uintptr_t nil = 17;\n");

  OBJECT_PTR body = CDDR(second(lambda_form));

  assert(cons_length(body) == 1 || cons_length(body) == 2);

  if(cons_length(body) == 2)
  {
    len += build_c_fragment(car(body), buf+len, false, serialize_flag);
    len += build_c_fragment(CADR(body), buf+len, false, serialize_flag);
  }
  else
  {
    len += build_c_fragment(car(body), buf+len, false, serialize_flag);
  }

  len += sprintf(buf+len, "\n}\n");

  //uncomment for debugging
  //printf("%s\n", buf);

  return len;
}

unsigned int build_c_fragment(OBJECT_PTR exp, char *buf, BOOLEAN nested_call, BOOLEAN serialize_flag)
{
  unsigned int len = 0;

  BOOLEAN primitive_call = false;

  if(is_atom(exp))
  {
    char *var = extract_variable_string(exp, serialize_flag);
    len += sprintf(buf+len, "%s;\n", var);
  }
  else if(car(exp) == LET || car(exp) == LET1)
  {
    len += sprintf(buf+len, "{\n");

    OBJECT_PTR rest = second(exp);

    while(rest != NIL)
    {
      char *var = extract_variable_string(car(car(rest)), serialize_flag);

      if(IS_CONS_OBJECT(second(car(rest))) && first(second(car(rest))) == EXTRACT_NATIVE_FN)
      {
        len += sprintf(buf+len, "nativefn %s = (nativefn)", var);
        len += build_c_fragment(CADR(car(rest)), buf+len, false, serialize_flag);
      }
      else
      {
        len += sprintf(buf+len, "uintptr_t %s = (uintptr_t)", var);
        len += build_c_fragment(CADR(car(rest)), buf+len, false, serialize_flag);

        //uncomment for debugging
        //len += sprintf(buf+len, "printf(\"%%p\\n\", %s);\n", var);
      }

      rest = cdr(rest);
    }

    if(first(third(exp)) != LET && first(third(exp)) != LET1)
      len += sprintf(buf+len, "return ");

    len += build_c_fragment(third(exp), buf+len, false, serialize_flag);

    len += sprintf(buf+len, "\n}");

  }
  else //primitive or user-defined function application
  {
    if(primop(car(exp)))
      primitive_call = true;

    char *var = extract_variable_string(car(exp), serialize_flag);

    len += sprintf(buf+len, "%s(", var);

    OBJECT_PTR rest = cdr(exp);

    if(primitive_call && rest == NIL)
    {
      len += sprintf(buf+len, "17)");
    }
    else
    {
      BOOLEAN first_time = true;

      int i = 0;
        
      while(rest != NIL)
      {
        if(!first_time)
          len += sprintf(buf+len, ", ");

        if(is_atom(car(rest)))
        {
          char *arg_name = extract_variable_string(car(rest), serialize_flag);
          len += sprintf(buf+len, "%s%s", (i == 2 && !strcmp(var, "create_closure")) ? "(nativefn)" : "", arg_name);
          //if(i == 1 && !strcmp(var, "create_closure"))
          //  len += sprintf(buf+len, "convert_native_fn_to_object((nativefn)%s)", arg_name);
          //else
          //  len += sprintf(buf+len, "%s", arg_name);
        }
        else
          len += build_c_fragment(car(rest), buf+len, true, serialize_flag);

        rest = cdr(rest);
        first_time = false;
        i++;
      }

      len += sprintf(buf+len, ")");
    }

    if(!nested_call)
    {
      len += sprintf(buf+len, ";\n");
      if(primitive_call || car(exp) == EXTRACT_NATIVE_FN)
        len += sprintf(buf+len, "if(in_error_condition()==1)return handle_exception();\n");
    }
  }

  return len;
}

char *extract_variable_string(OBJECT_PTR var, BOOLEAN serialize_flag)
{
  assert(is_atom(var));

  if(IS_SYMBOL_OBJECT(var))
  {
    char *raw_name = GC_strdup(get_symbol_name(var));

    if(primop(var))
    {
      char *s = (char *)GC_MALLOC(40*sizeof(char));
      //if(var == MESSAGE_SEND)
      //  sprintf(s,"message_send");
      //if(var == METHOD_LOOKUP)
      //  sprintf(s,"method_lookup");
      /* else*/ if(var == SAVE_CONTINUATION)
        sprintf(s, "save_continuation");
      else if(var == GET_CONTINUATION)
        sprintf(s, "get_continuation");
      else if(var == SAVE_CONTINUATION_TO_RESUME)
        sprintf(s, "save_cont_to_resume");
      else if(var == CAR)
        sprintf(s, "car");
      else if(var == SETCAR)
        sprintf(s, "setcar");
      else if(var == CONS)
        sprintf(s, "cons");
      else
      {
        print_object(var);
        assert(false);
      }

      return s;
    }
    else if(var != EXTRACT_NATIVE_FN && 
            var != NTH && 
            var != CREATE_CLOSURE &&
            var != NIL &&
            var != SAVE_CONTINUATION_TO_RESUME)
    {
      //symbols that are passed to message_send() should be passed as raw object pointers
      if(exists(var, message_selectors)) 
      {
        char *ret = (char *)GC_MALLOC(20 * sizeof(char));
        memset(ret, '\0', 20);
        sprintf(ret, "%lu", var);
        return ret;
      }
      else
        return convert_to_lower_case(convert_identifier(get_symbol_name(var)));
    }
    else
      return convert_to_lower_case(replace_hyphens(raw_name));
  }
  else
  {
      char *s = (char *)GC_MALLOC(50*sizeof(char));
      memset(s,'\0',50);

      if(IS_INTEGER_OBJECT(var))
        //since integers are not boxed objects (unlike in pLisp),
        //we can pass them directly. change this once the
        //compiler has been tested thoroughly.
        //sprintf(s, "%lu", var);
        sprintf(s, "convert_int_to_object(%d)", get_int_value(var));
      //else if(IS_FLOAT_OBJECT(var)) //TODO - when we come to floats
      //  sprintf(s, "conv_float_to_obj_for_fm(%lld)", wrap_float(var));
      else
#if __x86_64__
        sprintf(s, "%lld", (long long)var);
#elif __aarch64__
        sprintf(s, "%lld", (long long)var);      
#else
        sprintf(s, "%d", (int)var);
#endif

      return s;
  }
}

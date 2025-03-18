/**
  Copyright 2025 Rajesh Jayaprakash <rajesh.jayaprakash@pm.me>

  This file is part of <smalltalk>.

  <smalltalk> is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  <smalltalk> is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with <smalltalk>.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef GLOBAL_DECLS_H
#define GLOBAL_DECLS_H

#include "smalltalk.h"
#include "parser_header.h"

//workaround for variadic function arguments
//getting clobbered in ARM64
typedef OBJECT_PTR (*nativefn1)(OBJECT_PTR, OBJECT_PTR);

OBJECT_PTR           add_class_method(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           add_instance_method(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
int                  add_symbol(char *);
char                *append_char(char *, char);
OBJECT_PTR           apply_lisp_transforms(OBJECT_PTR);
OBJECT_PTR           CADR(OBJECT_PTR);
OBJECT_PTR           clone_object(OBJECT_PTR);
void                *compile_to_c(OBJECT_PTR);
OBJECT_PTR           cons(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           convert_array_object_to_object_ptr(array_object_t *);
OBJECT_PTR           convert_char_to_object(char);
OBJECT_PTR           convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR           convert_exec_code_to_lisp(executable_code_t *);
OBJECT_PTR           convert_fn_to_closure(nativefn);
OBJECT_PTR           convert_int_to_object(int);
OBJECT_PTR           convert_native_fn_to_object(nativefn);
OBJECT_PTR           create_and_signal_exception(OBJECT_PTR, OBJECT_PTR);
call_chain_entry_t  *create_call_chain_entry(OBJECT_PTR,
					    OBJECT_PTR,
					    OBJECT_PTR,
					    unsigned int,
					    OBJECT_PTR *,
					    OBJECT_PTR,
					    OBJECT_PTR,
					    BOOLEAN);
OBJECT_PTR           create_closure(OBJECT_PTR, OBJECT_PTR, nativefn, ...);
exception_handler_t *create_exception_handler(OBJECT_PTR,
					      OBJECT_PTR,
					      OBJECT_PTR,
					      stack_type *,
					      OBJECT_PTR);
OBJECT_PTR           decorate_message_selectors(OBJECT_PTR);
BOOLEAN              exists(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           extract_arity(OBJECT_PTR);
nativefn             extract_native_fn(OBJECT_PTR);
uintptr_t            extract_ptr(OBJECT_PTR);
int                  extract_symbol_index(OBJECT_PTR);
char                *extract_variable_string(OBJECT_PTR, BOOLEAN);
OBJECT_PTR           fifth(OBJECT_PTR);

OBJECT_PTR           get_binding(binding_env_t *, OBJECT_PTR);
OBJECT_PTR           get_binding_val(binding_env_t *, OBJECT_PTR);
BOOLEAN              get_binding_val_regular(binding_env_t *, OBJECT_PTR, OBJECT_PTR *);
OBJECT_PTR           get_class_object(OBJECT_PTR);
nativefn             get_function(void *, const char *);
int                  get_int_value(OBJECT_PTR);
OBJECT_PTR           get_parent_class(OBJECT_PTR);
OBJECT_PTR           get_smalltalk_symbol(char *);
char                *get_smalltalk_symbol_name(OBJECT_PTR);
OBJECT_PTR           get_string_obj(char *);
OBJECT_PTR           get_symbol(char *);
char                *get_symbol_name(OBJECT_PTR);
BOOLEAN              get_top_level_val(OBJECT_PTR, OBJECT_PTR *);
OBJECT_PTR           identity_function(OBJECT_PTR, ...);
OBJECT_PTR           initialize_object();
void                 invoke_curtailed_blocks();
BOOLEAN              IS_ARRAY_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CHARACTER_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CLASS_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CLOSURE_OBJECT(OBJECT_PTR);
BOOLEAN              IS_FALSE_OBJECT(OBJECT_PTR);
BOOLEAN              IS_NATIVE_FN_OBJECT(OBJECT_PTR);
BOOLEAN              IS_OBJECT_OBJECT(OBJECT_PTR);
BOOLEAN              IS_SMALLTALK_SYMBOL_OBJECT(OBJECT_PTR);
BOOLEAN              IS_STRING_LITERAL_OBJECT(OBJECT_PTR);
BOOLEAN              IS_STRING_OBJECT(OBJECT_PTR);
BOOLEAN              is_super_class(OBJECT_PTR, OBJECT_PTR);
BOOLEAN              IS_TRUE_OBJECT(OBJECT_PTR);
call_chain_entry_t  *is_termination_block_not_invoked(OBJECT_PTR);
OBJECT_PTR           last_cell(OBJECT_PTR);
OBJECT_PTR           message_send(OBJECT_PTR,
				  OBJECT_PTR,
				  OBJECT_PTR,
				  OBJECT_PTR,
				  ...);

OBJECT_PTR           message_send_super(OBJECT_PTR,
					OBJECT_PTR,
					OBJECT_PTR,
					OBJECT_PTR,
					...);
OBJECT_PTR           new_object(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           new_object_internal(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           niladic_block_value(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           nth(OBJECT_PTR, OBJECT_PTR);
uintptr_t            object_alloc(int, int);
BOOLEAN              primop(OBJECT_PTR);
void                 print_call_chain();
void                 print_exception_contexts();
void                 print_object(OBJECT_PTR);
void                 put_binding_val(binding_env_t *, OBJECT_PTR, OBJECT_PTR);
void                 repl();
OBJECT_PTR           replace_method_selector(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           reverse(OBJECT_PTR);
OBJECT_PTR           setcdr(OBJECT_PTR, OBJECT_PTR);
void                 set_heap(uintptr_t, unsigned int, OBJECT_PTR);
OBJECT_PTR           seventh(OBJECT_PTR);
OBJECT_PTR           signal_exception(OBJECT_PTR);
OBJECT_PTR           sixth(OBJECT_PTR);
OBJECT_PTR           third(OBJECT_PTR);
void                 update_binding(binding_env_t *, OBJECT_PTR, OBJECT_PTR);
#endif

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

OBJECT_PTR           add_class_method(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR, OBJECT_PTR, executable_code_t *);
OBJECT_PTR           add_instance_method(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR, OBJECT_PTR, executable_code_t *);
void                 add_native_fn_source(unsigned int, char *, nativefn, char *);
int                  add_symbol(char *);
char                *append_char(char *, char);
OBJECT_PTR           apply_lisp_transforms(OBJECT_PTR);
OBJECT_PTR           assignment_conversion(OBJECT_PTR, OBJECT_PTR);
unsigned int         build_c_string(OBJECT_PTR, char *, BOOLEAN);
OBJECT_PTR           build_selectors_list(OBJECT_PTR);
OBJECT_PTR           CADR(OBJECT_PTR);
BOOLEAN              call_chain_entry_exists(call_chain_entry_t *);
OBJECT_PTR           capture_local_variables(OBJECT_PTR);
OBJECT_PTR           capture_local_var_names(OBJECT_PTR);
OBJECT_PTR           clone_object(OBJECT_PTR);
OBJECT_PTR           closure_conv_transform(OBJECT_PTR);
void                *compile_functions(OBJECT_PTR);
OBJECT_PTR           cons(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           convert_array_object_to_object_ptr(array_object_t *);
OBJECT_PTR           convert_char_to_object(char);
OBJECT_PTR           convert_class_object_to_object_ptr(class_object_t *);
OBJECT_PTR           convert_exec_code_to_lisp(executable_code_t *);
OBJECT_PTR           convert_float_to_object(double);
OBJECT_PTR           convert_fn_to_closure(nativefn);
OBJECT_PTR           convert_int_to_object(int);
OBJECT_PTR           convert_message_sends(OBJECT_PTR);
OBJECT_PTR           convert_native_fn_to_object(nativefn);
OBJECT_PTR           create_and_signal_exception(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           create_and_signal_exception_with_text(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
binding_env_t       *create_binding_env();
call_chain_entry_t  *create_call_chain_entry(OBJECT_PTR,
					     BOOLEAN,
					     OBJECT_PTR,
					     OBJECT_PTR,
					     OBJECT_PTR,
					     OBJECT_PTR,
					     unsigned int,
					     OBJECT_PTR *,
					     OBJECT_PTR,
					     OBJECT_PTR,
					     BOOLEAN);
OBJECT_PTR           create_closure(OBJECT_PTR, OBJECT_PTR, nativefn, ...);
void                 create_image(char *);
exception_handler_t *create_exception_handler(OBJECT_PTR,
					      OBJECT_PTR,
					      OBJECT_PTR,
					      stack_type *,
					      OBJECT_PTR);
OBJECT_PTR           create_method(class_object_t *,
				   BOOLEAN,
				   OBJECT_PTR,
				   OBJECT_PTR,
				   OBJECT_PTR,
				   unsigned int,
				   OBJECT_PTR,
				   executable_code_t *);
OBJECT_PTR           decorate_message_selectors(OBJECT_PTR);
OBJECT_PTR           desugar_il(OBJECT_PTR);
BOOLEAN              exists(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           expand_bodies(OBJECT_PTR);
OBJECT_PTR           extract_arity(OBJECT_PTR);
nativefn             extract_native_fn(OBJECT_PTR);
uintptr_t            extract_ptr(OBJECT_PTR);
int                  extract_symbol_index(OBJECT_PTR);
char                *extract_variable_string(OBJECT_PTR, BOOLEAN);
OBJECT_PTR           fifth(OBJECT_PTR);
unsigned int         file_exists(char *);

OBJECT_PTR           get_binding(binding_env_t *, OBJECT_PTR);
OBJECT_PTR           get_binding_val(binding_env_t *, OBJECT_PTR);
BOOLEAN              get_binding_val_regular(binding_env_t *, OBJECT_PTR, OBJECT_PTR *);
char                 get_char_value(OBJECT_PTR);
OBJECT_PTR           get_class_object(OBJECT_PTR);
char                *get_file_contents(char *);
double               get_float_value(OBJECT_PTR);
OBJECT_PTR           get_free_variables(OBJECT_PTR);
nativefn             get_function(void *, const char *);
int                  get_int_value(OBJECT_PTR);
OBJECT_PTR           get_parent_class(OBJECT_PTR);
OBJECT_PTR           get_smalltalk_symbol(char *);
char                *get_smalltalk_symbol_name(OBJECT_PTR);
OBJECT_PTR           get_string_obj(char *);
OBJECT_PTR           get_symbol(char *);
char                *get_symbol_name(OBJECT_PTR);
OBJECT_PTR           get_top_level_symbols();
BOOLEAN              get_top_level_val(OBJECT_PTR, OBJECT_PTR *);
OBJECT_PTR           identity_function(OBJECT_PTR, ...);
OBJECT_PTR           initialize_object(OBJECT_PTR);
OBJECT_PTR           invoke_cont_on_val(OBJECT_PTR, OBJECT_PTR);
void                 invoke_curtailed_blocks(OBJECT_PTR);
BOOLEAN              IS_ARRAY_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CHARACTER_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CLASS_OBJECT(OBJECT_PTR);
BOOLEAN              IS_CLOSURE_OBJECT(OBJECT_PTR);
BOOLEAN              IS_FALSE_OBJECT(OBJECT_PTR);
BOOLEAN              IS_FLOAT_OBJECT(OBJECT_PTR);
BOOLEAN              IS_NATIVE_FN_OBJECT(OBJECT_PTR);
BOOLEAN              IS_OBJECT_OBJECT(OBJECT_PTR);
BOOLEAN              IS_SMALLTALK_SYMBOL_OBJECT(OBJECT_PTR);
BOOLEAN              IS_STRING_LITERAL_OBJECT(OBJECT_PTR);
BOOLEAN              IS_STRING_OBJECT(OBJECT_PTR);
BOOLEAN              is_super_class(OBJECT_PTR, OBJECT_PTR);
BOOLEAN              IS_TRUE_OBJECT(OBJECT_PTR);
call_chain_entry_t  *is_termination_block_not_invoked(OBJECT_PTR);
OBJECT_PTR           last_cell(OBJECT_PTR);
OBJECT_PTR           lift_transform(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           mcps_transform(OBJECT_PTR);
OBJECT_PTR           message_send(OBJECT_PTR,
				  OBJECT_PTR,
				  OBJECT_PTR,
				  OBJECT_PTR,
				  OBJECT_PTR,
				  ...);
OBJECT_PTR           message_send_internal(BOOLEAN,
					   OBJECT_PTR,
					   OBJECT_PTR,
					   OBJECT_PTR,
					   OBJECT_PTR,
					   OBJECT_PTR *);

OBJECT_PTR           message_send_super(OBJECT_PTR,
					OBJECT_PTR,
					OBJECT_PTR,
					OBJECT_PTR,
					OBJECT_PTR,
					...);
OBJECT_PTR           new_object(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           new_object_internal(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           niladic_block_value(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           nth(OBJECT_PTR, OBJECT_PTR);
uintptr_t            object_alloc(int, int);
void                 parse_from_fp(FILE *);
BOOLEAN              pop_if_top(call_chain_entry_t *);
char                *prepend_char(char *, char);
BOOLEAN              primop(OBJECT_PTR);
void                 print_call_chain();
void                 print_exception_contexts();
void                 print_object(OBJECT_PTR);
void                 print_object_to_string(OBJECT_PTR, char *);
void                 put_binding_val(binding_env_t *, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           ren_transform(OBJECT_PTR, binding_env_t *);
void                 repl();
OBJECT_PTR           repl_common();
OBJECT_PTR           replace_method_selector(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           reverse(OBJECT_PTR);
OBJECT_PTR           setcdr(OBJECT_PTR, OBJECT_PTR);
void                 set_heap(uintptr_t, unsigned int, OBJECT_PTR);
OBJECT_PTR           seventh(OBJECT_PTR);
void                 show_debug_window(BOOLEAN, OBJECT_PTR);
OBJECT_PTR           signal_exception(OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           signal_exception_with_text(OBJECT_PTR, OBJECT_PTR, OBJECT_PTR);
OBJECT_PTR           simplify_il(OBJECT_PTR);
OBJECT_PTR           sixth(OBJECT_PTR);
char                *substring(const char*, size_t, size_t);
OBJECT_PTR           third(OBJECT_PTR);
OBJECT_PTR           translate_to_il(OBJECT_PTR);
void                 update_binding(binding_env_t *, OBJECT_PTR, OBJECT_PTR);
int                  yyparse();
int                  yy_scan_string(char *);

#endif

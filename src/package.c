#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gc.h"

#include "stack.h"

#ifdef PACKAGE_TEST

typedef int BOOLEAN;

#define true 1
#define false 0

typedef struct smalltalk_package
{
  char *name;
  struct smalltalk_package *parent;
  unsigned int nof_children;
  struct smalltalk_package **children;
} smalltalk_package_t;
#else
#include "smalltalk.h"
#include "util.h"
#endif

unsigned int g_nof_smalltalk_packages;
smalltalk_package_t **g_smalltalk_packages = NULL;

#ifdef PACKAGE_TEST
char *substring(const char* str, size_t begin, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
    return 0;

  return GC_strndup(str + begin, len);
}
#endif

void add_child(smalltalk_package_t *parent, smalltalk_package_t *child)
{
  parent->nof_children++;

  if(!parent->children)
    parent->children = (smalltalk_package_t **)GC_MALLOC(parent->nof_children * sizeof(smalltalk_package_t *));
  else
  {
    smalltalk_package_t **temp = (smalltalk_package_t **)GC_REALLOC(parent->children,
                                                                    parent->nof_children * sizeof(smalltalk_package_t *));
    assert(temp);
    parent->children = temp;
  }

  parent->children[parent->nof_children-1] = child;
}

smalltalk_package_t *get_package_with_parent(char *name, smalltalk_package_t *parent)
{
  unsigned int i;

  for(i=0; i<g_nof_smalltalk_packages; i++)
  {
    if(!strcmp(name, g_smalltalk_packages[i]->name) &&
       g_smalltalk_packages[i]->parent == parent)
      return g_smalltalk_packages[i];
  }

  smalltalk_package_t *new_pkg = (smalltalk_package_t *)GC_MALLOC(sizeof(smalltalk_package_t));
  new_pkg->name = GC_strndup(name, strlen(name));
  new_pkg->nof_children = 0;
  new_pkg->children = NULL;

  if(parent)
  {
    new_pkg->parent = parent;
    add_child(parent, new_pkg);
  }
  else
    new_pkg->parent = NULL;

  g_nof_smalltalk_packages++;

  if(!g_smalltalk_packages)
    g_smalltalk_packages = (smalltalk_package_t **)GC_MALLOC(g_nof_smalltalk_packages * sizeof(smalltalk_package_t *));
  else
  {
    smalltalk_package_t **temp = (smalltalk_package_t **)GC_REALLOC(g_smalltalk_packages,
                                                                    g_nof_smalltalk_packages * sizeof(smalltalk_package_t *));
    assert(temp);
    g_smalltalk_packages = temp;
  }

  g_smalltalk_packages[g_nof_smalltalk_packages-1] = new_pkg;

  return new_pkg;
}

smalltalk_package_t *get_root_package(char *name)
{
  return get_package_with_parent(name, NULL);
}

smalltalk_package_t *get_package(char *name)
{
  unsigned int last_occ, i, j, len;
  char *s1, *s2;
  smalltalk_package_t *parent;

  i = 0;
  len = strlen(name);

  //TODO: more robust validation of package name string
  if(!len)
    return NULL;

  last_occ = len;

  for(j=0; j<len; j++)
  {
    if(name[j] == '.')
      last_occ = i+1;
    i++;
  }

  if(last_occ == len)
    return get_root_package(name);
  else
  {
    //if name is "core.utils.collection",
    //s1 will be "core.utils" and s2
    //will be "collection"
    s1 = substring(name, 0, last_occ-1);
    s2 = substring(name, last_occ, len - last_occ);

    parent = get_package(s1);

    return get_package_with_parent(s2, parent);
  }
}

char *get_qualified_name(smalltalk_package_t *pkg)
{
  int count=0, len=0;
  
  stack_type *stack = stack_create();

  stack_push(stack, (void *)pkg->name);
  count++;
  
  len += strlen(pkg->name);

  smalltalk_package_t *parent = pkg->parent;

  while(parent)
  {
    stack_push(stack, parent->name);
    
    count++;
    len += strlen(parent->name);
    
    parent = parent->parent;
  }

  //we allocate space for the names, the 'count-1' intervening dots, and a null terminator
  char *ret = (char *)GC_MALLOC( (len + count) * sizeof(char));
  memset(ret, '\0', len + count);
  int len1 = 0;

  BOOLEAN first_time = true;
  
  while(!stack_is_empty(stack))
  {
    char *name = (char *)stack_pop(stack);
    len1 += sprintf(ret+len1, "%s%s", first_time ? "" : ".", name);
    if(first_time)
      first_time = false;
  }

  return ret;
}

void print_package(smalltalk_package_t *pkg)
{
  printf("-----------------------------\n");
  printf("name = %s\n", pkg->name);
  printf("parent = %s\n", pkg->parent ? pkg->parent->name : "NONE");
  printf("children:\n");
  if(pkg->nof_children > 0)
  {
    unsigned int i;
    for(i=0; i<pkg->nof_children; i++)
    {
      printf("%s", pkg->children[i]->name);
      if(i != pkg->nof_children - 1)
        printf(", ");
      else
        printf("\n");
    }
  }
  else
    printf("NONE\n");
  printf("-----------------------------\n");
}

#ifdef PACKAGE_TEST
int main(int argc, char **argv)
{
  g_nof_smalltalk_packages = 0;
  g_smalltalk_packages = NULL;

  unsigned int i;

  for(i=1; i<argc; i++)
    get_package(argv[i]);

  printf("No of packages = %d\n", g_nof_smalltalk_packages);

  for(i=0; i<g_nof_smalltalk_packages; i++)
    //print_package(g_smalltalk_packages[i]);
    printf("%s\n", get_qualified_name(g_smalltalk_packages[i]));

  return 0;
}
#endif

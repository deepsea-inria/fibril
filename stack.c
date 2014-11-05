#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include "safe.h"
#include "util.h"
#include "debug.h"
#include "shmap.h"
#include "stack.h"
#include "config.h"

void *  STACK_ADDR;
void *  STACK_BOTTOM;
int  *  STACK_FILES;
void ** STACK_ADDRS;
intptr_t * STACK_OFFSETS;

static void find_stack(void ** addr, size_t * size)
{
  FILE * maps;
  SAFE_RETURN(maps, fopen("/proc/self/maps", "r"));

  void * start;
  void * end;
  char path[FILENAME_LIMIT];

  while (EOF != fscanf(maps,
        "%p-%p %*s %*x %*d:%*d %*d %[[/]%s%*[ (deleted)]\n",
        &start, &end, path, path + 1)) {
    if (strstr(path, "[stack]")) {
      *addr = start;
      *size = end - start;

      DEBUG_PRINTI("stack: addr=%p, size=%ld\n", *addr, *size);
      break;
    }
  }

  SAFE_FNCALL(fclose(maps));
}

void stack_init(int nprocs)
{
  size_t size;
  find_stack(&STACK_ADDR, &size);

  STACK_BOTTOM = STACK_ADDR + size;

  STACK_FILES = malloc(sizeof(int) * nprocs);
  STACK_OFFSETS = malloc(sizeof(intptr_t) * nprocs);

  void * stack = malloc(size) + size;
  STACK_EXECUTE(stack,
      STACK_FILES[0] = shmap_copy(STACK_ADDR, size, "stack_0")
  );
  free(stack - size);

  STACK_OFFSETS[0] = STACK_ADDR - shmap_mmap(NULL, size, STACK_FILES[0]);

  char path[FILENAME_LIMIT];
  int i;

  for (i = 1; i < nprocs; ++i) {
    sprintf(path, "%s_%d", "stack", i);

    int file = shmap_open(size, path);
    STACK_FILES[i] = file;

    void * addr = shmap_mmap(NULL, size, file);
    STACK_OFFSETS[i] = STACK_ADDR - addr;
  }

  /** Allocate space for scheduler stacks. */
  STACK_ADDRS = malloc(sizeof(void * [nprocs]));

  for (i = 0; i < nprocs; ++i) {
    STACK_ADDRS[i] = malloc(size) + size;
  }
}

void stack_init_child(int id)
{
  SAFE_ASSERT(id != 0);
  shmap_mmap(STACK_ADDR, STACK_BOTTOM - STACK_ADDR, STACK_FILES[id]);
}

void stack_finalize(int nprocs)
{
  free(STACK_FILES);
  free(STACK_OFFSETS);

  int i;
  for (i = 0; i < nprocs; ++i) {
    free(STACK_ADDRS[i] - (STACK_BOTTOM - STACK_ADDR));
  }

  free(STACK_ADDRS);
}


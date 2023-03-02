#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "bunnyhop.h"
#include <assemblyline.h>

#define LOW_MASK 0x7FFFFFFFF000
#define PAGELEN 4096
#define PAGEMASK 0xFFF

void free_buf(void *buffer, uint64_t len); 

bhfunc gentrain(uint64_t adrs, int size, int branch_distance)
{
  void *buffer = create_buffer(adrs, size);
  assemblyline_t al = asm_create_instance(buffer + (adrs & PAGEMASK), size);

  char str_distance[15];
  sprintf(str_distance, "%d", branch_distance);
  char str_jmp[20] = "JMP ";
  strcat(str_jmp, str_distance);
  asm_assemble_str(al, str_jmp);
  for (int i = 0; i < branch_distance; i++)
    asm_assemble_str(al, "NOP");
  asm_assemble_str(al, "RET");

  void (*func)() = asm_get_code(al);
  asm_destroy_instance(al);
  return func;
}

bhfunc genspy(uint64_t adrs, int size, int branch_distance)
{
  void *buffer = create_buffer(adrs, size);
  assemblyline_t al = asm_create_instance(buffer + (adrs & PAGEMASK), size);

  for (int i = 0; i < branch_distance; i++)
    asm_assemble_str(al, "NOP");
  asm_assemble_str(al, "RET");

  void (*func)() = asm_get_code(al);
  asm_destroy_instance(al);
  return func;
}


void *create_buffer(uint64_t adrs, int size)
{
  void * buffer = (void *)mmap ( (void*)(adrs & LOW_MASK), size,
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_POPULATE,
			     -1, 0 );
  if (buffer == MAP_FAILED) {
    printf ("Fail to allocate memory\n");
  } 
  return buffer;
}

void bhfree(bhfunc f, int size) {
  free_buf(f, size);
}

void free_buf(void *buffer, uint64_t len) {
  uintptr_t start = (uintptr_t)buffer;
  uint64_t base = start & PAGEMASK;
  uint64_t length = ((start + len - base) + PAGELEN - 1) & PAGEMASK;
  munmap((void *)base, length);
}


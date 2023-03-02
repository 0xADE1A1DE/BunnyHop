#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../../src/bunnyhop.h"


#include <mastik/low.h>
#include <mastik/util.h>
#include <assemblyline.h>

#define INFO 0
#define SAMPLES 100
#define THRESHOLD 80 // The cache miss threshold is various from processors
#define PAGEMASK 0xFFF

#define FLIP_MASK 1ULL << 33 
#define PAGE 4096
#define memory_barrier asm volatile ("sfence;\nmfence;\nlfence");

/* Change me. */
int chained_branches = 30;
int nop_in_probe = 150;

// Generate a function that contains eight continus JMP 128
bhfunc genjmps(uint64_t adrs, int size);

int main(int argc, char* argv[])
{
  int depth = atoi(argv[1]);
  uint64_t overall = 0;

  srand(time(NULL));
  
  uint64_t rand_adrs = ((uint64_t)random() << 32) | random();
  rand_adrs = rand_adrs & (~0xFFFULL);
  
  bhfunc branch_probe = genspy(rand_adrs, PAGE << 1, nop_in_probe);
  bhfunc branch_train = genjmps(rand_adrs ^ FLIP_MASK, PAGE << 2);
  uint64_t monitor_adrs = (uint64_t)branch_probe + depth * 64; 

  for (int i = 0; i < SAMPLES; i++) {
    memory_barrier
    
    // Write the branch (RET, JMPs) into BTB
    memory_barrier
    branch_train();

    memory_barrier
    clflush((void*)monitor_adrs);
    memory_barrier

    // Trigger instruction prefetcher
    branch_probe();
    delayloop(100000);
    memory_barrier

    //Test adrs
    uint32_t res = memaccesstime((void*)monitor_adrs);
    overall += res < 120 ? res : 120;
  }

  bhfree(branch_probe, PAGE << 1);
  bhfree(branch_train, PAGE << 2);
  memory_barrier

#ifdef THRESHOLD
  printf("Test Depth %2d, %s\n", depth,  (overall / SAMPLES) < THRESHOLD ? "cached" : "not cached");
#else
  printf("Test Depth %2d, %3ld\n", depth,  overall / SAMPLES);
#endif
}

bhfunc genjmps(uint64_t adrs, int size)
{
  void *buffer = create_buffer(adrs, size);
  assemblyline_t al = asm_create_instance(buffer + (adrs & PAGEMASK), size);

  for (int num = 0; num < chained_branches; num++) {
    asm_assemble_str(al, "JMP 128");
    for (int i = 0; i < 128; i++)
      asm_assemble_str(al, "NOP");
  }

  asm_assemble_str(al, "RET");

  void (*func)() = asm_get_code(al);
  asm_destroy_instance(al);
  return func;
}

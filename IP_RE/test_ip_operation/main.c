#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../../src/bunnyhop.h"


#include <mastik/low.h>
#include <mastik/util.h>

#define INFO 0
#define SAMPLES 100
#define THRESHOLD 80 // The cache miss threshold is various from processors

#define FLIP_MASK 1ULL << 33
#define PAGE 4096
#define memory_barrier asm volatile ("sfence;\nmfence;\nlfence");


int main(int argc, char* argv[])
{
  int depth = atoi(argv[1]);
  int num_nops = atoi(argv[2]);
  uint64_t overall = 0;

  srand(time(NULL));

  for (int i = 0; i < SAMPLES; i++) {
    // Every time, generate a new function to avoid impacts from BTB content
    uint64_t rand_adrs = ((uint64_t)random() << 32) | random();
    rand_adrs &= (~0xFFFULL);
    bhfunc op_train = genspy(rand_adrs, PAGE << 1, 0);
    bhfunc op_probe = genspy(rand_adrs ^ FLIP_MASK, PAGE << 1, num_nops); 
    uint64_t monitor_adrs = (uint64_t)op_probe + depth * 64; 
    memory_barrier
    
    // Write the branch (RET) into BTB
    memory_barrier
    op_train();

    memory_barrier
    clflush((void*)monitor_adrs);
    memory_barrier

    // Trigger instruction prefetcher
    op_probe();
    delayloop(10000);

    //Test adrs
    uint32_t res = memaccesstime((void*)monitor_adrs);
    overall += res < 120 ? res : 120;
    memory_barrier
    bhfree(op_train, PAGE << 1);
    bhfree(op_probe, PAGE << 1);
  }

  printf("%ld\n", overall / SAMPLES);
}

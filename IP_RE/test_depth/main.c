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

#define FLIP_MASK 1ULL << 30
#define PAGE 4096
#define memory_barrier asm volatile ("sfence;\nmfence;\nlfence");
int main(int argc, char* argv[])
{
  int depth = atoi(argv[1]);
  int train = atoi(argv[2]);
  uint64_t overall = 0;

  srand(time(NULL));

  for (int i = 0; i < SAMPLES; i++) {
    // Every time, generate a new function to avoid impacts from BTB content
    uint64_t rand_adrs = ((uint64_t)random() << 32) | random();
    bhfunc branch_probe = genspy(rand_adrs, PAGE * 2, 0);
    uint64_t monitor_adrs = (uint64_t)branch_probe + depth * 64; 
    memory_barrier
    
    // Write the branch (RET) into BTB
    if (train) {
      memory_barrier
      branch_probe();
    }

    memory_barrier
    clflush((void*)monitor_adrs);
    memory_barrier

    // Trigger instruction prefetcher
    branch_probe();

    //Test adrs
    uint32_t res = memaccesstime((void*)monitor_adrs);
    overall += res < 120 ? res : 120;
    bhfree(branch_probe, PAGE << 1);
    memory_barrier
  }

#ifndef THRESHOLD
  printf("Test Depth %2d, BTB is %s, %3ld\n", 
	depth, train ? "trained" : "not trained", overall / SAMPLES);
#else
  printf("Test Depth %2d, BTB is %s, test_block is %s\n", 
	depth, train ? "trained" : "not trained", 
	(overall / SAMPLES) < THRESHOLD ? "cached" : "not cached");
#endif
}

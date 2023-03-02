#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../src/bunnyhop.h"


#include <mastik/low.h>
#include <mastik/util.h>

#define FLIP_MASK 1ULL << 30
#define PAGE 4096
#define memory_barrier asm volatile ("sfence;\nmfence;\nlfence");
int main(int argc, char* argv[])
{
  int test_block = atoi(argv[1]);

  srand(time(NULL));
  uint64_t rand_adrs = ((uint64_t)random() << 32) | random();

  
  bhfunc branch_probe = genspy(rand_adrs, PAGE * 2, 0);
  uint64_t monitor_adrs = (uint64_t)branch_probe + test_block * 64; 
  memory_barrier

  //flush adrs
  clflush((void*)monitor_adrs);
  memory_barrier


  // Trigger instruction prefetcher
  branch_probe();



  //Test adrs
  uint32_t res = memaccesstime((void*)monitor_adrs);
  printf("%d\n", res);

  bhfree(branch_probe, PAGE << 1);
}

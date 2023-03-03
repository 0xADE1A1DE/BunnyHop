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

#define memory_barrier asm volatile (".rept 1;\nsfence;\nmfence;\nlfence\n.endr");
#define HIGH_MASK 1ULL << 45
#define LOW_MASK  0x7FFFFFFFF000


uint64_t global_variable __attribute__((aligned(4096))) = 0xff;
extern void __attribute__((aligned(4096))) access_variable(uint64_t adrs);
void __attribute__((aligned(4096))) delayloop(uint64_t cycles);
int dummy;

void *create_buffer(uint64_t adrs, int size);
void *create_victim(uint64_t btb_set, int size);

int main(int argc, char *argv[])
{   
    uint64_t overall = 0;
    uint64_t secret_input = atoi(argv[1]);
    
    uint64_t btb_set = ((uint64_t)(0x5555555f0680 + 0x437) >> 5) & 0x1FF;
    void (*victim)() = create_victim(btb_set, 4096 * 2);
    int secret[1024] = {0};
   
    for (int i = 0; i < 8; i++)
      secret[i] = (secret_input >> i) & 1;
#if 0
    for (int i = 0; i < 1024; i++) {
#if 0
      secret[i] = i % 2;
#else
      if ((i%5) == 0)      secret[i] = 1;
      else if ((i%5) == 1) secret[i] = 0;
      else if ((i%5) == 2) secret[i] = 1;
      else if ((i%5) == 3) secret[i] = 1;
      else if ((i%5) == 4) secret[i] = 0;
#endif
    }
#endif
    victim(1);
    victim(1);
    victim(1);
    victim(1);

    // Attacker flush access_variable 
    for (int i = 0; i < 8; i++) {
        delayloop(0x1000);
        memory_barrier
        
        access_variable((uint64_t)&global_variable);
        memory_barrier
        delayloop(2000);
        victim(secret[i]);
        //delayloop(1000);
        memory_barrier
    }
    memory_barrier
}

void access_variable(uint64_t adrs)
{
    asm volatile("mov (%0), %%eax":: "r"((void *)adrs));
}

void delayloop(uint64_t cycles)
{
    uint64_t start = __rdtscp(&dummy);
    while ((__rdtscp(&dummy) - start) < cycles)
	    asm volatile ("" :::"memory");
}

void *create_victim(uint64_t btb_set, int size)
{
    int i;
    uint64_t adrs = (((uint64_t)rand() << 32) | rand()) & ~(0x1FFULL << 5) | (btb_set << 5);
    void *buffer = create_buffer(adrs, size);

    assemblyline_t al = asm_create_instance(buffer, size);
#if 0
    for (i = 0; i < 10; i++)
	    asm_assemble_str(al, "NOP");
    
    asm_assemble_str(al, "JMP 4");
    for (int i = 0; i < 4; i++)
	asm_assemble_str(al, "NOP");
#endif

    asm_assemble_str(al, "CMP RDI, 1");
    asm_assemble_str(al, "JE 124");
    for (i = 0; i < 124; i++)
	    asm_assemble_str(al, "NOP");

    // align RET into next btb set
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <assemblyline.h>
#include <assert.h>
#include <time.h>

#define LOW_MASK  0x7FFFFFFFF000ULL
#define BRANCH_MASK 0xFFFFF000
//#define memory_barrier asm volatile (".rept 10000;\nsfence;\nmfence;\nlfence\n.endr");
#define memory_barrier asm volatile ("lfence; mfence"); delayloop(5000);



#define COUNT 1000

void *create_buffer(uint64_t adrs, int size);
void *create_jmpn512(uint64_t adrs, int size);
void *create_jmp2048(uint64_t adrs, int size);
void *create_testn512(uint64_t adrs, int size);
void *create_test2048(uint64_t adrs, int size);
void *create_space(uint64_t adrs, int size);
void free_alloced();
void delayloop(uint32_t cycles);


int dummy;
// Do long now...
int count_test[64] = {0};
int done_test = 0;


void *alloced[1000];
size_t sizes[1000];
int nalloced = 0;

int main()
{
    time_t t;
    srand((unsigned) time(&t));
    int do_loop = 1;
    int count = 0;
    setlinebuf(stdout);
    while (do_loop) {
      free_alloced();
        memory_barrier
        memory_barrier
        memory_barrier
        memory_barrier
        memory_barrier
        memory_barrier

        uint64_t branch_adrs = random() & BRANCH_MASK;
        uint64_t rand_tag = random() & 0xFF;
        uint64_t test_adrs = 0;
        uint64_t tmp_address, monitor_address, monitor_address2;
        void (*branch)() = NULL;
        void (*test)() = NULL;
        void (*space)() = NULL;
        uint64_t timing, timing2 = 200;
        int k = random()%32 + 14;
        int more_space;
        int same_target = 1;


        // Randomly test short or long branch
        int do_short = random()%2;

	void (*noise[10])();
	int nnoise;
    retry:
        if (do_short) {
	    uint64_t mask = (random() & 0x3ffULL) << 22;
	    mask |= (mask >> 8) & (0xff << 14);
            test_adrs = (branch_adrs+507) ^ mask;
            branch = create_jmpn512(branch_adrs, 4096 * 2);
            if (branch == NULL)
                goto retry;
            test = create_testn512(test_adrs, 4096 * 2);
            if (test == NULL)
                goto retry;
            tmp_address = (uint64_t)branch - 507;
            monitor_address = (((uint64_t)test >> k) << k) | (tmp_address & ((1ULL << k)-1)); 
            monitor_address2 =  (uint64_t)test - 507;
            if (monitor_address2 != monitor_address)
                same_target = 0;
	    nnoise = random() % 8 + 3;
	    for (int i = 0; i < nnoise; i++) {
	      mask = (random() & 0x3ffULL) << 22;
	      mask |= (mask >> 8) & (0xff << 14);
	      noise[i] = create_testn512(test_adrs, 4096 * 2);
	    }
        } else {
	    nnoise = 0;
            test_adrs = ((branch_adrs & ~(0x3ULL << 30)) | ((random() & 0x3) << 30)) ^ (rand_tag << 14) ^ (rand_tag << 22);
            branch = create_jmp2048(branch_adrs, 4096 * 2);
            test = create_test2048(test_adrs, 4096 * 2);
            if ((branch == NULL) || (test == NULL))
                goto retry;
            tmp_address = (uint64_t)branch + 2048 + 5;
            monitor_address = (((uint64_t)test >> k) << k) | (tmp_address & ((1ULL << k)-1));
            monitor_address2 = (((uint64_t)test >> k) << k) | (tmp_address & ((1ULL << k)-1)); 
        }

        more_space = (((uint64_t)test ^ monitor_address) >> 13) == 0 ? 0 : 1;
        if (more_space)
            space = create_space(monitor_address, 4096 * 2);

        memory_barrier
        memory_barrier
        memory_barrier

	for (int i = 0; i < nnoise; i++)
	  if (noise[i]) {
	    (noise[i])();
	    memory_barrier
	    memory_barrier
	    memory_barrier
	  }

        memory_barrier
        memory_barrier
        memory_barrier

        branch();
        memory_barrier
        memory_barrier
        
        asm volatile("clflush 0(%0)":: "r"((void*)monitor_address));
        asm volatile("clflush 0(%0)":: "r"((void*)monitor_address2));
        asm volatile("clflush 0(%0)":: "r"(test));
        memory_barrier
        memory_barrier
        memory_barrier

        test();
        memory_barrier
        memory_barrier

        timing = __rdtscp(&dummy);
        asm volatile("mov (%0), %%eax":: "r"((void*)monitor_address));
        timing = __rdtscp(&dummy) - timing;

        memory_barrier
        memory_barrier

        if (do_short && (!same_target)) {
            timing2 = __rdtscp(&dummy);
            asm volatile("mov (%0), %%eax":: "r"((void*)monitor_address2));
            timing2 = __rdtscp(&dummy) - timing2;
        }
        memory_barrier
        // long/short branch, k, timing, timing2

        if (count_test[k - 14 + do_short * 32] < COUNT) {
            count_test[k - 14 + do_short * 32] += 1;
            if (count_test[k - 14 + do_short * 32] >= COUNT)
                done_test += 1;
	  printf("%d, %d, %d, %d\n", do_short, k, timing > 100 ? 0 : 1, timing2 > 100 ? 0 : 1);
        }

        if (done_test >= 64)
            do_loop = 0;
        memory_barrier
    }
}

void *create_jmp2048(uint64_t adrs, int size)
{
    int i;
    adrs = ((uint64_t)rand() << 32) | (adrs & 0xFFFFFFFF);
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    if (buffer == NULL)
        return NULL;
    assemblyline_t al = asm_create_instance(buffer, size);

    asm_assemble_str(al, "JMP 2048");
    for (i = 0; i < 2048; i++)
	    asm_assemble_str(al, "NOP");
    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_jmpn512(uint64_t adrs, int size)
{
    int i;
    adrs = ((uint64_t)rand() << 32) | (adrs & 0xFFFFFFFF);
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    
    if (buffer == NULL)
        return NULL;

    assemblyline_t al = asm_create_instance(buffer, size);
    
    asm_assemble_str(al, "RET");

    for (int i = 0; i < 506; i++)
      asm_assemble_str(al, "NOP");

    asm_assemble_str(al, "JMP -512");
   
    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return (func+507);
}


void *create_test2048(uint64_t adrs, int size)
{
    int i;
    adrs = ((uint64_t)rand() << 32) | (adrs & 0xFFFFFFFF);
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    if (buffer == NULL)
        return NULL;
    assemblyline_t al = asm_create_instance(buffer, size);

    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_testn512(uint64_t adrs, int size)
{
    int i;
    adrs = ((uint64_t)rand() << 32) | (adrs & 0xFFFFFFFF);
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    if (buffer == NULL)
        return NULL;
    assemblyline_t al = asm_create_instance(buffer + (adrs & 0xFFF), size);

    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_space(uint64_t adrs, int size)
{
    int i;
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    if (buffer == NULL)
        return NULL;
    assemblyline_t al = asm_create_instance(buffer , size);

    for (i = 0; i < 64; i++)
	    asm_assemble_str(al, "NOP");
    asm_assemble_str(al, "RET");

    void (*func)() = asm_get_code(al);
    asm_destroy_instance(al);
    return func;
}

void *create_buffer(uint64_t adrs, int size)
{
    static int nfail = 0;
    void* buffer = (void*)mmap( (void*)(adrs & LOW_MASK), size,
			    PROT_READ | PROT_WRITE | PROT_EXEC,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_POPULATE,
			    -1, 0);
    if (buffer == MAP_FAILED) {
        printf("Fail to allocate memory\n");
	if (nfail > 100)
	  exit(1);
	nfail++;
        return NULL;
    }
    nfail = 0;
    alloced[nalloced] = buffer;
    sizes[nalloced] = size;
    nalloced++;
    return buffer;
}

void free_alloced() {
  while (nalloced > 0) {
    nalloced--;
    munmap(alloced[nalloced], sizes[nalloced]);
    alloced[nalloced] = NULL;
    sizes[nalloced] = 0;
  }
}

void delayloop(uint32_t cycles) {
  uint64_t start = __rdtscp(&dummy);
  while ((__rdtscp(&dummy)-start) < cycles)
    ;
}


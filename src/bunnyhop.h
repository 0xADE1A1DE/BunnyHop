#pragma once

#include <stdint.h>

typedef void (*bhfunc)(void);

bhfunc genspy(uint64_t adrs, int size, int branch_distance);
bhfunc genvictim(uint64_t adrs, int size, int branch_distance);
bhfunc gentrainer(uint64_t adrs, int size, int branch_distance);
void bhfree(bhfunc f, int size);

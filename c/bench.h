#ifndef BENCH_H
#define BENCH_H

#include <stdint.h>

/* Runs the model for `mcycles` M-cycles over a dummy bus and prints the rate.
 * Returns 0 if the performance criterion is met. */
int bench_run(uint64_t mcycles);

#endif

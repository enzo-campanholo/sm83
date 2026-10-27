#ifndef SST_H
#define SST_H

/* Runs every test in a SingleStepTests file and prints a summary line.
 * Returns 0 if all pass, 1 if any fails, -1 if the file cannot be used. */
int sst_run_file(const char *path);

#endif

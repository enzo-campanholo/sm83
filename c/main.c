/* Command-line interface of the emulator. */
#include "bench.h"
#include "sm83.h"
#include "sst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int usage(void)
{
    fprintf(stderr,
            "usage:\n"
            "  sm83 sst FILE...       run SingleStepTests files\n"
            "  sm83 bench [MCYCLES]   run the performance benchmark (default 100000000)\n");
    return 2;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return usage();
    if (sm83_init() != 0)
        return 2;

    if (strcmp(argv[1], "sst") == 0) {
        if (argc < 3)
            return usage();
        int status = 0;
        for (int i = 2; i < argc; i++) {
            int r = sst_run_file(argv[i]);
            if (r < 0)
                status = 2;
            else if (r > 0 && status == 0)
                status = 1;
        }
        return status;
    }
    if (strcmp(argv[1], "bench") == 0) {
        uint64_t n = 100000000;
        if (argc > 2) {
            char *end;
            n = strtoull(argv[2], &end, 10);
            if (*end != '\0' || n == 0)
                return usage();
        }
        return bench_run(n);
    }
    return usage();
}

/*
 * Test from attachement to this message:
 * http://old.nabble.com/-PATCH-0-1--ARM%3A-NEON-optimized-implementation-of-memcpy.-td24328820.html
 *
 * Originally, this code compared memcpy_neon to memcpy_arm. Those parts have
 * been ifdefed out and the test engine has been kept.
 */
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <eprintf.h>
#include <puny.h>

#if 0
void *memcpy_neon(void *, const void *, size_t);
void *memcpy_arm(void *, const void *, size_t);
#endif

#define CORRECTNESS_TESTS_COUNT 300000
#define CORRECTNESS_TEST_BUFFER_SIZE 16384

#define BUFFER_SIZE (4 * 1024 * 1024)

uint8_t *testbuffer8_1w;
uint8_t *testbuffer8_1r;
uint8_t *testbuffer8_2w;
uint8_t *testbuffer8_2r;

struct {
  double scale;
  char *units;
  char *legend;
} meg = { 1024.0 * 1024.0, "MiB", "2**20 or 1,048,576 bytes" };

uint8_t run_correctness = TRUE;

void *memcpy_trivial(void *d, const void *s, size_t count) {
  uint8_t *dst = d;
  const uint8_t *src = s;
  while (count-- > 0) *dst++ = *src++;
  return d;
}

int run_correctness_test(void) {
  int i;
  uint8_t c8;
  int offs1, offs2, size;
  printf("--- Running correctness tests (use '-b' option to skip) ---\n");
  for (i = 0; i < CORRECTNESS_TEST_BUFFER_SIZE; i++) {
    c8 = rand();
    testbuffer8_1r[i] = c8;
    testbuffer8_2r[i] = c8;
    testbuffer8_1w[i] = c8;
    testbuffer8_2w[i] = c8;
  }
  srand(0);
  for (i = 0; i < CORRECTNESS_TESTS_COUNT; i++) {
    offs1 = rand() % (BUFFER_SIZE / 2);
    offs2 = rand() % (BUFFER_SIZE / 2);
    size = (rand() % 2) ? (rand() % (CORRECTNESS_TEST_BUFFER_SIZE / 2))
                        : (rand() % 64);

    memcpy_trivial(testbuffer8_1w + offs1, testbuffer8_1r + offs2, size);
#if 0
    memcpy_neon(testbuffer8_2w + offs1, testbuffer8_2r + offs2, size);
#endif
    memcpy(testbuffer8_2w + offs1, testbuffer8_2r + offs2, size);
    if (memcmp(testbuffer8_1w, testbuffer8_2w,
               CORRECTNESS_TEST_BUFFER_SIZE) != 0) {
      printf("memcpy_trivial: test failed, i=%d, offs1=%d offs2=%d, size=%d\n",
             i, offs1, offs2, size);
      exit(1);
    }
  }
  printf("all the correctness tests passed\n\n");
  return 0;
}

static int64_t gettime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

#define OFFS1 64
#define OFFS2 64

void run_bench(const char *msg,
    uint8_t *dst, uint8_t *src, int size,
    void *(*f)(void *, const void *, size_t)) {
  int64_t before1, after1, before2, after2;
  int i, j, k, kmax;

  kmax = 1024 * 1024 / size;
  if (kmax == 0) kmax = 1;
  if (kmax > 256) kmax = 256;

  /* Note: we do copy in both directions on purpose. The point
     is that ARM does not support write-allocate for L1 cache.
     During the test, destination buffer may get sometimes
     evicted from cache (if some other process gets activated
     for a short period of time and suddenly triggers load of
     lots of data into cache, evicting our data). If this
     happens, writes into a destination buffer would always
     miss cache, hugely impacting performance. As a result,
     benchmark numbers would vary a lot in a nonreproducible
     way. Reading from both source and destination buffers
     during test ensures that the data would be immediately
     reloaded into L1 cache */
  f(dst, src, size + 64);
  f(src, dst, size + 64);

  before1 = gettime();
  for (k = 0; k < kmax; k++)
  for (i = 0; i < OFFS1; i++)
  for (j = 0; j < OFFS2; j++) {
    f(dst + i, src + j, size);
    f(src + j, dst + i, size);
  }
  after1 = gettime();

  before2 = gettime();
  for (k = 0; k < kmax; k++)
  for (i = 0; i < OFFS1; i++)
  for (j = 0; j < OFFS2; j++) {
    f(dst, src, size);
    f(src, dst, size);
  }
  after2 = gettime();

  printf("%s (%d bytes copy) = %6.1f %s/s / %6.1f %s/s\n", msg, size,
       (double)size * OFFS1 * OFFS2 * 1000000. * kmax * 2 /
           (double)(after1 - before1) / meg.scale,
        meg.units,
       (double)size * OFFS1 * OFFS2 * 1000000. * kmax * 2 /
           (double)(after2 - before2) / meg.scale,
       meg.units);
}

void run_bench_for_for_size(int size) {
#if 0
  run_bench("memcpy_neon   : ", testbuffer8_1w, testbuffer8_2w,
            size, memcpy_neon);
  run_bench("memcpy_arm    : ", testbuffer8_1w, testbuffer8_2w,
            size, memcpy_arm);
#endif
  /* insert a call to benchmark your own implementation here */
  run_bench("memcpy_trivial: ", testbuffer8_1w, testbuffer8_2w,
            size, memcpy_trivial);
  run_bench("memcpy        : ", testbuffer8_1w, testbuffer8_2w,
            size, memcpy);
}

void run_performance_tests(void) {
  printf("--- Running benchmarks (average case/perfect alignment case) ---\n");

  printf("\nvery small data test:\n");
  run_bench_for_for_size(3);
  run_bench_for_for_size(4);
  run_bench_for_for_size(5);
  run_bench_for_for_size(7);
  run_bench_for_for_size(8);
  run_bench_for_for_size(11);
  run_bench_for_for_size(12);
  run_bench_for_for_size(15);
  run_bench_for_for_size(16);
  run_bench_for_for_size(24);
  run_bench_for_for_size(31);

  printf("\nL1 cached data:\n");
  run_bench_for_for_size(4096);
  run_bench_for_for_size(4096 + 2048);

  printf("\nL2 cached data:\n");
  run_bench_for_for_size(65536);
  run_bench_for_for_size(65536 + 32768);

  printf("\nSDRAM:\n");
  run_bench_for_for_size(2 * 1024 * 1024);
  run_bench_for_for_size(3 * 1024 * 1024);

  printf("\n(*) 1 %s = %s\n", meg.units, meg.legend);
#if 0
  printf("(*) 'memcpy_arm' - an implementation for"
         " older ARM cores from glibc-ports\n");
#endif
}

bool myopt (int c) {
  switch (c) {
  case 'b':
    run_correctness = FALSE;
    break;
  case 'm':
    meg.scale  = 1000. * 1000.;
    meg.legend = "1,000,000 bytes";
    meg.units  = "MB";
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void usage(void) {
  pr_usage("-bhm\n"
           "\tb - only run benchmark, don't include correctness test\n"
           "\th - help\n"
           "\tm - Use Meg = 1,000,000; default is 2**20 or 1,048,576\n");
}

int main(int argc, char *argv[]) {
  uint8_t *p;
  int rc = posix_memalign((void **)&p, 4096, BUFFER_SIZE * 4);
  if (rc) {
    fatal("posix_memalign %d", rc);
  }
  testbuffer8_1w = p + 0 * BUFFER_SIZE;
  testbuffer8_1r = p + 1 * BUFFER_SIZE;
  testbuffer8_2w = p + 2 * BUFFER_SIZE;
  testbuffer8_2r = p + 3 * BUFFER_SIZE;

  punyopt(argc, argv, myopt, "bm");
  if (run_correctness)
    run_correctness_test();
  run_performance_tests();

  free(p);
  return 0;
}

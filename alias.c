#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "CStopWatch.h"

#define NUM_SIDES 1000000
#define NUM_ROLLS 1000000

#ifndef _OPENMP
int omp_get_num_threads() { return 1; }
int omp_get_thread_num()  { return 0; }
int omp_get_max_threads() { return 1; }
void omp_set_num_threads(int i) {}
#endif

typedef struct {
        int id;
        float p;
} bar;

typedef struct drand48_data rand_buffer;

DEFSW(generation);
DEFSW(normalisation);
DEFSW(split);
DEFSW(construction);
DEFSW(final);
DEFSW(sampling);
DEFSW(total);


rand_buffer buffer[1];
#pragma omp threadprivate(buffer)


void split_large_small(float* weights, bar *small_bars, bar *large_bars,
                int *num_small_ptr, int *num_large_ptr, int num_sides, int splitsize)
{
        int num_threads = omp_get_max_threads();
        int i;

#pragma omp parallel
        {
                int me = omp_get_thread_num();
                int num_small = 0;
                int num_large = 0;
                bar *small_bar = small_bars + me * splitsize;
                bar *large_bar = large_bars + me * splitsize;
#pragma omp for nowait schedule(static)
                for (i = 0; i < num_sides; i++) {
                        bar new_bar;
                        new_bar.p = weights[i];
                        new_bar.id = i;
                        if (new_bar.p < 1) {
                                *small_bar = new_bar;
                                small_bar++;
                                num_small++;
                        } else {
                                *large_bar = new_bar;
                                large_bar++;
                                num_large++;
                        }
                }
                
                num_small_ptr[me] = num_small;
                num_large_ptr[me] = num_large;
                
        }
        
}

void make_table(float* weights, float *dartboard, int *aliases, int num_sides)
{
        SWTICK(split);
        int i;
        int num_threads = omp_get_max_threads();
        bar *all_small_bars = malloc(num_sides * sizeof(bar));
        bar *all_large_bars = malloc(num_sides * sizeof(bar));
        int all_num_small[num_threads], all_num_large[num_threads];

        int splitsize = num_sides / num_threads;
        if (num_sides % num_threads != 0) {
          splitsize++;
        }

        split_large_small(weights, all_small_bars, all_large_bars,
                        all_num_small, all_num_large, num_sides, splitsize);

        SWTOCK(split);
        SWTICK(construction);

        bar *small_bars, *large_bars;
        int num_small, num_large;
#ifdef _OPENMP
#pragma omp parallel private(small_bars, large_bars, num_small, num_large)
        {
                int me = omp_get_thread_num();
                small_bars = all_small_bars + (me * splitsize);
                large_bars = all_large_bars + (me * splitsize);
                num_small = all_num_small[me];
                num_large = all_num_large[me];
                while ((num_small > 0) && (num_large > 0)) {
                        bar small = small_bars[num_small - 1];
                        bar large = large_bars[num_large - 1];
                        dartboard[small.id] = small.p;
                        aliases[small.id] = large.id;
                        large.p = small.p + large.p - 1;
                        if (large.p >= 1) {
                                large_bars[num_large - 1] = large;
                                num_small--;
                        } else {
                                small_bars[num_small - 1] = large;
                                num_large--;
                        }
                }
                all_num_small[me] = num_small;
                all_num_large[me] = num_large;
        }

        num_small = all_num_small[0];
        num_large = all_num_large[0];
        bar *small_out, *large_out;
        small_bars = all_small_bars;
        small_out = all_small_bars;
        large_bars = all_large_bars;
        large_out = all_large_bars;
        for (i = 1; i < num_threads; i++) {
                small_out += all_num_small[i-1];
                small_bars += splitsize;
                num_small += all_num_small[i];
                memcpy(small_out, small_bars, all_num_small[i] * sizeof(bar));
                large_out += all_num_large[i-1];
                large_bars += splitsize;
                num_large += all_num_large[i];
                memcpy(large_out, large_bars, all_num_large[i] * sizeof(bar));
        }
#else
        num_small = all_num_small[0];
        num_large = all_num_large[0];
#endif
        
        small_bars = all_small_bars;
        large_bars = all_large_bars;
        while ((num_small > 0) && (num_large > 0)) {
                bar small = small_bars[num_small - 1];
                bar large = large_bars[num_large - 1];
                dartboard[small.id] = small.p;
                aliases[small.id] = large.id;
                large.p = small.p + large.p - 1;
                if (large.p >= 1) {
                        large_bars[num_large - 1] = large;
                        num_small--;
                } else {
                        small_bars[num_small - 1] = large;
                        num_large--;
                }
        }

        SWTOCK(construction);
        SWTICK(final);        

#pragma omp parallel
        {
#pragma omp for nowait
                for (i = 0; i < num_small; i++) {
                        bar small = small_bars[i];
                        dartboard[small.id] = 1;
                        aliases[small.id] = small.id;
                }
#pragma omp for nowait
                for (i = 0; i < num_large; i++) {
                        bar large = large_bars[i];
                        dartboard[large.id] = 1;
                        aliases[large.id] = large.id;
                }
        }
        free(all_small_bars);
        free(all_large_bars);
        SWTOCK(final);
}

/* Scale weights so their mean is 1 */
void normalise(float *weights, int num_sides)
{
        int i;
        float sum;
#pragma omp parallel
        {
#pragma omp for reduction(+:sum)
                for (i = 0; i < num_sides; i++) {
                        sum += weights[i];
                }
                
                float temp = num_sides/sum;
                
#pragma omp for
                for (i = 0; i < num_sides; i++) {
                        weights[i] *= temp;
                }
        }
}

int roll(float *dartboard, int *aliases, int num_sides, rand_buffer* buffer)
{
        double x, y;
        drand48_r(buffer, &x);
        drand48_r(buffer, &y);
        int col = x * num_sides;
        if (y > dartboard[col]) {
                return aliases[col];
        } else {
                return col;
        }
}

void print_interval(char *description, unsigned long interval)
{
        printf("%s: %fs\n", description,
                        (double) interval / (double) 1000000);
}

int main(int argc, char *argv[])
{
        SWRESET(generation);
        SWRESET(normalisation);
        SWRESET(split);
        SWRESET(construction);
        SWRESET(final);
        SWRESET(sampling);
        SWRESET(total);

        if (argc > 1) {
          omp_set_num_threads(atoi(argv[1]));
        }

        int j;
        SWTICK(total);
#pragma omp parallel
        {
#pragma omp master
                printf("Running with %d threads\n",
                                omp_get_num_threads());
                srand48_r(omp_get_thread_num(), buffer);
        }
        for (j = 0; j < 100; j++) {
                float *weights = malloc(NUM_SIDES * sizeof(float));
                float *dartboard = malloc(NUM_SIDES * sizeof(float));
                int *aliases = malloc(NUM_SIDES * sizeof(int));
                int i;
                SWTICK(generation);
#pragma omp parallel for shared(weights)
                for (i = 0; i < NUM_SIDES; i++) {
                        double w;
                        drand48_r(buffer, &w);
                        weights[i] = (float) w;
                }       
                SWTOCK(generation);
                SWTICK(normalisation);
                normalise(weights, NUM_SIDES);
                SWTOCK(normalisation);
                make_table(weights, dartboard, aliases, NUM_SIDES);
                SWTICK(sampling);
#pragma omp parallel for shared(dartboard, aliases) schedule(static) 
                for (i = 0; i < NUM_ROLLS; i++) {
                        roll(dartboard, aliases, NUM_SIDES, buffer);
                }
                SWTOCK(sampling);
                free(aliases);
                free(dartboard);
                free(weights);
        }
        SWTOCK(total);
        print_interval("Weight generation", SWGET(generation));
        print_interval("Normalisation", SWGET(normalisation));
        print_interval("Table split", SWGET(split));
        print_interval("Table construction", SWGET(construction));
        print_interval("Table final", SWGET(final));
        print_interval("Sampling", SWGET(sampling));
        print_interval("Total", SWGET(total));
        return 0;
}

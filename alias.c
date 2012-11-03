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
#endif

typedef struct {
        int id;
        float p;
} bar;

typedef struct drand48_data rand_buffer;

rand_buffer buffer[1];
#pragma omp threadprivate(buffer)


void split_large_small(float* weights, bar *small_bars, bar *large_bars,
                int *num_small_ptr, int *num_large_ptr, int num_sides)
{
        int num_threads = omp_get_max_threads();
        bar *shared_small_bars = (bar *) malloc (num_sides * sizeof(bar));
        bar *shared_large_bars = (bar *) malloc (num_sides * sizeof(bar));
        int all_num_small[num_threads];
        int all_num_large[num_threads];
        int i;

        int splitsize = num_sides / num_threads;
        if (num_sides % num_threads != 0) {
          splitsize++;
        }

#pragma omp parallel
        {
                int me = omp_get_thread_num();
                int num_small = 0;
                int num_large = 0;
                bar *small_bar = shared_small_bars + me * splitsize;
                bar *large_bar = shared_large_bars + me * splitsize);
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
                
                all_num_small[me] = num_small;
                all_num_large[me] = num_large;

#pragma omp barrier

                int local_small = 0;
                int local_large = 0;
                int j;
                
                for (j = 0; j < me; j++) {
                        local_small += all_num_small[j];
                        local_large += all_num_large[j];
                }
                
                bar *small_bar_out = small_bars + local_small;
                bar *large_bar_out = large_bars + local_large;
                
                small_bar -= num_small;
                large_bar -= num_large;

                memcpy(small_bar_out, small_bar, num_small * sizeof(bar));
                memcpy(large_bar_out, large_bar, num_large * sizeof(bar));
                
        }
        
        int num_small = 0;
        int num_large = 0;
        
        for (i = 0; i < num_threads; i++) {
          num_small += all_num_small[i];
          num_large += all_num_large[i];
        }
        *num_small_ptr = num_small;
        *num_large_ptr = num_large;
        
        free(shared_small_bars);
        free(shared_large_bars);
}

void make_table(float* weights, float *dartboard, int *aliases, int num_sides)
{
        bar *small_bars = malloc(num_sides * sizeof(bar));
        bar *large_bars = malloc(num_sides * sizeof(bar));
        int num_small, num_large;
        split_large_small(weights, small_bars, large_bars,
                        &num_small, &num_large, num_sides);
        while ((num_small > 0) && (num_large > 0)) {
                bar small = small_bars[num_small - 1];
                bar large = large_bars[num_large - 1];
                dartboard[small.id] = small.p;
                aliases[small.id] = large.id;
                large.p = small.p + large.p - 1;
                if (large.p >= 1) {
                        large_bars[num_large] = large;
                        num_small--;
                } else {
                        small_bars[num_small] = large;
                        num_large--;
                }
        }
        while (num_small > 0) {
                bar small = small_bars[num_small - 1];
                dartboard[small.id] = 1;
                aliases[small.id] = small.id;
                num_small--;
        }
        while (num_large > 0) {
                bar large = large_bars[num_large - 1];
                dartboard[large.id] = 1;
                aliases[large.id] = large.id;
                num_large--;
        }
        free(small_bars);
        free(large_bars);
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
#pragma omp for
                for (i = 0; i < num_sides; i++) {
                        weights[i] = num_sides * weights[i]/sum;
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
        printf("%s: %.2fs\n", description,
                        (double) interval / (double) 1000000);
}

int main()
{
        int j;
        DEFSW(generation);
        DEFSW(normalisation);
        DEFSW(construction);
        DEFSW(sampling);
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
                SWTICK(construction);
                make_table(weights, dartboard, aliases, NUM_SIDES);
                SWTOCK(construction);
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
        print_interval("Weight generation", SWGET(generation));
        print_interval("Normalisation", SWGET(normalisation));
        print_interval("Table construction", SWGET(construction));
        print_interval("Sampling", SWGET(sampling));
        return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_SIDES 1000000
#define NUM_ROLLS 1000000

#ifndef _OPENMP
int omp_get_num_threads() { return 1; }
int omp_get_thread_num()  { return 0; }
#endif

typedef struct {
        int id;
        float p;
} bar;

void split_large_small(float* weights, bar *small_bars, bar *large_bars,
                int *num_small_ptr, int *num_large_ptr, int num_sides)
{
        int num_small = 0;
        int num_large = 0;
        int i;
        for (i = 0; i < num_sides; i++) {
                bar new_bar;
                new_bar.p = weights[i];
                new_bar.id = i;
                if (new_bar.p < 1) {
                        small_bars[num_small] = new_bar;
                        num_small++;
                } else {
                        large_bars[num_large] = new_bar;
                        num_large++;
                }
        }
        *num_small_ptr = num_small;
        *num_large_ptr = num_large;
}

void make_table(float* weights, float *dartboard, int *aliases, int num_sides)
{
        bar *small_bars = malloc(num_sides * sizeof(float));
        bar *large_bars = malloc(num_sides * sizeof(float));
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
}

/* Scale weights so their mean is 1 */
void normalise(float *weights, int num_sides)
{
        int i;
        float sum;
        for (i = 0; i < num_sides; i++) {
                sum += weights[i];
        }
        for (i = 0; i < num_sides; i++) {
                weights[i] = num_sides * weights[i]/sum;
        }
}

int roll(float *dartboard, int *aliases, int num_sides)
{
        double x = drand48();
        double y = drand48();
        int col = x * num_sides;
        if (y > dartboard[col]) {
                return aliases[col];
        } else {
                return col;
        }
}

void print_interval(char *description, clock_t interval)
{
        printf("%s: %fs\n", description,
                        (double) interval / (double) CLOCKS_PER_SEC);
}

int main()
{
        int j;
        clock_t generation, normalisation, construction, sampling;
#pragma omp parallel
        {
                if (omp_get_thread_num() == 0) {
                        printf("Running with %d threads\n",
                                        omp_get_num_threads());
                }
        }
        for (j = 0; j < 100; j++) {
                float weights[NUM_SIDES];
                clock_t start = clock();
                int i;
                for (i = 0; i < NUM_SIDES; i++) {
                        weights[i] = (float) drand48();
                }
                clock_t generated_sides = clock();
                normalise(weights, NUM_SIDES);
                clock_t normalised = clock();
                float *dartboard = malloc(NUM_SIDES * sizeof(float));
                int *aliases = malloc(NUM_SIDES * sizeof(int));
                make_table(weights, dartboard, aliases, NUM_SIDES);
                clock_t made_table = clock();
#pragma omp parallel for schedule(static) shared(dartboard, aliases)
                for (i = 0; i < NUM_ROLLS; i++) {
                        roll(dartboard, aliases, NUM_SIDES);
                }
                clock_t rolled = clock();
                generation += (generated_sides - start);
                normalisation += (normalised - generated_sides);
                construction += (made_table - normalised);
                sampling += (rolled - made_table);
        }
        print_interval("Weight generation", generation);
        print_interval("Normalisation", normalisation);
        print_interval("Table construction", construction);
        print_interval("Sampling", sampling);
        return 0;
}

#include <stdio.h>
#include <stdlib.h>

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

int main()
{
        float weights[6] = { 1.0/2, 1.0/3, 1.0/24, 1.0/24, 1.0/24, 1.0/24 };
        int num_sides = 6;
        normalise(weights, num_sides);
        float *dartboard = malloc(num_sides * sizeof(float));
        int *aliases = malloc(num_sides * sizeof(int));
        make_table(weights, dartboard, aliases, num_sides);
        int i;
        for (i = 0; i < 1000000; i++) {
                roll(dartboard, aliases, num_sides);
        }
}

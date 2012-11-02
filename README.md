This is a finger exercise for experimenting with different approaches to
parallelising code. The file alias.c contains an implementation of Vose's alias
method for sampling from a finite probability distribution and some timing
code. We loop 100 times, performing the following steps:

1. Generate 1,000,000 random numbers, corresponding to weights of a
   1,000,000-sided loaded die.
2. Normalise these numbers so that their mean is 1.
3. Construct "dartboard" and "alias" tables allowing us to sample from this
   distribution in constant time.
4. Roll the die 1,000,000 times.

Steps 1 and 4 are embarrassingly parallel; step 2 is a parallelisable reduction
followed by an embarrassingly parallel division; step 3 is easy to parallelise
with a work-stealing approach but hard to make data-parallel.

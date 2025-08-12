//
// Created by ljw on 25-7-23.
//

#include "math.h"
double ceil(double x) {
    long i= (long) x;
    if (x > (double) i) return (double) i+1;
    else return (double) i;
}
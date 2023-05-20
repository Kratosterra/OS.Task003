#include <stdio.h>
#include <string.h>
#include <ctype.h>

double f(double x)
{
    return x * x / 1000.0;
}

double calculateArea(double a, double b, int all_op)
{
    double h = (b - a) / all_op;
    double sum = 0.0;
    double x;
    int i;
    for (i = 0; i < all_op; i++)
    {
        x = a + (i + 0.5) * h;
        sum += f(x);
    }
    return h * sum;
}

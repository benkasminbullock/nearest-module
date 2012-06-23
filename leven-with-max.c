#include <string.h>
#include <stdio.h>
#include "edit-distance.h"


struct pair {
    char * left;
    char * right;
    int distance;
}
pairs [] = {
    {"cat", "cut", 1},
    {"Miyagi", "Miyagawa", 3},
    {"Monster::Baby", "Manitor:;Bobi", 6},
};

int n_pairs = sizeof (pairs) / sizeof (struct pair);

static void run (int max)
{
    int i;
    printf ("Setting maximum allowed distance to %d.\n", max);
    for (i = 0; i < n_pairs; i++) {
        int left_len;
        int right_len;
        char * left;
        char * right;
        int d;

        left = pairs[i].left;
        right = pairs[i].right;
        left_len = strlen (left);
        right_len = strlen (right);
        d = distance (left, left_len, right, right_len, max);
        if (d != pairs[i].distance && pairs[i].distance <= max) {
            fprintf (stderr, "Error with distance for %s/%s: "
                     "expect %d got %d\n",
                     left, right, pairs[i].distance, d);
        }
        else {
            printf ("OK %s/%s: %d\n", left, right, d);
        }
    }
}

int main ()
{
    int max;

    max = 10;
    run (max);
    max = 6;
    run (max);
    max = 3;
    run (max);
    max = 1;
    run (max);
    return 0;
}

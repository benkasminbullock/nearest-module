/* For INT_MAX */
#include <limits.h>
#include "edit-distance.h"

int distance (const char * word1,
              int len1,
              const char * word2,
              int len2,
              int max)
{
    int matrix[len1 + 1][len2 + 1];
    int i;
    int j;

    for (i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }
    /* If the maximum is small enough to make any difference, fill in
       every cell of the matrix with a "too big" number. */
    if (max < len1 || max < len2) {
        for (i = 1; i <= len1; i++) {
            for (j = 1; j <= len2; j++) {
                matrix[i][j] = max + 1;
            }
        }
    }
    for (i = 1; i <= len1; i++) {
        char c1;
        /* The bottom of the column. */
        int min_j;
        /* The top of the column. */
        int max_j;
        /* The smallest value of the matrix in column i. */
        int col_min;

        min_j = 1;
        if (i > max) {
            min_j = i - max;
        }
        max_j = len2;
        if (len2 > max + i) {
            max_j = max + i;
        }
        //        printf ("%d - %d\n", min_j, max_j);
        c1 = word1[i-1];
        col_min = INT_MAX;
        for (j = min_j; j <= max_j; j++) {
            char c2;

            c2 = word2[j-1];
            if (c1 == c2) {
                matrix[i][j] = matrix[i-1][j-1];
            }
            else {
                int delete;
                int insert;
                int substitute;
                int minimum;

                delete = matrix[i-1][j] + 1;
                insert = matrix[i][j-1] + 1;
                substitute = matrix[i-1][j-1] + 1;
                minimum = delete;
                if (insert < minimum) {
                    minimum = insert;
                }
                if (substitute < minimum) {
                    minimum = substitute;
                }
                matrix[i][j] = minimum;
            }
            if (matrix[i][j] < col_min) {
                col_min = matrix[i][j];
            }
        }
        if (col_min > max) {
            /* All the elements of this column are greater than the
               maximum. */
            /*
            printf ("All too big at column %d.\n", i);
            for (j = min_j; j <= max_j; j++) {
                printf ("%d:%d ", j, matrix[i][j]);
            }
            printf ("\n");
            */
            return max + 1;
        }
    }
    return matrix[len1][len2];
}


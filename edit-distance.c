#include <string.h>
#include <stdio.h>
/* For INT_MAX */
#include <limits.h>
#include "edit-distance.h"

int distance (const char * word1,
              int len1,
              const char * word2,
              int len2,
              int max)
{
    int matrix[2][len2 + 1];
    int i;
    int j;

    /*
      Initialize the 0 row of "matrix".

        0  
        1  
        2  
        3  

     */

    for (j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }

    /* Loop over column. */
    for (i = 1; i <= len1; i++) {
        char c1;
        /* The first value to consider of the ith column. */
        int min_j;
        /* The last value to consider of the ith column. */
        int max_j;
        /* The smallest value of the matrix in the ith column. */
        int col_min;
        /* The next column of the matrix to fill in. */
        int next;
        /* The previously-filled-in column of the matrix. */
        int prev;

        c1 = word1[i-1];
        min_j = 1;
        if (i > max) {
            min_j = i - max;
        }
        max_j = len2;
        if (len2 > max + i) {
            max_j = max + i;
        }
        col_min = INT_MAX;
        next = i % 2;
        if (next == 1) {
            prev = 0;
        }
        else {
            prev = 1;
        }
        matrix[next][0] = i;
        /* Loop over rows. */
        for (j = 1; j <= len2; j++) {
            if (j < min_j || j > max_j) {
                /* Put a large value in there. */
                matrix[next][j] = max + 1;
            }
            else {
                char c2;

                c2 = word2[j-1];
                if (c1 == c2) {
                    /* The character at position i in word1 is the same as
                       the character at position j in word2. */
                    matrix[next][j] = matrix[prev][j-1];
                }
                else {
                    /* The character at position i in word1 is not the
                       same as the character at position j in word2, so
                       work out what the minimum cost for getting to cell
                       i, j is. */
                    int delete;
                    int insert;
                    int substitute;
                    int minimum;

                    delete = matrix[prev][j] + 1;
                    insert = matrix[next][j-1] + 1;
                    substitute = matrix[prev][j-1] + 1;
                    minimum = delete;
                    if (insert < minimum) {
                        minimum = insert;
                    }
                    if (substitute < minimum) {
                        minimum = substitute;
                    }
                    matrix[next][j] = minimum;
                }
            }
            /* Find the minimum value in the ith column. */
            if (matrix[next][j] < col_min) {
                col_min = matrix[next][j];
            }
        }
        if (col_min > max) {
            /* All the elements of the ith column are greater than the
               maximum, so no match less than or equal to max can be
               found by looking at succeeding columns. */
            return max + 1;
        }
    }
    return matrix[len1 % 2][len2];
}


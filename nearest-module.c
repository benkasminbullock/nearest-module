/* for "fail" */
#include <stdarg.h>
/* for "fprintf" */
#include <stdio.h>
#include <zlib.h>
/* For INT_MAX */
#include <limits.h>
/* For strncpy. */
#include <string.h>
/* for EXIT_FAILURE. */
#include <stdlib.h>
#include <errno.h>
/* for isspace. */
#include <ctype.h>

#include "nearest-module.h"

#ifdef HEADER

/* String length restriction for maximum length of a module name. If
   there is a CPAN module with a longer name than this, we are
   finished. */

#define MAXLEN 0x400

typedef struct nearest_module
{
    /* The file to look at. */
    gzFile * gzfile;
    FILE * file;
    /* Is this file a gz file? */
    int is_gz : 1;
    /* The term to search for. */
    const char * search_term;
    /* The length of the search term. */
    int search_len;
    /* The buffer. */
    char buf[MAXLEN];
    /* The length of the buffer. */
    int buf_len;
    /* The nearest module to the search term. */
    char nearest[MAXLEN];
    /* The edit distance from "search_term" to "nearest". */
    int distance;
    /* The name of the file to read from. */
    const char * file_name;
}
nearest_module_t;

#endif /* def HEADER */

static void fail (int test, const char * message, ...)
{
    if (test) {
        va_list arg;
        va_start (arg, message);
        vfprintf (stderr, message, arg);
        va_end (arg);
        exit (EXIT_FAILURE);
    }
    return;
}

/* Copy-paste from
   "http://www.lemoda.net/c/levenshtein/index.html". */

static int distance (const char * word1,
                     int len1,
                     const char * word2,
                     int len2)
{
    int matrix[len1 + 1][len2 + 1];
    int i;
    for (i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (i = 0; i <= len2; i++) {
        matrix[0][i] = i;
    }
    for (i = 1; i <= len1; i++) {
        int j;
        char c1;

        c1 = word1[i-1];
        for (j = 1; j <= len2; j++) {
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
        }
    }
    return matrix[len1][len2];
}

static void nearest_compare_line (nearest_module_t * nearest)
{
    /* The edit distance between "nearest->search_term" and the
       truncated version of "nearest->buf". */
    int d;
    /* The length of "nearest->buf" after truncation. */
    int l;
    /* Shorthand for "nearest->buf". */
    char * b;

    b = nearest->buf;
    /* Truncate "nearest->buf" at the first space character, or \0. */
    for (l = 0; !isspace (b[l]) && b[l]; l++)
        ;
    b[l] = '\0';

    d = distance (b, l, nearest->search_term, nearest->search_len);
    if (d < nearest->distance) {
        nearest->distance = d;
        strncpy (nearest->nearest, nearest->buf, l);
    }
    return;
}

static void
nearest_open_file (nearest_module_t * nearest)
{
    const char * gz;
    gz = strstr (nearest->file_name, ".gz");
    if (gz &&
        strlen (nearest->file_name) - (gz - nearest->file_name) ==
        strlen (".gz")) {
        nearest->is_gz = 1;
    }
    if (nearest->is_gz) {
        nearest->gzfile = gzopen (nearest->file_name, "r");
    }
    else {
        nearest->file = fopen (nearest->file_name, "r");
    }
    fail ((nearest->is_gz && !nearest->gzfile) ||
          (!nearest->is_gz && !nearest->file),
          "Cannot open package file %s: %s",
          nearest->file_name, strerror (errno));
    return;
}

static void
nearest_close_file (nearest_module_t * nearest)
{
    if (nearest->is_gz) {
        gzclose (nearest->gzfile);
    }
    else {
        fclose (nearest->file);
    }
    return;
}

static int
nearest_get_line (nearest_module_t * nearest)
{
    char * ret;

    if (nearest->is_gz) {
        ret = gzgets (nearest->gzfile, nearest->buf, MAXLEN);
        if (ret == Z_NULL) {
            if (! gzeof (nearest->gzfile)) {
                int err;
                const char * error_string;
                error_string = gzerror (nearest->gzfile, & err);
                fail (err, "Error reading %s: %s",
                      nearest->file_name, error_string);
            }
            return 0;
        }
    }
    else {
        ret = fgets (nearest->buf, MAXLEN - 1, nearest->file);
        if (! ret) {
            if (! feof (nearest->file)) {
                fail (1, "Error reading %s: %s",
                      nearest->file_name, strerror (errno));
            }
            return 0;
        }
    }
    return 1;
}

static void search_packages (nearest_module_t * nearest)
{
    nearest_open_file (nearest);
    nearest->distance = INT_MAX;
    while (nearest_get_line (nearest)) {
        nearest_compare_line (nearest);
    }
    nearest_close_file (nearest);
    return;
}

static void
nearest_set_search_term (nearest_module_t * nearest,
                         const char * search_term)
{
    nearest->search_term = search_term;
    nearest->search_len = strlen (nearest->search_term);
    return;
}

static void
nearest_set_search_file (nearest_module_t * nearest,
                         const char * file_name)
{
    nearest->file_name = file_name;
    return;
}

char *
cpan_nearest_search (char * file_name, char * search_term)
{
    nearest_module_t nearest = {0};

    nearest_set_search_term (& nearest, search_term);
    nearest_set_search_file (& nearest, file_name);

    search_packages (& nearest);
    return strdup (nearest.nearest);
}

#ifdef TEST

static void print_result (nearest_module_t * nearest)
{
    if (nearest->distance == 0) {
        printf ("Found '%s'\n", nearest->nearest);
    }
    else {
        printf ("Closest to '%s' is '%s'.\n", nearest->search_term,
                nearest->nearest);
    }
    return;
}

int main (int argc, char ** argv)
{
    nearest_module_t nearest = {0};

    nearest_set_search_term (& nearest, "Lingua::Stop::Weirds");
    nearest_set_search_file (& nearest, "02packages.details.txt");

    search_packages (& nearest);
    print_result (& nearest);
    return 0;
}
#endif /* TEST */

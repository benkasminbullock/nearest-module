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

static int compare_line (char * buf, int len, const char * search_term,
                         int search_len)
{
    int buf_len;
    int d;
    /* */
    for (buf_len = 0; !isspace (buf[buf_len]); buf_len++)
        ;
    buf[buf_len] = '\0';
    //    printf ("%s\n", buf);
    d = distance (buf, buf_len, search_term, search_len);
    return d;
}

static void search_packages_gz (const char * file_name, const char * search_term)
{
    gzFile * file;
    int len = 0x400;
    char buf[len];
    file = gzopen (file_name, "r");
    fail (! file, "Cannot open package file %s: %s", file_name, strerror (errno));
    int min_dist = INT_MAX;
    char closest[len];
    int search_len;
    search_len = strlen (search_term);
    while (1) {
        int err;
        char * ret;
        int dist;
        ret = gzgets (file, buf, len);
        if (ret == Z_NULL) {
            if (! gzeof (file)) {
                const char * error_string;
                error_string = gzerror (file, & err);
                fail (err, "Error reading %s: %s",
                      file_name, error_string);
            }
            break;
        }
        dist = compare_line (buf, len, search_term, search_len);
        if (dist < min_dist) {
            min_dist = dist;
            strncpy (closest, buf, len);
        }
    }
    gzclose (file);
    if (min_dist == 0) {
        printf ("Found '%s'\n", closest);
    }
    else {
        printf ("Closest to '%s' is '%s'.\n", search_term, closest);
    }
    return;
}

static void search_packages (const char * file_name, const char * search_term)
{
    FILE * file;
    int len = 0x400;
    char buf[len];
    file = fopen (file_name, "r");
    fail (! file, "Cannot open package file %s: %s", file_name, strerror (errno));
    int min_dist = INT_MAX;
    char closest[len];
    int search_len;
    search_len = strlen (search_term);
    while (1) {
        char * ret;
        int dist = INT_MAX;
        ret = fgets (buf, len, file);
        if (! ret) {
            if (! feof (file)) {
                fail (1, "Error reading %s: %s",
                      file_name, strerror (errno));
            }
            break;
        }
        dist = compare_line (buf, len, search_term, search_len);
        if (dist < min_dist) {
            min_dist = dist;
            strncpy (closest, buf, len);
        }
    }
    fclose (file);
    if (min_dist == 0) {
        printf ("Found '%s'\n", closest);
    }
    else {
        printf ("Closest to '%s' is '%s'.\n", search_term, closest);
    }
    return;
}

int main (int argc, char ** argv)
{
    const char * search_term;
    /* The package file. */

    search_term = "Lingua::HarrayPotter::Stop::Weirds";
#if 0
    const char * packages =
        "/home/ben/.cpan/sources/modules/02packages.details.txt.gz";
    search_packages_gz (packages, search_term);
#else
    const char * packages =
        "02packages.details.txt";
    search_packages (packages, search_term);
#endif
    //    search_term = argv[1];
    //    search_term = "CGI::Compress:Gzip";
    return 0;
}

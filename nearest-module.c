/* for "fail" */
#include <stdarg.h>
/* for "fprintf" */
#include <stdio.h>
#include <zlib.h>
/* For strncpy. */
#include <string.h>
/* for EXIT_FAILURE. */
#include <stdlib.h>
#include <errno.h>
/* for isspace. */
#include <ctype.h>

#include "nearest-module.h"
#include "edit-distance.h"


#ifdef HEADER

/* String length restriction for maximum length of a module name. If
   there is a CPAN module with a longer name than this, we are
   finished. */

#define MAXLEN 0x400

/* The length of the buffer used to read the file when the file is
   compressed. Setting this to 0x10000 made the program no faster, and
   setting it to 0x100000 made the program slower. */

#define GZ_BUFFER_LEN 0x1000

static int max_unique_characters = 45;

typedef struct nearest_module
{
    /* The file to look at. */
    gzFile * gzfile;
    FILE * file;
    unsigned char * gz_buffer;
    int gz_buffer_at;
    /* Is this file a gz file? */
    int is_gz : 1;
    /* Print blah messages? */
    int verbose : 1;
    /* Actually found something? */
    int found : 1;
    /* Use alphabet filter? */
    int no_alphabet_filter : 1;

    int use_alphabet : 1;

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
    /* Alphabet */
    int alphabet[0x100];
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
    if (nearest->use_alphabet) {
        if (nearest->no_alphabet_filter) {
            for (l = 0; !isspace (b[l]) && b[l]; l++)
                ;
        }
        else {
            int alphabet_misses;
            alphabet_misses = 0;
            /* Truncate "nearest->buf" at the first space character, or \0. */
            for (l = 0; !isspace (b[l]) && b[l]; l++) {
                int a = (unsigned char) b[l];
                if (! nearest->alphabet[a]) {
                    alphabet_misses++;
                    if (alphabet_misses > nearest->distance) {
                        return;
                    }
                }
            }
        }
    }
    else {
        for (l = 0; !isspace (b[l]) && b[l]; l++)
            ;
    }
    b[l] = '\0';

    d = distance (b, l, nearest->search_term, nearest->search_len,
                  nearest->distance);
    /*
    printf ("%d ---> '%s' %d '%s' %d\n", d, 
            nearest->search_term, nearest->search_len,
            nearest->buf, l);
    */
    if (d < nearest->distance) {
        if (nearest->verbose) {
            printf ("%s (%d) is nearer than %d.\n", nearest->buf, d,
                    nearest->distance);
        }
        nearest->found = 1;
        nearest->distance = d;
        strncpy (nearest->nearest, nearest->buf, l);
        nearest->nearest[l] = '\0';
    }
    return;
}

static int
nearest_gz_get_bytes (nearest_module_t * nearest)
{
    int ret;
    /* Read some more bytes from the file. */
    ret = gzread (nearest->gzfile, nearest->gz_buffer, GZ_BUFFER_LEN);
    /* Error checking stuff. */
    if (ret == Z_NULL) {
        if (gzeof (nearest->gzfile)) {
            return 0;
        }
        else {
            int err;
            const char * error_string;
            error_string = gzerror (nearest->gzfile, & err);
            fail (err, "Error reading %s: %s",
                  nearest->file_name, error_string);
        }
    }
    /* Set the pointer into the buffer back to the start of the
       buffer. */
    nearest->gz_buffer_at = 0;
    return 1;
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
    if (nearest->is_gz) {
        nearest->gz_buffer = malloc (GZ_BUFFER_LEN);
        fail (! nearest->gz_buffer, "no memory");
        if (! nearest_gz_get_bytes (nearest)) {
            fail (1, "File is empty");
        }
    }
    return;
}

static void
nearest_close_file (nearest_module_t * nearest)
{
    if (nearest->is_gz) {
        gzclose (nearest->gzfile);
        free (nearest->gz_buffer);
    }
    else {
        fclose (nearest->file);
    }
    return;
}

static int
nearest_gz_get_line (nearest_module_t * nearest)
{
    /* The number of bytes remaining in "nearest->gz_buffer". */
    int remaining;
    /* Boolean, set to true if we need to read more bytes from the
       gzfile. */
    int read_more_bytes;
    /* The number of bytes read into "nearest->buf so far". */
    int bt;
    int i;

    read_more_bytes = 1;
    /* Initially, "bt" is zero, but if more bytes need to be read from
       the .gz file, it can be incremented. */
    bt = 0;
 more_bytes:

    remaining = GZ_BUFFER_LEN - nearest->gz_buffer_at;
    for (i = 0; i < remaining; i++) {
        nearest->buf[bt] = nearest->gz_buffer[nearest->gz_buffer_at];
        /* If there is a carriage return in the stuff read so far, we
           don't need to read any more bytes, so we can just leave
           this routine now, without calling the gzlib. */
        nearest->gz_buffer_at++;
        if (nearest->buf[bt] == '\n') {
            read_more_bytes = 0;
            nearest->buf[bt] = '\0';
            break;
        }
        bt++;
    }
    if (read_more_bytes) {
        if (! nearest_gz_get_bytes (nearest)) {
            return 0;
        }
        goto more_bytes;
    }
    return 1;
}

static int
nearest_get_line (nearest_module_t * nearest)
{
    char * ret;

    if (nearest->is_gz) {
        return nearest_gz_get_line (nearest);
    }
    ret = fgets (nearest->buf, MAXLEN - 1, nearest->file);
    if (! ret) {
        if (! feof (nearest->file)) {
            fail (1, "Error reading %s: %s",
                  nearest->file_name, strerror (errno));
        }
        return 0;
    }
    return 1;
}

static void search_packages (nearest_module_t * nearest)
{
    static int max_sane_distance = 10;
    static int more_lines;
    nearest_open_file (nearest);
    /* Don't use INT_MAX here or get overflow. */
    nearest->distance = nearest->search_len + 1;
    if (nearest->search_len > max_sane_distance) {
        nearest->distance = max_sane_distance;
    }
    more_lines = 1;
    while (more_lines) {
        more_lines = nearest_get_line (nearest);
        nearest_compare_line (nearest);
    }
    nearest_close_file (nearest);
    return;
}

static void
nearest_set_search_term (nearest_module_t * nearest,
                         const char * search_term)
{
    int unique_characters;
    int i;
    nearest->search_term = search_term;
    nearest->search_len = strlen (nearest->search_term);
    if (nearest->use_alphabet) {
        for (i = 0; i < 0x100; i++) {
            nearest->alphabet[i] = 0;
        }
        unique_characters = 0;
        for (i = 0; i < nearest->search_len; i++) {
            int c;
            c = (unsigned char) search_term[i];
            if (! nearest->alphabet[c]) {
                unique_characters++;
                nearest->alphabet[c] = 1;
            }
        }
        if (unique_characters > max_unique_characters) {
            nearest->no_alphabet_filter = 1;
        }
        else {
            nearest->no_alphabet_filter = 0;
        }
    }
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

    /* Switch on the alphabet filter. */
    nearest.use_alphabet = 1;

    nearest_set_search_term (& nearest, search_term);
    nearest_set_search_file (& nearest, file_name);

    search_packages (& nearest);
    if (nearest.found) {
        return strdup (nearest.nearest);
    }
    else {
        return 0;
    }
}

#ifdef TEST

static void print_result (nearest_module_t * nearest)
{
    if (nearest->found) {
        if (nearest->distance == 0) {
            printf ("Found '%s'\n", nearest->nearest);
        }
        else {
            printf ("Closest to '%s' is '%s'.\n", nearest->search_term,
                    nearest->nearest);
        }
    }
    else {
        printf ("Nothing similar was found.\n");
    }
    return;
}

const char * file_name =
    "/home/ben/.cpan/sources/modules/02packages.details.txt.gz";

int main (int argc, char ** argv)
{
    nearest_module_t nearest = {0};
    char * st;
    if (argc > 1) {
        while (argc) {
            char * arg = * argv;
            if (strcmp (arg, "-v") == 0) {
                nearest.verbose = 1;
            }
            else if (strcmp (arg, "-a") == 0) {
                if (nearest.verbose) {
                    printf ("Using alphabet.\n");
                }
                nearest.use_alphabet = 1;
            }
            else {
                st = arg;
            }
            argc--;
            argv++;
        }
    }
    else {
        st = "Lingua::Stop::Weirds";
    }
    nearest_set_search_term (& nearest, st);
    nearest_set_search_file (& nearest, file_name);

    search_packages (& nearest);
    print_result (& nearest);
    return 0;
}
#endif /* TEST */

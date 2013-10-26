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

#include "text-fuzzy.h"
#include "nearest-module.h"


#ifdef HEADER

/* String length restriction for maximum length of a module name. If
   there is a CPAN module with a longer name than this, we are
   finished. */

#define MAXLEN 0x400

/* The length of the buffer used to read the file when the file is
   compressed. Setting this to 0x10000 made the program no faster, and
   setting it to 0x100000 made the program slower. */

#define GZ_BUFFER_LEN 0x1000

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
    /* The buffer. */
    char buf[MAXLEN];
    /* The length of the buffer. */
    int buf_len;
    /* The nearest module to the search term. */
    char nearest[MAXLEN];
    /* The name of the file to read from. */
    const char * file_name;
    /* Text::Fuzzy object. */
    text_fuzzy_t tf;
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

static int nearest_compare_line (nearest_module_t * nearest)
{
    /* The length of "nearest->buf" after truncation. */
    int l;
    text_fuzzy_string_t * b;

    b = & nearest->tf.b;
    b->text = nearest->buf;
    /* Compute the length. */
    for (l = 0; !isspace (b->text[l]) && b->text[l]; l++)
	;
    b->length = l;
    /* This is only necessary for the printf below, the edit
       distance routines completely ignore this, and only use "l" for
       the length. */
    b->text[l] = '\0';

    TEXT_FUZZY (compare_single (& nearest->tf));
    if (nearest->tf.found) {
	nearest->found = 1;
	if (nearest->verbose) {
            printf ("%s (%d) is nearer.\n", b->text, nearest->tf.distance);
	}
        strncpy (nearest->nearest, nearest->buf, l);
        nearest->nearest[l] = '\0';
    }
    return 0;
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

/* Open the file for reading. */

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

/* Close the file and free memory if necessary. */

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

/* Get the lines from the file. */

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

static int search_packages (nearest_module_t * nearest)
{
    static int max_sane_distance = 10;
    static int more_lines;
    int max;

    nearest_open_file (nearest);
    /* Don't use INT_MAX here or get overflow. */

    max = nearest->tf.text.length + 1;
    if (max > max_sane_distance) {
	max = max_sane_distance;
    }
    nearest->tf.max_distance = max;
    TEXT_FUZZY (begin_scanning (& nearest->tf));
    more_lines = 1;
    while (more_lines) {
        more_lines = nearest_get_line (nearest);
        nearest_compare_line (nearest);
    }
    TEXT_FUZZY (end_scanning (& nearest->tf));
    nearest_close_file (nearest);
    return 0;
}

static int
nearest_set_search_term (nearest_module_t * nearest,
                         const char * search_term)
{
    nearest->tf.text.text = (char *) search_term;
    nearest->tf.text.length = strlen (nearest->tf.text.text);
    if (nearest->tf.use_alphabet) {
	TEXT_FUZZY (generate_alphabet (& nearest->tf));
    }
    return 0;
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

    nearest.tf.use_alphabet = 1;
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
        if (nearest->tf.distance == 0) {
            printf ("Found exact match '%s'\n", nearest->nearest);
        }
        else {
            printf ("Closest to '%s' is '%s' (distance %d).\n",
		    nearest->tf.text.text,
                    nearest->nearest, nearest->tf.distance);
        }
    }
    else {
        printf ("Nothing similar was found.\n");
    }
    return;
}
/*
const char * file_name =
    "/home/ben/.cpan/sources/modules/02packages.details.txt.gz";
*/

const char * file_name =
    "/home/ben/.cpan/sources/modules/02packages.details.txt";

int main (int argc, char ** argv)
{
    nearest_module_t nearest = {0};
    char * st;
    st = "Lingua::Stop::Weirds";
    nearest.tf.use_alphabet = 1;
    if (argc > 1) {
        while (argc) {
            char * arg = * argv;
            if (strcmp (arg, "-v") == 0) {
                nearest.verbose = 1;
            }
            else {
                st = arg;
            }
            argc--;
            argv++;
        }
    }
    nearest_set_search_term (& nearest, st);
    nearest_set_search_file (& nearest, file_name);

    search_packages (& nearest);
    print_result (& nearest);
    return 0;
}
#endif /* TEST */

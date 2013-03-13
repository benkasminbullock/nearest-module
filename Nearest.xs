#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "zlib.h"

#include "text-fuzzy.h"
#include "nearest-module.h"

MODULE= CPAN::Nearest PACKAGE=CPAN::Nearest

PROTOTYPES: ENABLE

char * search (char * file_name, char * search_term)
CODE:
RETVAL = cpan_nearest_search (file_name, search_term);
OUTPUT:
RETVAL

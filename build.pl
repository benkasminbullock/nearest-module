#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use Perl::Build;
perl_build (
    makefile => 'makeitfile',
    pod => ['lib/CPAN/Nearest.pod',],
);
exit;

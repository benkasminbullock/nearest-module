use strict;
use warnings;
use ExtUtils::MakeMaker;

my $pm = 'lib/CPAN/Nearest.pm';
my $repo = 'https://github.com/benkasminbullock/nearest-module';

WriteMakefile (
    NAME => 'CPAN::Nearest',
    VERSION_FROM => $pm,
    ABSTRACT_FROM => $pm,
    AUTHOR => 'Ben Bullock <bkb@cpan.org>',
    LICENSE => 'perl',
    PREREQ_PM => {
    },
    META_MERGE => {
        resources => {
            repository => $repo,
            bugtracker => "$repo/issues",
        },
    },
    OBJECT => 'nearest-module.o Nearest.o',
    LIBS => ['-lz',],
);

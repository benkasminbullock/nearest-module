use strict;
use warnings;
use ExtUtils::MakeMaker;

my $pm = 'lib/CPAN/Nearest.pm';
my $pod = 'lib/CPAN/Nearest.pod';
my $repo = 'https://github.com/benkasminbullock/nearest-module';

# Objects to link to.

my $object = 'nearest-module.o Nearest.o';

#if ($^O eq 'MSWin32' || $^O eq 'cygwin') {
#    $object .= ' text-fuzzy.o edit-distance-char.o edit-distance-int.o';
#}

WriteMakefile (
    NAME => 'CPAN::Nearest',
    VERSION_FROM => $pm,
    ABSTRACT_FROM => $pod,
    AUTHOR => 'Ben Bullock <bkb@cpan.org>',
    LICENSE => 'perl',
    PREREQ_PM => {
	'Text::Fuzzy' => '0.08',
    },
    META_MERGE => {
        resources => {
            repository => $repo,
            bugtracker => "$repo/issues",
        },
    },
    OBJECT => $object,
    LIBS => ['-lz',],
);

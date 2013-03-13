package CPAN::Nearest;
use Text::Fuzzy '0.08';
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw/search/;
use warnings;
use strict;
our $VERSION = 0.07;
use XSLoader;
XSLoader::load 'CPAN::Nearest', $VERSION;

1;

__END__

package CPAN::Nearest;
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw/search/;
use warnings;
use strict;
our $VERSION = 0.05;
use XSLoader;
XSLoader::load 'CPAN::Nearest', $VERSION;

1;

__END__

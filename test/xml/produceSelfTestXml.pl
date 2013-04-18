#!/usr/bin/perl -w

use strict;

die "Usage: produceXml.pl HOST PORT templateFile\n" if @ARGV != 3;

my $host          = $ARGV[0];
my $port          = $ARGV[1];
my $templateFile  = $ARGV[2];

die "The file \"$templateFile\" does not exist\n" unless -e $templateFile;

open TEMPLATE_FILE, "<$templateFile";
while(<TEMPLATE_FILE>) {
  s#HOST#$host#g;
  s#PORT#$port#g;

  print;
}

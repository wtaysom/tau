#!/usr/bin/perl
use strict;
my %In = ();
my $NumFiles = 0;
my $JScode = '';

# 1. Read what the JavaScript sent.
($In{var},$In{loc}) = split /&/,$ENV{QUERY_STRING};

# 2. Grab a files listing from the directory 
# 3. and count the number of files.
if(opendir D,$In{loc})
{
	my @d = grep ! /^\.\.?$/,readdir D;
	closedir D;
	$NumFiles = @d;
}

# 4. Create JavaScript code to send back.
$JScode = "$In{var}=$NumFiles;";

# 5. Send code to the JavaScript on the web page.
print "Content-type: text/javascript\n\n";
print $JScode;
### end of script ###

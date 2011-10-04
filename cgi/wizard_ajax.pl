#!/usr/bin/perl

use strict;
use IO::Socket;

my $user = <STDIN>;
chomp $user;

my $server_addr = 'localhost';
my $server_port = '17000';

open LOG, ">/tmp/wizard.log" or die;
print LOG "opened: '$user'\n";
close LOG;
open LOG, ">>/tmp/wizard.log" or die;

print LOG "opening...\n";

my $sock = new IO::Socket::INET(
                                PeerAddr => $server_addr,
                                PeerPort => $server_port,
                                Proto => 'tcp',
                               );
die "Could not open socket: $!\n" unless $sock;

print LOG "reading...\n";

print $sock qq~<login name="$user"/>\0~;

while(my $line = <STDIN>) {
	print LOG $line;
	print $sock "$line\n";
}

print $sock "\0";

print $sock qq~<close_ajax/>\0~;

print LOG "outputting...\n";
close LOG;

print "Content-type: text/xml\n\n";
while(my $line = <$sock>) {
	print $line;
}

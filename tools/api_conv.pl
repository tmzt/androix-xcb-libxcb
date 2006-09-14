#!/usr/bin/perl -w
use strict;

sub trans_lines($);

my @xids=("WINDOW","VISUALTYPE","DRAWABLE","FONT","ATOM","COLORMAP","FONTABLE","GCONTEXT","PIXMAP","SCREEN");

my $input = $ARGV[0];

open(INFILE,"<",$input) or die("Couldn't open file $input.\n");

my @in_data = <INFILE>;
my @out_data;

foreach my $line (@in_data) {
	
	if($line =~ /#[a-z]/ or $line =~ /print/ or $line =~ /\/\// or $line =~ /\/\*/) {
		$out_data[@out_data] = $line;
		next;
	}

	trans_lines($line);
}


foreach my $newline (@out_data) {
	print $newline;
}

#################
sub trans_lines($)
{
	my $line = $_[0];
	
	$line =~ s/XCB/xcb_/g;	
	
	foreach my $xid (@xids) {
		if($line =~ /$xid/ and $line =~ /xcb_/) {
			my $lcxid = lc($xid);
			
			#var
			my $xidsp = $lcxid . " ";
			my $xidspun = $lcxid . "_t ";

			##
			$line =~ s/$xid/$lcxid/g;

			#var
			$line =~ s/$xidsp/$xidspun/g;
		}
	}

	#func without XID in it
	my $funcline = $line;

	if($funcline =~ /xcb_/) {
		$funcline =~ s/[A-Z]/"_" . lc($&)/eg;
		$funcline =~ s/__/_/g;

		if($funcline =~ /event/i) {
			$funcline =~ /event/i;
			$funcline = $` . "event" . "_t" . $';

			$funcline =~ s/__/_/g;
		}

		#repair NULL's
		$funcline =~ s/_n_u_l_l/NULL/g;
		#repair XCBSCREEN
		$funcline =~ s/s_c_r_e_e_n/screen/g;	
	}

	$line = $funcline;
		
	$out_data[@out_data] = $line;
}


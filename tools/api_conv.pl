#!/usr/bin/perl -w
use strict;

sub trans_lines();

my @xids=("WINDOW","VISUALTYPE","DRAWABLE","FONT","ATOM","COLORMAP","FONTABLE","GCONTEXT","PIXMAP","SCREEN");

while(<>) {
	
	trans_lines() unless (/#[a-z]/ or /print/ or /\/\// or /\/\*/);
	print;
}

#################
sub trans_lines()
{
	s/XCB/xcb_/g;	
	
	foreach my $xid (@xids) {
		if(/$xid/ and /xcb_/) {
			my $lcxid = lc($xid);
			
			#var
			my $xidsp = $lcxid . " ";
			my $xidspun = $lcxid . "_t ";

			##
			s/$xid/$lcxid/g;

			#var
			s/$xidsp/$xidspun/g;
		}
	}

	#func without XID in it
	if(/xcb_/) {
		s/[A-Z]/"_" . lc($&)/eg;
		s/__/_/g;

		if(/event/i) {
			$_ = $` . "event" . "_t" . $';

			s/__/_/g;
		}

		#repair NULL's
		s/_n_u_l_l/NULL/g;
		#repair XCBSCREEN
		s/s_c_r_e_e_n/screen/g;	
	}
}


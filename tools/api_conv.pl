#!/usr/bin/perl -plw
use strict;

BEGIN {
	%::const = map { $_ => 1 } (
		# constants in xcb.h
		"XCBNone",
		"XCBCopyFromParent",
		"XCBCurrentTime",
		"XCBNoSymbol",
		"XCBError",
		"XCBReply",
		# renamed constants
		"XCBButtonAny",
		"XCBButton1",
		"XCBButton2",
		"XCBButton3",
		"XCBButton4",
		"XCBButton5",
		"XCBHostInsert",
		"XCBHostDelete",
	);
	open(CONST, shift) or die "failed to open constants list: $!";
	while(<CONST>)
	{
		chomp;
		die "invalid constant name: \"$_\"" unless /^XCB[A-Za-z0-9_]*$/;
		$::const{$_} = 1;
	}
	close(CONST);
}

sub convert($$)
{
	local $_ = shift;
	my ($fun) = @_;

	return "uint$1_t" if /^CARD(8|16|32)$/;
	return "int$1_t" if /^INT(8|16|32)$/;
	return "uint8_t" if $_ eq 'BOOL' or $_ eq 'BYTE';
	return $_ if /^[A-Z]*_[A-Z_]*$/ or !/^XCB(.+)/;
	my $const = defined $::const{$_};
	$_ = $1;

	s/^(GX|RandR|XFixes|XP|XvMC)(.)/uc($1) . "_" . $2/e;

	my %abbr = (
		"Iter" => "iterator",
		"Req" => "request",
		"Rep" => "reply",
	);

	s/([0-9]+|[A-Z](?:[A-Z]*|[a-z]*))_?(?=[0-9A-Z]|$)/"_" . ($abbr{$1} or lc($1))/eg;

	$_ = "_family_decnet" if $_ eq "_family_de_cnet";
	return "XCB" . uc($_) if $const;

	$_ .= "_t" unless $fun or /_id$/;

	return "xcb" . $_;
}

s/([_A-Za-z][_A-Za-z0-9]*)([ \t]*\()?/convert($1, defined $2) . ($2 or "")/eg;

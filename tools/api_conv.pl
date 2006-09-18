#!/usr/bin/perl -plw
use strict;

BEGIN {
	%::const = map { $_ => 1 } (
		"XCBNone",
		"XCBCopyFromParent",
		"XCBCurrentTime",
		"XCBNoSymbol",
		"XCBError",
		"XCBReply",
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
	return $_ if /_/ or !/^XCB(.+)/;
	my $const = defined $::const{$_};
	$_ = $1;

	my %abbr = (
		"Iter" => "iterator",
		"Req" => "request",
		"Rep" => "reply",
	);

	s/[A-Z](?:[A-Z0-9]*|[a-z0-9]*)(?=[A-Z]|$)/"_" . ($abbr{$&} or lc($&))/eg;

	return "XCB" . uc($_) if $const;

	$_ .= "_t" unless $fun;

	return "xcb" . $_;
}

s/([_A-Za-z][_A-Za-z0-9]*)([ \t]*\()?/convert($1, defined $2) . ($2 or "")/eg;

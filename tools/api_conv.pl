#!/usr/bin/perl -plw
use strict;

sub convert($$)
{
	local $_ = shift;
	my ($fun) = @_;

	return "uint$1_t" if /^CARD(8|16|32)$/;
	return "int$1_t" if /^INT(8|16|32)$/;
	return "uint8_t" if $_ eq 'BOOL' or $_ eq 'BYTE';
	return $_ if /_/ or !/^XCB(.+)/;
	$_ = $1;

	my %abbr = (
		"Iter" => "iterator",
		"Req" => "request",
		"Rep" => "reply",
	);

	s/[A-Z](?:[A-Z0-9]*|[a-z0-9]*)(?=[A-Z]|$)/"_" . ($abbr{$&} or lc($&))/eg;
	$_ .= "_t" unless $fun;

	return "xcb" . $_;
}

s/([_A-Za-z][_A-Za-z0-9]*)([ \t]*\()?/convert($1, defined $2) . ($2 or "")/eg;

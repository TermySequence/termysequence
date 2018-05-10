#!/usr/bin/perl
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

use warnings;
use strict;

use FindBin;
use lib "$FindBin::Bin";
use Uniset;

#
## Classifier functions
#
sub is_zero_width {
    my ($codepoint, $category) = @_;

    # Negative exceptions:
    # SOFT HYPHEN
    return 0 if ($codepoint == 0xad);
    # Combining enclosing borders
    return 0 if ($codepoint >= 0x20dd) && ($codepoint <= 0x20e0);
    return 0 if ($codepoint >= 0x20e2) && ($codepoint <= 0x20e4);

    # Categories
    return 1 if $category eq 'Mn';
    return 1 if $category eq 'Me';
    return 1 if $category eq 'Cf';

    # Positive exceptions:
    # Hangul Jamo medial vowels and final consonants
    return 1 if ($codepoint >= 0x1160) && ($codepoint <= 0x11ff);
    # Unset range joiners
    return 1 if ($codepoint == 0x1dfa);

    return 0;
}

sub is_double_width {
    my ($codepoint, $category) = @_;

    # Negative exceptions:
    # IDEOGRAPHIC HALF FILL SPACE
    return 0 if ($codepoint == 0x303f);

    # Categories
    return 1 if $category eq 'F';
    return 1 if $category eq 'W';

    # Positive exceptions:
    # Combining enclosing borders
    return 1 if ($codepoint >= 0x20dd) && ($codepoint <= 0x20e0);
    return 1 if ($codepoint >= 0x20e2) && ($codepoint <= 0x20e4);
    # All CJK ... Yi
    return 1 if ($codepoint >= 0x2e80) && ($codepoint <= 0xa4cf);
    # All CJK Compatibility Forms
    return 1 if ($codepoint >= 0xfe30) && ($codepoint <= 0xfe6f);
    # All fullwidth forms
    return 1 if ($codepoint >= 0xff01) && ($codepoint <= 0xff60);
    # Regional indicator emoji
    return 1 if ($codepoint >= 0x1f1e6) && ($codepoint <= 0x1f1ff);

    return 0;
}

sub is_ambiguous_width {
    my ($codepoint, $category) = @_;

    #Categories
    return 1 if $category eq 'A';

    return 0;
}

sub is_emoji_all {
    my ($codepoint, $category) = @_;

    return 1 if $category;

    # Positive exceptions
    # FEMALE SIGN
    return 1 if $codepoint == 0x2640;
    # MALE SIGN
    return 1 if $codepoint == 0x2642;
    # STAFF OF AESCULAPIUS
    return 1 if $codepoint == 0x2695;

    return 0;
}

sub is_argument_true {
    my ($codepoint, $category) = @_;
    return !!$category;
}

#
## File parsing
#
if (@ARGV != 3) {
    print STDERR "Usage: uniwidth.pl <UnicodeData.txt> <EastAsianWidth.txt> <EmojiData.txt>\n";
    exit 1;
}

open(UH, '<', $ARGV[0]) or die "$ARGV[0]: $!\n";
my $zeroset = Uniset->new("ZERO WIDTH SET");
while (<UH>) {
    my @fields = split(';');

    my $codepoint = hex $fields[0];
    my $category = $fields[2];

    $zeroset->append_checked($codepoint, \&is_zero_width, $category, '');
}
close(UH);

open(WH, '<', $ARGV[1]) or die "$ARGV[1]: $!\n";
my $doubleset = Uniset->new("DOUBLE WIDTH SET");
while (<WH>) {
    chomp;
    s/#.*//;
    s/\s*$//;
    next if $_ eq '';

    my @fields = split(';');
    my ($start, $end);

    if ($fields[0] =~ m/([0-9a-fA-F]+)\.\.([0-9a-fA-F]+)/) {
        $start = hex $1;
        $end = hex $2;
    } else {
        $start = $end = hex $fields[0];
    }

    while ($start <= $end) {
        $doubleset->append_checked($start, \&is_double_width, $fields[1], 'N');
        ++$start;
    }
}
close(WH);

open(WH, '<', $ARGV[1]) or die "$ARGV[1]: $!\n";
my $ambigset = Uniset->new("AMBIGUOUS WIDTH SET");
while (<WH>) {
    chomp;
    s/#.*//;
    s/\s*$//;
    next if $_ eq '';

    my @fields = split(';');
    my ($start, $end);

    if ($fields[0] =~ m/([0-9a-fA-F]+)\.\.([0-9a-fA-F]+)/) {
        $start = hex $1;
        $end = hex $2;
    } else {
        $start = $end = hex $fields[0];
    }

    while ($start <= $end) {
        $ambigset->append_checked($start, \&is_ambiguous_width, $fields[1], 'N');
        ++$start;
    }
}
close(WH);

open(EH, '<', $ARGV[2]) or die "$ARGV[2]: $!\n";
my $emojiset = Uniset->new('ALL EMOJI');
my $emojipset = Uniset->new('PRESENTATION EMOJI');
my $emojimods = Uniset->new('EMOJI MODIFIERS');
while (<EH>) {
    chomp;
    s/#.*//;
    s/\s*$//;
    next if $_ eq '';

    my @fields = split(';');
    $fields[0] =~ s/\s*//g;
    $fields[1] =~ s/\s*//g;
    my ($start, $end);

    if ($fields[0] =~ m/([0-9a-fA-F]+)\.\.([0-9a-fA-F]+)/) {
        $start = hex $1;
        $end = hex $2;
    } else {
        $start = $end = hex $fields[0];
    }

    my $set = undef;
    my $func = \&is_argument_true;

    if ($fields[1] eq 'Emoji') {
        $set = $emojiset;
        $func = \&is_emoji_all;
    }
    elsif ($fields[1] eq 'Emoji_Presentation') {
        $set = $emojipset;
    }
    elsif ($fields[1] eq 'Emoji_Modifier') {
        $set = $emojimods;
    }

    if (defined($set)) {
        while ($start <= $end) {
            $set->append_checked($start, $func, 1, 0);
            ++$start;
        }
    }
}
close(EH);

print <<'EOF';
// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

EOF

# Combining Enclosing Keycap as an Emoji modifier
$emojimods->insert_range(0x20E3, 0x20E3);
$emojiset->insert_range(0x20E3, 0x20E3);

# Private-use Emoji range
$emojiset->insert_range(0xF5000, 0xF51FF);
$emojipset->insert_range(0xF5000, 0xF51FF);
$doubleset->insert_range(0xF5000, 0xF51FF);

$doubleset->tighten_uniset($zeroset);
$ambigset->tighten_uniset($zeroset);
$ambigset->insert_uniset($doubleset);

my $emojiflags = Uniset->new('NATION FLAGS EMOJI');
$emojiflags->insert_range(0x1F1E6, 0x1F1FF);

$zeroset->print('zerowidth', "\n");
$doubleset->print('doublewidth', "\n");
$ambigset->print('ambigwidth', "\n");
$emojiset->print('emoji_all', "\n");
$emojipset->print('emoji_pres', "\n");
$emojimods->print('emoji_mods', "\n");
$emojiflags->print('emoji_flags', '');

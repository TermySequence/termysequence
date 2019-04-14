#!/usr/bin/perl
# Copyright © 2019 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

use warnings;
use strict;

use FindBin;
use lib "$FindBin::Bin";
use Unitable;

#
## File parsing
#
sub parse_file {
    my ($file, $pred, $func) = @_;
    open(FH, '<', "$ARGV[0]/$file") or die "$file: $!\n";
    while (<FH>) {
        chomp;
        s/#.*//;
        s/\s*$//;
        next if $_ eq '';

        my @fields = split(/\s*(?:;|#)\s*/);
        next unless $pred->($fields[1]);
        my ($start, $end);

        if ($fields[0] =~ m/([0-9a-fA-F]+)\.\.([0-9a-fA-F]+)/) {
            $start = hex $1;
            $end = hex $2;
        } else {
            $end = $start = hex $fields[0];
        }

        $func->($start, $end, $fields[1]) if $end < 0xF0000;
    }
    close(FH);
}

sub parse_holes {
    my ($file, $func) = @_;
    my $prev = -1;
    open(FH, '<', "$ARGV[0]/$file") or die "$file: $!\n";
    while (<FH>) {
        my @fields = split(';');
        my $cur = hex $fields[0];

        if ($cur > $prev + 1) {
            if ($cur - $prev < 256) {
                $func->($prev + 1, $cur - 1);
            }
        }
        $prev = $cur;
    }
    close(FH);
}

if (@ARGV != 1 || ! -d $ARGV[0]) {
    print STDERR "Usage: unitable.pl <UnicodeDataDir>\n";
    exit 1;
}
my $datadir = $ARGV[0];
my $table = Unitable->new();
my $hangul = Unitable->new();
my $emoji = Unitable->new();

#
##
### Types
##
#

#
## GcbCombining
#
parse_file('GraphemeBreakProperty.txt',
           sub { return $_[0] eq 'Extend' || $_[0] eq 'SpacingMark' },
           sub { $table->emplace($_[0], $_[1], ['GcbCombining']) });
$table->emplace(0x200D, 0x200D, ['GcbCombining', 'GcbZwj']);

#
## GcbPictographic
#
parse_file('emoji-data.txt',
           sub { return $_[0] eq 'Extended_Pictographic' },
           sub { $table->emplace($_[0], $_[1], ['GcbPictographic']) });

#
## GcbEmojiModifier
#
$table->replace(0x1F3FB, 0x1F3FF, ['GcbEmojiModifier', 'GcbSkinToneModifier']);
$table->replace(0xFE0F, 0xFE0F, ['GcbEmojiModifier']);

#
## GcbTextModifier
#
$table->replace(0xFE0E, 0xFE0E, ['GcbTextModifier']);

#
## GcbRegionalIndicator
#
$table->emplace(0x1F1E6, 0x1F1FF, ['GcbRegionalIndicator']);

#
## GcbHangul
#
parse_file('HangulSyllableType.txt',
           sub { return 1 },
           sub { $table->emplace($_[0], $_[1], ['GcbHangul', 'DblWidthChar']) });
parse_file('HangulSyllableType.txt',
           sub { return 1 },
           sub { $hangul->emplace($_[0], $_[1], ['GcbHangul' . $_[2]]) unless $_[2] eq 'LVT' });

#
##
### Flags
##
#

#
## GcbSkinToneBase
#
#parse_file('emoji-data.txt',
#           sub { return $_[0] eq 'Emoji_Modifier_Base' },
#           sub { $table->augment($_[0], $_[1], ['GcbSkinToneBase']) });

#
## EmojiChar
#
parse_file('emoji-data.txt',
           sub { return $_[0] eq 'Emoji_Presentation' },
           sub { $table->augment($_[0], $_[1], ['EmojiChar', 'DblWidthChar']) });

parse_file('emoji-data.txt',
           sub { return $_[0] eq 'Emoji_Presentation' },
           sub { $emoji->emplace($_[0], $_[1], ['EmojiChar']) });

#
## DblWidthChar
#
parse_file('EastAsianWidth.txt',
           sub { return $_[0] eq 'F' || $_[0] eq 'W' },
           sub { $table->augment($_[0], $_[1], ['DblWidthChar']) });

#
##
### Post-Processing
##
#

# Private-use Emoji
$table->emplace(0xF5000, 0xF5FFF, ['GcbPictographic', 'EmojiChar', 'DblWidthChar']);
# All CJK ... Yi
$table->augment(0x2E80, 0xA4CF, ['DblWidthChar']);
# All CJK Compatibility Forms
$table->augment(0xF900, 0xFAFF, ['DblWidthChar']);
$table->augment(0xFE30, 0xFE6F, ['DblWidthChar']);
# All Fullwidth forms
$table->augment(0xFF01, 0xFF60, ['DblWidthChar']);

# Make a hole
$table->remove(0x1FA96, 0x1FFFD);
# Plug a hole
$table->augment(0x2FFFE, 0x2FFFF, ['DblWidthChar']);

sub handle_hole {
    my ($start, $end) = @_;
    $table->remove($start, $end);
    $table->bridge($start, $end);
}
parse_holes('UnicodeData.txt', \&handle_hole);

#
##
### Print 1
##
#
$table->print('codepoint_t', 's_single_ambig_data', "\n");
$emoji->print_set('codepoint_t', 's_emoji_data', "\n");

#
##
### Double-ambig
##
#
parse_file('EastAsianWidth.txt',
           sub { return $_[0] eq 'A' },
           sub { $table->augment($_[0], $_[1], ['DblWidthChar']) });

$table->emplace(0xF0000, 0xF4FFF, ['DblWidthChar']);
$table->emplace(0xF6000, 0x10FFFD, ['DblWidthChar']);
parse_holes('UnicodeData.txt', \&handle_hole);

#
##
### Print 2
##
#
$table->print('codepoint_t', 's_double_ambig_data', "\n");
$hangul->print('uint16_t', 's_hangul_data', '');

# Copyright © 2019 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

package Unitable;

use warnings;
use strict;

sub flagseq {
    my ($a, $b) = @_;
    return 0 if @$a != @$b;
    for (my $i = 0; $i < @$a; ++$i) {
        return 0 if $$a[$i] ne $$b[$i];
    }
    return 1;
}

sub flagsadd {
    my ($a, $b) = @_;
    foreach my $flag (@$b) {
        # Invalid flag combinations
        next if ($flag eq 'DblWidthChar') && grep($_ eq 'GcbCombining', @$a);
        next if ($flag eq 'DblWidthChar') && grep($_ eq 'GcbEmojiModifier', @$a);
        next if ($flag eq 'DblWidthChar') && grep($_ eq 'GcbTextModifier', @$a);
        # Dupe check
        push @$a, $flag unless grep($_ eq $flag, @$a);
    }
    return $a;
}

sub flagssum {
    my ($a, $b) = @_;
    return flagsadd([ @$a ], $b);
}

sub flagsstr {
    my ($a, $b) = @_;
    if (defined($b)) {
        return sprintf "%s -> %s", join('|', @$a), join('|', @$b);
    } else {
        return sprintf "%s", join('|', @$a);
    }
}

sub new {
    my ($class, $name) = @_;

    my $self = {};
    $$self{RANGES} = [];
    bless $self, $class;
    return $self;
}

sub merge {
    my ($self) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges - 3; $i += 3) {
        if ($$ranges[$i + 1] == $$ranges[$i + 3] - 1 && flagseq($$ranges[$i + 2], $$ranges[$i + 5])) {
            splice @$ranges, $i + 1, 3;
            $i -= 3;
        }
    }
}

sub emplace {
    my ($self, $start, $end, $flags) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 3) {
        if ($$ranges[$i] <= $end && $start <= $$ranges[$i + 1]) {
            printf STDERR "emplace: range 0x%x-0x%x exists! %s\n",
                $start, $end, flagsstr($$ranges[$i + 2], $flags);
            die;
        }
        last if $$ranges[$i] > $end;
    }

    for (my $i = 0; $i < @$ranges; $i += 3) {
        if ($$ranges[$i] > $end) {
            splice @$ranges, $i, 0, $start, $end, $flags;
            return $self->merge();
        }
    }

    push @$ranges, $start, $end, $flags;
}

sub remove {
    my ($self, $start, $end) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 3) {
        if ($$ranges[$i] >= $start && $$ranges[$i + 1] <= $end) {
            splice @$ranges, $i, 3;
            $i -= 3;
            next;
        }
        if ($$ranges[$i] < $start && $$ranges[$i + 1] > $end) {
            # Split range
            splice @$ranges, $i, 3,
                $$ranges[$i], $start - 1, [ @{$$ranges[$i + 2]} ],
                $end + 1, $$ranges[$i + 1], [ @{$$ranges[$i + 2]} ];
            last;
        }
        if ($$ranges[$i] <= $end && $start <= $$ranges[$i + 1]) {
            if ($$ranges[$i] < $start) {
                $$ranges[$i + 1] = $start - 1;
                next;
            } else {
                $$ranges[$i] = $end + 1;
                next;
            }
        }
        last if $$ranges[$i] > $end;
    }
}

sub augment_broken {
    my ($self, $start, $end, $flags) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 3) {
        if ($$ranges[$i] >= $start && $$ranges[$i + 1] <= $end) {
            flagsadd($$ranges[$i + 2], $flags);
            next;
        }
        if ($$ranges[$i] < $start && $$ranges[$i + 1] > $end) {
            # Split range
            splice @$ranges, $i, 3,
                $$ranges[$i], $start - 1, [ @{$$ranges[$i + 2]} ],
                $start, $end, flagssum($$ranges[$i + 2], $flags),
                $end + 1, $$ranges[$i + 1], [ @{$$ranges[$i + 2]} ];
            last;
        }
        if ($$ranges[$i] <= $end && $start <= $$ranges[$i + 1]) {
            if ($$ranges[$i] < $start) {
                my $prevend = $$ranges[$i + 1];
                $$ranges[$i + 1] = $start - 1;
                splice @$ranges, $i + 3, 3, $start, $prevend, flagssum($$ranges[$i + 2], $flags);
                if ($end > $prevend) {
                    $self->augment($prevend + 1, $end, $flags);
                }
                last;
            } else {
                my $prevstart = $$ranges[$i];
                $$ranges[$i] = $end + 1;
                splice @$ranges, $i, 3, $prevstart, $end, flagssum($$ranges[$i + 2], $flags);
                if ($start < $prevstart) {
                    splice @$ranges, $i, 3, $start, $prevstart - 1, $flags;
                }
                last;
            }
        }
        if ($$ranges[$i] > $end) {
            splice @$ranges, $i, 3, $start, $end, $flags;
            last;
        }
    }

    push @$ranges, $start, $end, $flags if $$ranges[@$ranges - 2] < $start;
    $self->merge();
}

sub lookup {
    my ($self, $elt) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 3) {
        return $$ranges[$i + 2] if $$ranges[$i] <= $elt && $$ranges[$i + 1] >= $elt;
        last if $$ranges[$i] > $elt;
    }

    return [];
}

sub augment {
    my ($self, $start, $end, $flags) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = $start; $i <= $end; ++$i) {
        my $curflags = $self->lookup($i);
        my $nextflags = flagssum($curflags, $flags);
        $self->remove($i, $i);
        $self->emplace($i, $i, $nextflags);
    }
}

sub replace {
    my ($self, $start, $end, $flags) = @_;

    $self->remove($start, $end);
    $self->emplace($start, $end, $flags);
}

sub bridge {
    my ($self, $start, $end) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges - 3; $i += 3) {
        if ($$ranges[$i + 1] == $start - 1 && $$ranges[$i + 3] == $end + 1) {
            if (flagseq($$ranges[$i + 2], $$ranges[$i + 5])) {
                splice @$ranges, $i + 1, 3;
                return;
            }
        }
        last if $$ranges[$i] > $end;
    }
}

sub print {
    my ($self, $type, $varname, $suffix) = @_;
    my $ranges = $$self{RANGES};

    print "static const $type ${varname}[] = {\n";

    for (my $i = 0; $i < @$ranges; $i += 3) {
        my $flags = join('|', @{$$ranges[$i + 2]});
        if ($$ranges[$i] > 0xffff) {
            printf "    0x%5X, 0x%5X, %s,", $$ranges[$i], $$ranges[$i + 1], $flags;
        } else {
            printf "    0x%04X, 0x%04X, %s,", $$ranges[$i], $$ranges[$i + 1], $flags;
        }

        print "\n" if ($i + 3 < @$ranges);
    }

    print "\n};\n", $suffix;
}

1;

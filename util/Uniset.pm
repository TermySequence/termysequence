# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

package Uniset;

sub new {
    my ($class, $name) = @_;

    my $self = {};
    $$self{NAME} = $name;
    $$self{CURRENT_BOUND} = -999;
    $$self{LAST_UNCHECKED} = 0;
    $$self{RANGES} = [];
    bless $self, $class;
    return $self;
}

sub append {
    my ($self, $elt) = @_;
    my $ranges = $$self{RANGES};

    if ($$self{CURRENT_BOUND} == $elt - 1) {
        # Add to current range
        $$ranges[@$ranges - 1] = $elt;
    } else {
        # Start new range
        push @$ranges, $elt, $elt;
    }

    $$self{CURRENT_BOUND} = $elt;
}

sub append_checked {
    my ($self, $elt, $func, $arg, $defarg) = @_;

    # Check previously unchecked elements
    my $last = $$self{LAST_UNCHECKED};
    while ($last < $elt) {
        if (&{$func}($last, $defarg)) {
            $self->append($last);
        }
        ++$last;
    }

    # Check this element
    if (&{$func}($elt, $arg)) {
        $self->append($elt);
    }

    $$self{LAST_UNCHECKED} = $elt + 1;
}

sub insert_range {
    my ($self, $start, $end) = @_;
    my $ranges = $$self{RANGES};

    my $soff = -1, $eoff = -1;

    for (my $i = 0; $i < @$ranges; $i += 2) {
        if ($$ranges[$i] <= $start && $$ranges[$i + 1] >= $start) {
            $soff = $i;
            $start = $$ranges[$i];
        }
        elsif ($$ranges[$i] > $start && $soff == -1) {
            $soff = $i;
        }
        if ($$ranges[$i] <= $end && $$ranges[$i + 1] >= $end) {
            $eoff = $i + 2;
            $end = $$ranges[$i + 1];
        }
        elsif ($$ranges[$i] > $end && $eoff == -1) {
            $eoff = $i;
        }
    }
    $soff = @$ranges if $soff == -1;
    $eoff = @$ranges if $eoff == -1;

    splice @$ranges, $soff, $eoff - $soff, $start, $end;
}

sub insert_uniset {
    my ($self, $other) = @_;
    my $ranges = $$other{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 2) {
        $self->insert_range($$ranges[$i], $$ranges[$i + 1]);
    }
}

sub tighten {
    my ($self, $elt) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 2) {
        if ($$ranges[$i] == $elt && $$ranges[$i + 1] == $elt) {
            splice @$ranges, $i, 2;
            return 1;
        }
        elsif ($$ranges[$i] == $elt) {
            ++$$ranges[$i];
            return 1;
        }
        elsif ($$ranges[$i + 1] == $elt) {
            --$$ranges[$i + 1];
            return 1;
        }
    }
}

sub for_each {
    my ($self, $obj, $func) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 2) {
        for (my $elt = $$ranges[$i]; $elt <= $$ranges[$i + 1]; ++$elt) {
            (&{$func}($obj, $elt));
        }
    }
}

sub for_each_backwards {
    my ($self, $obj, $func) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = @$ranges - 2; $i >= 0; $i -= 2) {
        for (my $elt = $$ranges[$i + 1]; $elt >= $$ranges[$i]; --$elt) {
            (&{$func}($obj, $elt));
        }
    }
}

sub tighten_uniset {
    my ($self, $other) = @_;

    # First add all the elements of the other set
    $self->insert_uniset($other);
    # Tighten forward
    $other->for_each($self, \&tighten);
    # Tighten backward
    $other->for_each_backwards($self, \&tighten);
}

sub contains {
    my ($self, $elt) = @_;
    my $ranges = $$self{RANGES};

    for (my $i = 0; $i < @$ranges; $i += 2) {
        return 1 if $elt >= $$ranges[$i] && $elt <= $$ranges[$i + 1];
    }

    return 0;
}

sub contains_uniset {
    my ($self, $other) = @_;
    my $iranges = $$self{RANGES};
    my $oranges = $$other{RANGES};
    my $ipos = 0;

    for (my $i = 0; $i < @$oranges; $i += 2) {
        while ($ipos < @$iranges && $$iranges[$ipos + 1] < $$oranges[$i]) {
            $ipos += 2;
        }
        if ($ipos == @$iranges) {
            return 0;
        }
        if ($$oranges[$i] < $$iranges[$ipos] || $$oranges[$i + 1] > $$iranges[$ipos + 1]) {
            # printf STDERR "At %x,%x\n", $$iranges[$ipos], $$iranges[$ipos + 1];
            # printf STDERR "Mismatch on %x,%x\n", $$oranges[$i], $$oranges[$i + 1];
            return 0;
        }
    }

    return 1;
}

sub intersection {
    my ($self, $other, $name) = @_;
    my $ranges = $$self{RANGES};
    my $result = Uniset->new($name);

    for (my $i = 0; $i < @$ranges; $i += 2) {
        for (my $elt = $$ranges[$i]; $elt <= $$ranges[$i + 1]; ++$elt) {
            $result->append($elt) if $other->contains($elt);
        }
    }

    return $result;
}

sub print_summary {
    my ($self) = @_;
    my $ranges = $$self{RANGES};
    my $count = 0;
    my $size = 0;

    for (my $i = 0; $i < @$ranges; $i += 2) {
        $size += $$ranges[$i + 1] - $$ranges[$i] + 1;
        ++$count;
    }

    printf "%s: %d elements, %d ranges\n", $$self{NAME}, $size, $count;
}

sub print {
    my ($self, $varname, $suffix) = @_;
    my $ranges = $$self{RANGES};

    print "static const Tsq::Uniset s_$varname = {\n";

    for (my $i = 0; $i < @$ranges; $i += 2) {
        if ($$ranges[$i] > 0xffff) {
            printf "{ 0x%5X, 0x%5X },", $$ranges[$i], $$ranges[$i + 1];
        } else {
            printf "{ 0x%04X, 0x%04X },", $$ranges[$i], $$ranges[$i + 1];
        }

        print "\n" if ($i + 2 < @$ranges);
    }

    print "\n};\n", $suffix;
}

1;

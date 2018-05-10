#!/usr/bin/perl
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

use warnings;
use strict;

use IO::Socket::UNIX;
use UUID::Tiny ':std';
use FindBin;
use lib "$FindBin::Bin";
use TermyClient;

my ($sd, $c, $t);
$| = 1;

use constant SOCK_PATH => "/tmp/termy-serverd${<}/sock";

#
## Commands
#
sub client_announce {
    my %attrs = (
        host => 'soakhost',
        name => '5.6.7.8',
        product => 'soaktest.pl 1-RC',
        started => '0',
        user => 'soaktest',
        userfull => 'Soak Tester'
    );

    $c->client_announce(\%attrs);
}

sub term_create {
    my $uuid = $c->serverid();
    $c->term_create($uuid);
}

sub term_send_input {
    my $input = shift;
    $c->term_send_input($t, $input);
}

sub term_get_basic {
    $c->term_get_basic($t);
}

sub term_get_attr {
    my $input = shift;
    $c->term_get_attr($t, $input);
}

sub term_set_attr {
    my ($key, $value) = @_;
    $c->term_set_attr($t, $key, $value);
}

sub term_del_attr {
    my $input = shift;
    $c->term_del_attr($t, $input);
}

sub term_resize {
    my ($w, $h) = @_;
    $c->term_resize($t, $w, $h);
}

sub term_caporder {
    my $caporder = shift;
    $c->term_caporder($t, $caporder);
}

sub term_fetch {
    my ($start, $end) = @_;
    $c->term_fetch($t, $start, $end);
}

sub term_disconnect {
    $c->term_disconnect($t);
}

sub term_close {
    $c->term_close($t);
}

sub term_owner {
    $c->term_owner($t);
}

sub term_scroll_lock {
    $c->term_scroll_lock($t);
}

sub term_signal {
    my $sig = shift;
    $c->term_signal($t, $sig);
}

sub server_get_basic {
    $c->server_get_basic($c->serverid);
}

sub server_get_attr {
    my $input = shift;
    $c->server_get_attr($c->serverid, $input);
}

sub server_set_attr {
    my ($key, $value) = @_;
    $c->server_set_attr($c->serverid, $key, $value);
}

sub server_del_attr {
    my $input = shift;
    $c->server_del_attr($c->serverid, $input);
}

sub server_get_time {
    $c->server_get_time($c->serverid);
}

#
## Term Responses
#
sub handle_term_announce {
    my $args = shift;
    my $term = $$args{term};

    $t = $term;
}

sub handle_term_closed {
    my $args = shift;
    my $term = $$args{term};

    return defined($t) && $t eq $term;
}

#
## Setup Handling
#
sub setup_callback {
    my ($type, $args) = @_;

    if ($type == TSQ_ANNOUNCE_TERM) {
        handle_term_announce($args);
        return 0;
    }

    return 1;
}

sub setup {
    my $rin = '';
    vec($rin, fileno($sd), 1) = 1;

    my $rout;
    my $rc = select($rout=$rin, undef, undef, undef);
    die "select: $!\n" unless defined($rc);

    return $c->handle_socket if vec($rout, fileno($sd), 1);
    return 1;
}

#
# Runtime handling
#
sub run_callback {
    my ($type, $args) = @_;

    if ($type == TSQ_DISCONNECT) {
        my $code = $$args{reason};
        printf("Received disconnect (code $code)\n");
        return 0;
    }
    if ($type == TSQ_REMOVE_TERM && handle_term_closed($args)) {
        printf("Terminal was closed\n");
        return 0;
    }

    return 1;
}

my $altbuf = 0;

sub launch_command {
    while (1) {
        my $rando = int(rand(7));

        if ($rando == 0) {
            # Big output command
            next if $altbuf;
            my $times = 1 + int(rand(10));
            my $input = "ls -l /bin/; " x $times;
            term_send_input($input . "\x0d");
            last;
        }
        if ($rando == 1) {
            # Press enter a lot
            next if $altbuf;
            my $times = 1 + int(rand(100));
            my $input = "\x0d" x $times;
            term_send_input($input);
            last;
        }
        if ($rando == 2) {
            # Resize terminal
            my $cols = 20 + int(rand(181));
            my $rows = 10 + int(rand(91));
            term_resize($cols, $rows);
            last;
        }
        if ($rando == 3) {
            # Adjust scrollback
            my $caporder = int(rand(21));
            term_caporder($caporder);
            last;
        }
        if ($rando == 4) {
            # Enter alt buf
            next if $altbuf;
            my $input = "man 2 read\x0d";
            term_send_input($input);
            $altbuf = 1;
            last;
        }
        if ($rando == 5) {
            # Exit alt buf
            next if !$altbuf;
            my $input = "q\x0d";
            term_send_input($input);
            $altbuf = 0;
            last;
        }
        if ($rando == 6) {
            # Fetch some rows
            last;
        }
    }
}

sub run {
    my $rin = '';
    vec($rin, 0, 1) = 1;
    vec($rin, fileno($sd), 1) = 1;

    my $rout;
    my $rc = select($rout=$rin, undef, undef, 0.5);
    die "select: $!\n" unless defined($rc);

    return 0 if vec($rout, 0, 1);
    return $c->handle_socket if vec($rout, fileno($sd), 1);

    launch_command();
    return 1;
}

$sd = IO::Socket::UNIX->new( Type => SOCK_STREAM, Peer => SOCK_PATH );
die "connect: $!\n" unless defined($sd);
$sd->autoflush(1);
binmode($sd, ':raw');

$c = TermyClient->new($sd, \&setup_callback);
printf "I am client %s\n", uuid_to_string($c->id());
$c->perform_handshake();
client_announce;
term_create;

1 while setup;
printf "I am term %s\n", uuid_to_string($t);
$c->set_callback(\&run_callback);
1 while run;

close($sd);
exit;

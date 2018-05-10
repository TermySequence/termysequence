# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

package TermyClient;

use warnings;
use strict;
use Carp;
use base 'Exporter';
use UUID::Tiny ':std';

use constant TSQ_PROTOCOL_VERSION => 1;
use constant TSQ_PROTOCOL_REJECT => 0;
use constant TSQ_PROTOCOL_TERM => 1;
use constant TSQ_PROTOCOL_RAW => 2;
use constant TSQ_PROTOCOL_CLIENTFD => 3;
use constant TSQ_PROTOCOL_SERVER => 4;
use constant TSQ_PROTOCOL_SERVERFD => 5;

use constant TSQ_TASK_RUNNING => 0;
use constant TSQ_TASK_STARTING => 1;
use constant TSQ_TASK_ACKING => 2;
use constant TSQ_TASK_FINISHED => 3;
use constant TSQ_TASK_ERROR => 4;

use constant TSQ_HANDSHAKE_COMPLETE => 0x1;
use constant TSQ_ANNOUNCE_SERVER => 0x2;
use constant TSQ_ANNOUNCE_TERM => 0x3;
use constant TSQ_ANNOUNCE_CONN => 0x4;
use constant TSQ_DISCONNECT => 0x5;
use constant TSQ_KEEPALIVE => 0x6;
use constant TSQ_CONFIGURE_KEEPALIVE => 0x7;

use constant TSQ_GET_SERVER_TIME => 0x10003e8;
use constant TSQ_GET_SERVER_TIME_RESPONSE => 0x20003e8;
use constant TSQ_GET_SERVER_ATTRIBUTES => 0x10003e9;
use constant TSQ_GET_SERVER_ATTRIBUTES_RESPONSE => 0x20003e9;
use constant TSQ_GET_SERVER_ATTRIBUTE => 0x10003ea;
use constant TSQ_GET_SERVER_ATTRIBUTE_RESPONSE => 0x20003ea;
use constant TSQ_SET_SERVER_ATTRIBUTE => 0x10003eb;
use constant TSQ_REMOVE_SERVER_ATTRIBUTE => 0x10003ec;
use constant TSQ_REMOVE_SERVER => 0x10003ed;
use constant TSQ_CREATE_TERM => 0x10003ee;
use constant TSQ_TASK_INPUT => 0x10003f0;
use constant TSQ_TASK_OUTPUT => 0x20003f0;
use constant TSQ_TASK_ANSWER => 0x10003f1;
use constant TSQ_TASK_QUESTION => 0x20003f1;
use constant TSQ_CANCEL_TASK => 0x10003f2;
use constant TSQ_UPLOAD_FILE => 0x10003f3;
use constant TSQ_DOWNLOAD_FILE => 0x10003f4;
use constant TSQ_DELETE_FILE => 0x10003f5;
use constant TSQ_RENAME_FILE => 0x10003f6;
use constant TSQ_UPLOAD_PIPE => 0x10003f7;
use constant TSQ_DOWNLOAD_PIPE => 0x10003f8;
use constant TSQ_CONNECTING_PORTFWD => 0x10003f9;
use constant TSQ_LISTENING_PORTFWD => 0x10003fa;
use constant TSQ_RUN_COMMAND => 0x10003fb;
use constant TSQ_RUN_CONNECT => 0x10003fc;
use constant TSQ_MOUNT_FILE_READWRITE => 0x10003fd;
use constant TSQ_MOUNT_FILE_READONLY => 0x10003fe;
use constant TSQ_MONITOR_INPUT => 0x10003ff;

use constant TSQ_ANNOUNCE_CLIENT => 0x20007d0;
use constant TSQ_REMOVE_CLIENT => 0x20007d1;
use constant TSQ_GET_CLIENT_ATTRIBUTE => 0x10007d2;
use constant TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE => 0x20007d2;
use constant TSQ_THROTTLE_PAUSE => 0x20007d5;
use constant TSQ_THROTTLE_RESUME => 0x30007d5;

use constant TSQ_INPUT => 0x3000bb8;
use constant TSQ_BEGIN_OUTPUT => 0x3000bb8;
use constant TSQ_BEGIN_OUTPUT_RESPONSE => 0x2000bb8;
use constant TSQ_FLAGS_CHANGED => 0x3000bb9;
use constant TSQ_BUFFER_CAPACITY => 0x3000bba;
use constant TSQ_BUFFER_LENGTH => 0x3000bbb;
use constant TSQ_BUFFER_SWITCHED => 0x3000bbc;
use constant TSQ_SIZE_CHANGED => 0x3000bbd;
use constant TSQ_CURSOR_MOVED => 0x3000bbe;
use constant TSQ_BELL_RANG => 0x3000bbf;
use constant TSQ_ROW_CONTENT => 0x3000bc0;
use constant TSQ_ROW_CONTENT_RESPONSE => 0x2000bc0;
use constant TSQ_REGION_UPDATE => 0x3000bc1;
use constant TSQ_REGION_UPDATE_RESPONSE => 0x2000bc1;
use constant TSQ_DIRECTORY_UPDATE => 0x3000bc2;
use constant TSQ_FILE_UPDATE => 0x3000bc3;
use constant TSQ_FILE_REMOVED => 0x3000bc4;
use constant TSQ_END_OUTPUT => 0x3000bc5;
use constant TSQ_END_OUTPUT_RESPONSE => 0x2000bc5;
use constant TSQ_MOUSE_MOVED => 0x3000bc6;
use constant TSQ_IMAGE_CONTENT => 0x3000bc7;
use constant TSQ_IMAGE_CONTENT_RESPONSE => 0x2000bc7;
use constant TSQ_DOWNLOAD_IMAGE => 0x3000bc8;

use constant TSQ_GET_TERM_ATTRIBUTES => 0x3000c1c;
use constant TSQ_GET_TERM_ATTRIBUTES_RESPONSE => 0x2000c1c;
use constant TSQ_GET_CONN_ATTRIBUTES_RESPONSE => 0x2000c1d;
use constant TSQ_GET_TERM_ATTRIBUTE => 0x3000c1d;
use constant TSQ_GET_CONN_ATTRIBUTE => 0x3000c1e;
use constant TSQ_GET_TERM_ATTRIBUTE_RESPONSE => 0x2000c1f;
use constant TSQ_GET_CONN_ATTRIBUTE_RESPONSE => 0x2000c20;
use constant TSQ_SET_TERM_ATTRIBUTE => 0x3000c1e;
use constant TSQ_REMOVE_TERM_ATTRIBUTE => 0x3000c1f;
use constant TSQ_RESIZE_TERM => 0x3000c20;
use constant TSQ_REMOVE_TERM => 0x3000c21;
use constant TSQ_REMOVE_CONN => 0x3000c22;
use constant TSQ_DUPLICATE_TERM => 0x3000c22;
use constant TSQ_RESET_TERM => 0x3000c23;
use constant TSQ_CHANGE_OWNER => 0x3000c24;
use constant TSQ_REQUEST_DISCONNECT => 0x3000c25;
use constant TSQ_TOGGLE_SOFT_SCROLL_LOCK => 0x3000c26;
use constant TSQ_SEND_SIGNAL => 0x3000c27;

use constant TSQ_CREATE_REGION => 0x3000c80;
use constant TSQ_GET_REGION => 0x3000c81;
use constant TSQ_REMOVE_REGION => 0x3000c82;

#
## Parsing
#
sub parse_uuid {
    my $buf = shift;
    return substr($$buf, 0, 16, '');
}

sub parse_string {
    my $buf = shift;
    return undef unless length($$buf);

    my $str = unpack('Z*', $$buf);

    substr($$buf, 0, length($str) + 1) = '';
    return $str;
}

sub parse_bytes {
    my $buf = shift;
    my $count = shift;
    my $str;
    return '' unless length($$buf);

    if (defined($count)) {
        $str = unpack("a$count", $$buf);
    } else {
        $str = unpack('a*', $$buf);
    }

    substr($$buf, 0, length($str)) = '';
    $str =~ tr/\x00/ /;
    return $str;
}

sub parse_number {
    my ($buf, $defval) = @_;
    return $defval unless length($$buf) >= 4;

    my $number = unpack('V', $$buf);

    substr($$buf, 0, 4) = '';
    return $number;
}

sub parse_number64 {
    my $buf = shift;
    return undef unless length($$buf) >= 8;

    my ($lo, $hi) = unpack('VV', $$buf);

    substr($$buf, 0, 8) = '';
    return ($hi << 32) | $lo;
}

sub parse_range {
    my $buf = shift;
    return undef unless length($$buf) >= 24;

    my @range = unpack('VVVVVV', $$buf);

    substr($$buf, 0, 24) = '';
    return @range;
}

sub syswrite_padded {
    my ($self, $buf) = @_;
    my $sd = $$self{sd};
    my $length = length($buf);
    my $wrote = 0;

    my $padding = ($length % 4) ? (4 - ($length % 4)) : 0;
    $wrote = syswrite($sd, $buf) or die;

    while ($padding-- > 0) {
        $wrote += syswrite($sd, '\x00', 1) or die;
    }

    # printf("Wrote %u bytes\n", $wrote);
}

sub dumpbytes {
    my $body = shift;
    my @bytes = unpack('C*', $body);

    for (@bytes) {
        if ($_ >= 32 && $_ < 127) {
            printf STDERR "%s ", chr($_);
        } else {
            printf STDERR "(%x) ", $_;
        }
    }

    print STDERR "\n";
}

#
## Commands
#
sub client_announce {
    my ($self, $attrs) = @_;
    my $buf = pack('a16VVV', $$self{id}, 1, 0, 0);

    foreach my $key (keys %$attrs) {
        $buf .= pack('Z*Z*', $key, $$attrs{$key});
    }

    $buf = pack('VV', TSQ_ANNOUNCE_CLIENT, length($buf)) . $buf;
    syswrite_padded($self, $buf);
}

sub client_keepalive {
    my ($self) = @_;
    my $buf = pack('Vx[V]', TSQ_KEEPALIVE);
    syswrite($$self{sd}, $buf) or die;
}

sub client_set_keepalive {
    my ($self, $time) = @_;
    my $buf = pack('VVV', TSQ_CONFIGURE_KEEPALIVE, 4, $time);
    syswrite($$self{sd}, $buf) or die;
}

sub term_create {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $termid = create_uuid(UUID_RANDOM);
    my $buf = pack('VVa16a16a16VV', TSQ_CREATE_TERM, 56, $uuid, $id, $termid, 80, 24);
    syswrite_padded($self, $buf);
}

sub term_send_input {
    my ($self, $uuid, $input) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_INPUT, 32 + length($input), $uuid, $id, $input);
    syswrite_padded($self, $buf);
}

sub term_get_attrs {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_GET_TERM_ATTRIBUTES, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub term_get_attr {
    my ($self, $uuid, $key) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_GET_TERM_ATTRIBUTE, 32 + length($key), $uuid, $id, $key);
    syswrite_padded($self, $buf);
}

sub term_set_attr {
    my ($self, $uuid, $key, $value) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*xa*', TSQ_SET_TERM_ATTRIBUTE, 33 + length($key) + length($value), $uuid, $id, $key, $value);
    syswrite_padded($self, $buf);
}

sub term_del_attr {
    my ($self, $uuid, $key) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_REMOVE_TERM_ATTRIBUTE, 32 + length($key), $uuid, $id, $key);
    syswrite_padded($self, $buf);
}

sub term_resize {
    my ($self, $uuid, $w, $h) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16VV', TSQ_RESIZE_TERM, 40, $uuid, $id, $w, $h);
    syswrite($$self{sd}, $buf) or die;
}

sub term_caporder {
    my ($self, $uuid, $c) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16V', TSQ_BUFFER_CAPACITY, 36, $uuid, $id, $c << 8);
    syswrite($$self{sd}, $buf) or die;
}

sub term_fetch {
    my ($self, $uuid, $start, $end) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16Vx[V]Vx[VV]', TSQ_ROW_CONTENT, 52, $uuid, $id, $start, $end);
    syswrite($$self{sd}, $buf) or die;
}

sub term_disconnect {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_REQUEST_DISCONNECT, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub term_close {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_REMOVE_TERM, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub term_set_owner {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_CHANGE_OWNER, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub term_scroll_lock {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_TOGGLE_SOFT_SCROLL_LOCK, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub term_signal {
    my ($self, $uuid, $sig) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16V', TSQ_SEND_SIGNAL, 36, $uuid, $id, $sig);
    syswrite($$self{sd}, $buf) or die;
}

sub term_get_content {
    my ($self, $uuid, $cid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16VV', TSQ_IMAGE_CONTENT, 40, $uuid, $id, $cid, $cid >> 32);
    syswrite($$self{sd}, $buf) or die;
}

sub term_download_content {
    my ($self, $uuid, $task, $cid, $chunksize, $winsize) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16VVVV', TSQ_DOWNLOAD_IMAGE, 64, $uuid, $id, $task,
                   $cid, $cid >> 32, $chunksize, $winsize);
    syswrite_padded($self, $buf);
}

sub server_get_attrs {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_GET_SERVER_ATTRIBUTES, 32, $uuid, $id);
    syswrite($$self{sd}, $buf) or die;
}

sub server_get_attr {
    my ($self, $uuid, $key) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_GET_SERVER_ATTRIBUTE, 32 + length($key), $uuid, $id, $key);
    syswrite_padded($self, $buf);
}

sub server_set_attr {
    my ($self, $uuid, $key, $value) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*xa*', TSQ_SET_SERVER_ATTRIBUTE, 33 + length($key) + length($value), $uuid, $id, $key, $value);
    syswrite_padded($self, $buf);
}

sub server_del_attr {
    my ($self, $uuid, $key) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_REMOVE_SERVER_ATTRIBUTE, 32 + length($key), $uuid, $id, $key);
    syswrite_padded($self, $buf);
}

sub server_get_time {
    my ($self, $uuid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16', TSQ_GET_SERVER_TIME, 32, $uuid, $id);
    syswrite_padded($self, $buf);
}

sub server_task_answer {
    my ($self, $uuid, $task, $answer) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16V', TSQ_TASK_ANSWER, 52, $uuid, $id, $task, $answer);
    syswrite_padded($self, $buf);
}

sub server_task_input {
    my ($self, $uuid, $task, $data) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16a*', TSQ_TASK_INPUT, 48 + length($data), $uuid, $id, $task, $data);
    syswrite_padded($self, $buf);
}

sub server_upload_file {
    my ($self, $uuid, $task, $chunksize, $mode, $config, $dst) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16VVVa*', TSQ_UPLOAD_FILE, 60 + length($dst), $uuid, $id, $task,
                   $chunksize, $mode, $config, $dst);
    syswrite_padded($self, $buf);
}

sub server_download_file {
    my ($self, $uuid, $task, $chunksize, $winsize, $src) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16VVa*', TSQ_DOWNLOAD_FILE, 56 + length($src), $uuid, $id, $task,
                   $chunksize, $winsize, $src);
    syswrite_padded($self, $buf);
}

sub server_remove_file {
    my ($self, $uuid, $task, $config, $dst) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16Va*', TSQ_DELETE_FILE, 52 + length($dst), $uuid, $id, $task,
                   $config, $dst);
    syswrite_padded($self, $buf);
}

sub server_rename_file {
    my ($self, $uuid, $task, $config, $src, $dst) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16VZ*Z*', TSQ_RENAME_FILE, 54 + length($src) + length($dst),
                   $uuid, $id, $task, $config, $src, $dst);
    syswrite_padded($self, $buf);
}

sub server_run_command {
    my ($self, $uuid, $task, $chunksize, $winsize, $cmdline) = @_;
    my $id = $$self{id};
    my $length = 64 + length($cmdline);
    my $buf = pack('VVa16a16a16VVZ*a*', TSQ_RUN_COMMAND, $length, $uuid, $id, $task,
                   $chunksize, $winsize, 'command', $cmdline);
    syswrite_padded($self, $buf);
}

sub server_run_connect {
    my ($self, $uuid, $task, $cmdline) = @_;
    my $id = $$self{id};
    my $length = 56 + length($cmdline);
    my $buf = pack('VVa16a16a16Z*a*', TSQ_RUN_CONNECT, $length, $uuid, $id, $task,
                   'command', $cmdline);
    syswrite_padded($self, $buf);
}

sub server_monitor_input {
    my ($self, $uuid, $input) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a*', TSQ_MONITOR_INPUT, 32 + length($input), $uuid, $id, $input);
    syswrite_padded($self, $buf);
}

sub server_get_client_attr {
    my ($self, $serverid, $clientid, $key) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16a16a*', TSQ_GET_CLIENT_ATTRIBUTE, 48 + length($key),
                   $serverid, $id, $clientid, $key);
    syswrite_padded($self, $buf);
}

sub region_create_region {
    my ($self, $uuid, $bufid, $type, $srow, $erow, $scol, $ecol) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16VVVVVVVV', TSQ_CREATE_REGION, 64, $uuid, $id, $bufid,
                   $type, $srow, $srow >> 32, $erow, $erow >> 32, $scol, $ecol);
    syswrite_padded($self, $buf);
}

sub region_get_region {
    my ($self, $uuid, $bufid, $regid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16VV', TSQ_GET_REGION, 40, $uuid, $id, $bufid, $regid);
    syswrite($$self{sd}, $buf) or die;
}

sub region_remove_region {
    my ($self, $uuid, $bufid, $regid) = @_;
    my $id = $$self{id};
    my $buf = pack('VVa16a16VV', TSQ_REMOVE_REGION, 40, $uuid, $id, $bufid, $regid);
    syswrite_padded($self, $buf);
}

#
## Server Responses
#
sub client_check {
    my ($client, $id) = @_;
    unless ($client eq $id) {
        croak "Received message addressed to incorrect client " . uuid_to_string($client);
    }
}

sub term_check {
    my ($term, $termhash) = @_;
    unless (exists($$termhash{$term})) {
        croak "Unknown terminal " . uuid_to_string($term);
    }
}

sub server_check {
    my ($serv, $servhash) = @_;
    unless (exists($$servhash{$serv})) {
        croak "Unknown server " . uuid_to_string($serv);
    }
}

sub handle_server_announce {
    my ($self, $type, $body) = @_;
    my $serv = parse_uuid(\$body);
    my $hop = parse_uuid(\$body);
    my $version = parse_number(\$body);
    my $hopcount = parse_number(\$body);
    my $termcount = parse_number(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        server => $serv,
        hop => $hop,
        version => $version,
        hopcount => $hopcount,
        termcount => $termcount,
        attrs => \%attrs
    };

    if (exists(${$$self{servhash}}{$serv})) {
        die "Duplicate server announcement for " . uuid_to_string($serv);
    }
    ${$$self{servhash}}{$serv} = 1;
    return &{$$self{callback}}($type, $hash);
}

sub handle_server_attributes {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $serv = parse_uuid(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        server => $serv,
        attrs => \%attrs
    };

    client_check($client, $$self{id});
    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_server_attribute {
    my ($self, $type, $body) = @_;
    my $serv = parse_uuid(\$body);
    my $key = parse_string(\$body);
    die "No key received" unless defined($key);
    my $value = parse_string(\$body);
    $value =~ tr/\x1f/,/ if defined($value);

    my $hash = {
        server => $serv,
        key => $key,
        value => $value
    };

    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_server_attribute_get {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);

    client_check($client, $$self{id});
    return handle_server_attribute($self, $type, $body);
}

sub handle_server_removed {
    my ($self, $type, $body) = @_;
    my $serv = parse_uuid(\$body);
    my $reason = parse_number(\$body);

    my $hash = {
        server => $serv,
        reason => $reason
    };

    server_check($serv, $$self{servhash});
    delete ${$$self{servhash}}{$serv};
    return &{$$self{callback}}($type, $hash);
}

sub handle_server_time {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $serv = parse_uuid(\$body);
    my $num = parse_number64(\$body);

    my $hash = {
        server => $serv,
        time => $num
    };

    client_check($client, $$self{id});
    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_task_question {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $serv = parse_uuid(\$body);
    my $task = parse_uuid(\$body);
    my $question = parse_number(\$body);

    my $hash = {
        server => $serv,
        task => $task,
        question => $question
    };

    client_check($client, $$self{id});
    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_task_output {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $serv = parse_uuid(\$body);
    my $task = parse_uuid(\$body);

    my $hash = {
        server => $serv,
        task => $task,
        data => $body
    };

    client_check($client, $$self{id});
    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_throttle_pause {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $dest = parse_uuid(\$body);
    my $term = parse_uuid(\$body);
    my $size = parse_number64(\$body);
    my $limit = parse_number64(\$body);

    my $hash = {
        dest => $dest,
        term => $term,
        size => $size,
        limit => $limit
    };

    client_check($client, $$self{id});
    return &{$$self{callback}}($type, $hash);
}

sub handle_throttle_resume {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);

    my $hash = {
        term => $term
    };

    return &{$$self{callback}}($type, $hash);
}

sub handle_disconnect {
    my ($self, $type, $body) = @_;

    my $hash = {
        reason => parse_number(\$body, -1)
    };

    return &{$$self{callback}}($type, $hash);
}

#
## Term Responses
#
sub handle_term_announce {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $hop = parse_uuid(\$body);
    my $hopcount = parse_number(\$body);
    my $width = parse_number(\$body);
    my $height = parse_number(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        hop => $hop,
        hopcount => $hopcount,
        width => $width,
        height => $height,
        attrs => \%attrs
    };

    if (exists(${$$self{termhash}}{$term})) {
        die "Duplicate terminal announcement for " . uuid_to_string($term);
    }
    ${$$self{termhash}}{$term} = 1;
    return &{$$self{callback}}($type, $hash);
}

sub handle_conn_announce {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $hop = parse_uuid(\$body);
    my $hopcount = parse_number(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        hop => $hop,
        hopcount => $hopcount,
        attrs => \%attrs
    };

    if (exists(${$$self{termhash}}{$term})) {
        die "Duplicate terminal announcement for " . uuid_to_string($term);
    }
    ${$$self{termhash}}{$term} = 1;
    return &{$$self{callback}}($type, $hash);
}

sub handle_term_attributes {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $term = parse_uuid(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        attrs => \%attrs
    };

    client_check($client, $$self{id});
    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_term_attribute {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $key = parse_string(\$body);
    die "No key received" unless defined($key);
    my $value = parse_string(\$body);
    $value =~ tr/\x1f/,/ if defined($value);

    my $hash = {
        term => $term,
        key => $key,
        value => $value
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_term_attribute_get {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);

    client_check($client, $$self{id});
    return handle_term_attribute($self, $type, $body);
}

sub handle_term_closed {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $reason = parse_number(\$body);

    my $hash = {
        term => $term,
        reason => $reason
    };

    term_check($term, $$self{termhash});
    delete ${$$self{termhash}}{$term};
    return &{$$self{callback}}($type, $hash);
}

sub handle_begin_output {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);

    my $hash = {
        term => $term
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_begin_output_response {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);

    client_check($client, $$self{id});
    return handle_begin_output($self, $type, $body);
}

sub handle_flags_changed {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);

    my $hash = {
        term => $term,
        flags => parse_number64(\$body)
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_buffer_capacity {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $size = parse_number64(\$body);
    my $spec = parse_number(\$body);
    my $bufid = $spec & 0xff;
    my $caporder = $spec >> 8 & 0xff;

    my $hash = {
        term => $term,
        size => $size,
        bufid => $bufid,
        caporder => $caporder
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_buffer_length {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $size = parse_number64(\$body);

    my $hash = {
        term => $term,
        size => $size,
        bufid => parse_number(\$body),
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_buffer_switched {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);

    my $hash = {
        term => $term,
        bufid => parse_number(\$body),
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_size_changed {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $width = parse_number(\$body);
    my $height = parse_number(\$body);
    my $marginx = parse_number(\$body);
    my $marginy = parse_number(\$body);
    my $marginw = parse_number(\$body);
    my $marginh = parse_number(\$body);

    my $hash = {
        term => $term,
        width => $width,
        height => $height,
        marginx => $marginx,
        marginy => $marginy,
        marginw => $marginw,
        marginh => $marginh
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_cursor_moved {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $x = parse_number(\$body);
    my $y = parse_number(\$body);
    my $pos = parse_number(\$body);
    my $spec = parse_number(\$body);
    my $subpos = $spec & 0xff;
    my $flags = $spec >> 8;

    my $hash = {
        term => $term,
        x => $x,
        y => $y,
        pos => $pos,
        subpos => $subpos,
        flags => $flags
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_bell_rang {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $code = parse_number(\$body);
    my $count = parse_number(\$body);

    my $hash = {
        term => $term,
        code => $code,
        count => $count
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_output_row {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $rownum = parse_number64(\$body);
    my $spec = parse_number(\$body);
    my $bufid = $spec & 0xff;
    my $flags = $spec >> 8;
    my $modtime = parse_number(\$body);
    my $nranges = parse_number(\$body);
    my @ranges;

    for (my $i = 0; $i < $nranges; ++$i) {
        push @ranges, [ parse_range(\$body) ];
    }

    my $hash = {
        term => $term,
        rownum => $rownum,
        bufid => $bufid,
        flags => $flags,
        modtime => $modtime,
        ranges => \@ranges,
        content => parse_bytes(\$body)
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_output_row_response {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);

    client_check($client, $$self{id});
    return handle_output_row($self, $type, $body);
}

sub handle_region_update {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $regid = parse_number(\$body);
    my $regtype = parse_number(\$body);
    my $flags = parse_number(\$body);
    my $parent = parse_number(\$body);
    my $srow = parse_number64(\$body);
    my $erow = parse_number64(\$body);
    my $scol = parse_number(\$body);
    my $ecol = parse_number(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        regid => $regid,
        type => $regtype,
        flags => $flags,
        parent => $parent,
        srow => $srow,
        erow => $erow,
        scol => $scol,
        ecol => $ecol,
        attrs => \%attrs
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_region_update_response {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);

    client_check($client, $$self{id});
    return handle_region_update($self, $type, $body);
}

sub handle_directory_update {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $mtime = parse_number64(\$body);
    my $name = parse_string(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        mtime => $mtime,
        name => $name,
        attrs => \%attrs
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_file_update {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $mtime = parse_number64(\$body);
    my $size = parse_number64(\$body);
    my $mode = parse_number(\$body);
    my $uid = parse_number(\$body);
    my $gid = parse_number(\$body);
    my $name = parse_string(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        mode => $mode,
        mtime => $mtime,
        size => $size,
        uid => $uid,
        gid => $gid,
        name => $name,
        attrs => \%attrs
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_file_removed {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $mtime = parse_number64(\$body);
    my $name = parse_string(\$body);
    my %attrs;

    while (1) {
        my $key = parse_string(\$body);
        last unless defined($key);
        my $value = parse_string(\$body);
        die "Attribute '$key' missing a value" unless defined($value);
        $value =~ tr/\x1f/,/;
        $attrs{$key} = $value;
    }

    my $hash = {
        term => $term,
        mtime => $mtime,
        name => $name,
        attrs => \%attrs
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_end_output {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);

    my $hash = {
        term => $term
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_end_output_response {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $term = parse_uuid(\$body);

    my $hash = {
        client => $client,
        term => $term
    };

    client_check($client, $$self{id});
    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_mouse_moved {
    my ($self, $type, $body) = @_;
    my $term = parse_uuid(\$body);
    my $x = parse_number(\$body);
    my $y = parse_number(\$body);

    my $hash = {
        term => $term,
        x => $x,
        y => $y
    };

    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_image_content_response {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $term = parse_uuid(\$body);
    my $id = parse_number64(\$body);

    my $hash = {
        term => $term,
        id => $id,
        data => $body
    };

    client_check($client, $$self{id});
    term_check($term, $$self{termhash});
    return &{$$self{callback}}($type, $hash);
}

sub handle_client_attribute {
    my ($self, $type, $body) = @_;
    my $client = parse_uuid(\$body);
    my $serv = parse_uuid(\$body);
    my $target = parse_uuid(\$body);
    my $key = parse_string(\$body);
    die "No key received" unless defined($key);
    my $value = parse_string(\$body);
    $value =~ tr/\x1f/,/ if defined($value);

    my $hash = {
        server => $serv,
        client => $target,
        key => $key,
        value => $value
    };

    client_check($client, $$self{id});
    server_check($serv, $$self{servhash});
    return &{$$self{callback}}($type, $hash);
}

#
## Handling
#
sub perform_handshake {
    my ($self) = @_;
    my ($sd, $id) = ($$self{sd}, $$self{id});
    my $buf;
    my $rc = sysread($sd, $buf, 48);
    die "Failed to read handshake bytes!\n" unless defined($rc) and $rc == 48;

    $$self{sversion} = int(substr($buf, 6, 1));
    $$self{pversion} = int(substr($buf, 8, 1));
    $$self{serverid} = string_to_uuid(substr($buf, 10, 36));

    $buf = "\x1b\x5d" . "511;1;2;" . uuid_to_string($id) . "\x1b\x5c";
    syswrite($sd, $buf) or die "Failed to write handshake bytes!";

    $rc = sysread($sd, $buf, 8);
    die "Failed to read ready bytes!" unless defined($rc) and $rc == 8;
    my $code = unpack('V', $buf);

    if ($code == TSQ_DISCONNECT) {
        $rc = sysread($sd, $buf, 4);
        die "Failed to read disconnect code!" unless defined($rc) and $rc == 4;
        handle_disconnect($self, $code, $buf);
        exit 2;
    }
    elsif ($code != TSQ_HANDSHAKE_COMPLETE) {
        die "Received unexpected handshake response $code!";
    }
}

sub handle_socket {
    my $self = shift;
    my $sd = $$self{sd};

    my $header = '';
    my $body = '';
    my $need = 8;
    my $padding;
    my $rc;

    while (length($header) < $need)
    {
        $rc = sysread($sd, $header, $need - length($header), length($header));
        die unless defined($rc) && $rc > 0;
    }
    my ($type, $length) = unpack('VV', $header);
    #printf("Got response %u (length %u)\n", $type, $length);

    $padding = ($length % 4) ? (4 - ($length % 4)) : 0;
    $need = $length + $padding;

    while (length($body) < $need)
    {
        $rc = sysread($sd, $body, $need - length($body), length($body));
        die unless defined($rc) && $rc > 0;
    }
    chop($body) while ($padding-- > 0);

    return handle_server_announce($self, $type, $body) if $type == TSQ_ANNOUNCE_SERVER;
    return handle_server_attributes($self, $type, $body) if $type == TSQ_GET_SERVER_ATTRIBUTES_RESPONSE;
    return handle_server_attribute($self, $type, $body) if $type == TSQ_GET_SERVER_ATTRIBUTE;
    return handle_server_attribute_get($self, $type, $body) if $type == TSQ_GET_SERVER_ATTRIBUTE_RESPONSE;
    return handle_server_removed($self, $type, $body) if $type == TSQ_REMOVE_SERVER;
    return handle_server_time($self, $type, $body) if $type == TSQ_GET_SERVER_TIME_RESPONSE;
    return handle_task_question($self, $type, $body) if $type == TSQ_TASK_QUESTION;
    return handle_task_output($self, $type, $body) if $type == TSQ_TASK_OUTPUT;
    return handle_throttle_pause($self, $type, $body) if $type == TSQ_THROTTLE_PAUSE;
    return handle_throttle_resume($self, $type, $body) if $type == TSQ_THROTTLE_RESUME;

    return handle_term_announce($self, $type, $body) if $type == TSQ_ANNOUNCE_TERM;
    return handle_conn_announce($self, $type, $body) if $type == TSQ_ANNOUNCE_CONN;
    return handle_term_attributes($self, $type, $body) if $type == TSQ_GET_TERM_ATTRIBUTES_RESPONSE;
    return handle_term_attributes($self, $type, $body) if $type == TSQ_GET_CONN_ATTRIBUTES_RESPONSE;
    return handle_term_attribute($self, $type, $body) if $type == TSQ_GET_TERM_ATTRIBUTE;
    return handle_term_attribute($self, $type, $body) if $type == TSQ_GET_CONN_ATTRIBUTE;
    return handle_term_attribute_get($self, $type, $body) if $type == TSQ_GET_TERM_ATTRIBUTE_RESPONSE;
    return handle_term_attribute_get($self, $type, $body) if $type == TSQ_GET_CONN_ATTRIBUTE_RESPONSE;
    return handle_term_closed($self, $type, $body) if $type == TSQ_REMOVE_TERM;
    return handle_term_closed($self, $type, $body) if $type == TSQ_REMOVE_CONN;
    return handle_begin_output($self, $type, $body) if $type == TSQ_BEGIN_OUTPUT;
    return handle_begin_output_response($self, $type, $body) if $type == TSQ_BEGIN_OUTPUT_RESPONSE;
    return handle_flags_changed($self, $type, $body) if $type == TSQ_FLAGS_CHANGED;
    return handle_buffer_capacity($self, $type, $body) if $type == TSQ_BUFFER_CAPACITY;
    return handle_buffer_length($self, $type, $body) if $type == TSQ_BUFFER_LENGTH;
    return handle_buffer_switched($self, $type, $body) if $type == TSQ_BUFFER_SWITCHED;
    return handle_size_changed($self, $type, $body) if $type == TSQ_SIZE_CHANGED;
    return handle_cursor_moved($self, $type, $body) if $type == TSQ_CURSOR_MOVED;
    return handle_bell_rang($self, $type, $body) if $type == TSQ_BELL_RANG;
    return handle_output_row($self, $type, $body) if $type == TSQ_ROW_CONTENT;
    return handle_output_row_response($self, $type, $body) if $type == TSQ_ROW_CONTENT_RESPONSE;
    return handle_region_update($self, $type, $body) if $type == TSQ_REGION_UPDATE;
    return handle_region_update_response($self, $type, $body) if $type == TSQ_REGION_UPDATE_RESPONSE;
    return handle_directory_update($self, $type, $body) if $type == TSQ_DIRECTORY_UPDATE;
    return handle_file_update($self, $type, $body) if $type == TSQ_FILE_UPDATE;
    return handle_file_removed($self, $type, $body) if $type == TSQ_FILE_REMOVED;
    return handle_end_output($self, $type, $body) if $type == TSQ_END_OUTPUT;
    return handle_end_output_response($self, $type, $body) if $type == TSQ_END_OUTPUT_RESPONSE;
    return handle_mouse_moved($self, $type, $body) if $type == TSQ_MOUSE_MOVED;
    return handle_image_content_response($self, $type, $body) if $type == TSQ_IMAGE_CONTENT_RESPONSE;
    return handle_client_attribute($self, $type, $body) if $type == TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE;

    return handle_disconnect($self, $type, $body) if $type == TSQ_DISCONNECT;
    return &{$$self{callback}}($type) if $type == TSQ_KEEPALIVE;
    die "Received unknown message type $type";
}

sub new {
    my ($class, $sd, $callback) = @_;
    my $self = {
        sd => $sd,
        callback => $callback,
        id => create_uuid(UUID_RANDOM),
        termhash => {},
        servhash => {}
    };

    bless $self, $class;
    return $self;
}

sub set_callback {
    my ($self, $callback) = @_;
    $$self{callback} = $callback;
}

sub id {
    my $self = shift;
    return $$self{id};
}

sub serverid {
    my $self = shift;
    return $$self{serverid};
}

sub termhash {
    my $self = shift;
    return $$self{termhash};
}

sub servhash {
    my $self = shift;
    return $$self{servhash};
}

our @EXPORT = qw/
parse_number
parse_number64
parse_string

TSQ_PROTOCOL_VERSION
TSQ_PROTOCOL_REJECT
TSQ_PROTOCOL_TERM
TSQ_PROTOCOL_RAW
TSQ_PROTOCOL_CLIENTFD
TSQ_PROTOCOL_SERVER
TSQ_PROTOCOL_SERVERFD

TSQ_TASK_RUNNING
TSQ_TASK_STARTING
TSQ_TASK_ACKING
TSQ_TASK_FINISHED
TSQ_TASK_ERROR

TSQ_HANDSHAKE_COMPLETE
TSQ_ANNOUNCE_SERVER
TSQ_ANNOUNCE_TERM
TSQ_ANNOUNCE_CONN
TSQ_DISCONNECT
TSQ_KEEPALIVE
TSQ_CONFIGURE_KEEPALIVE

TSQ_GET_SERVER_TIME
TSQ_GET_SERVER_TIME_RESPONSE
TSQ_GET_SERVER_ATTRIBUTES
TSQ_GET_SERVER_ATTRIBUTES_RESPONSE
TSQ_GET_SERVER_ATTRIBUTE
TSQ_GET_SERVER_ATTRIBUTE_RESPONSE
TSQ_SET_SERVER_ATTRIBUTE
TSQ_REMOVE_SERVER_ATTRIBUTE
TSQ_REMOVE_SERVER
TSQ_CREATE_TERM
TSQ_TASK_INPUT
TSQ_TASK_OUTPUT
TSQ_TASK_ANSWER
TSQ_TASK_QUESTION
TSQ_CANCEL_TASK
TSQ_UPLOAD_FILE
TSQ_DOWNLOAD_FILE
TSQ_DELETE_FILE
TSQ_RENAME_FILE
TSQ_UPLOAD_PIPE
TSQ_DOWNLOAD_PIPE
TSQ_CONNECTING_PORTFWD
TSQ_LISTENING_PORTFWD
TSQ_RUN_COMMAND
TSQ_RUN_CONNECT
TSQ_MOUNT_FILE_READWRITE
TSQ_MOUNT_FILE_READONLY
TSQ_MONITOR_INPUT

TSQ_ANNOUNCE_CLIENT
TSQ_REMOVE_CLIENT
TSQ_GET_CLIENT_ATTRIBUTE
TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE
TSQ_THROTTLE_PAUSE
TSQ_THROTTLE_RESUME

TSQ_INPUT
TSQ_BEGIN_OUTPUT
TSQ_BEGIN_OUTPUT_RESPONSE
TSQ_FLAGS_CHANGED
TSQ_BUFFER_CAPACITY
TSQ_BUFFER_LENGTH
TSQ_BUFFER_SWITCHED
TSQ_SIZE_CHANGED
TSQ_CURSOR_MOVED
TSQ_BELL_RANG
TSQ_ROW_CONTENT
TSQ_ROW_CONTENT_RESPONSE
TSQ_REGION_UPDATE
TSQ_REGION_UPDATE_RESPONSE
TSQ_DIRECTORY_UPDATE
TSQ_FILE_UPDATE
TSQ_FILE_REMOVED
TSQ_END_OUTPUT
TSQ_END_OUTPUT_RESPONSE
TSQ_MOUSE_MOVED
TSQ_IMAGE_CONTENT
TSQ_IMAGE_CONTENT_RESPONSE
TSQ_DOWNLOAD_IMAGE

TSQ_GET_TERM_ATTRIBUTES
TSQ_GET_TERM_ATTRIBUTES_RESPONSE
TSQ_GET_CONN_ATTRIBUTES_RESPONSE
TSQ_GET_TERM_ATTRIBUTE
TSQ_GET_CONN_ATTRIBUTE
TSQ_GET_TERM_ATTRIBUTE_RESPONSE
TSQ_GET_CONN_ATTRIBUTE_RESPONSE
TSQ_SET_TERM_ATTRIBUTE
TSQ_REMOVE_TERM_ATTRIBUTE
TSQ_RESIZE_TERM
TSQ_REMOVE_TERM
TSQ_REMOVE_CONN
TSQ_DUPLICATE_TERM
TSQ_RESET_TERM
TSQ_CHANGE_OWNER
TSQ_REQUEST_DISCONNECT
TSQ_TOGGLE_SOFT_SCROLL_LOCK
TSQ_SEND_SIGNAL

TSQ_CREATE_REGION
TSQ_GET_REGION
TSQ_REMOVE_REGION
/;

1;

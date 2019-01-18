#!/usr/bin/perl
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

use warnings;
use strict;
use IO::Socket::UNIX;
use UUID::Tiny ':std';
use Term::ReadKey;

use FindBin;
use lib "$FindBin::Bin";
use TermyClient;

my $sd;
my $c;
$| = 1;

use constant SOCK_PATH => "/tmp/termy-serverd${<}/sock";

my %tasks;
my %tasktypes;
my $amslow = 0;
my $ignore_keepalives = 0;

#
## Menus
#
sub print_menu {
    print STDERR "Menu\n";
    print STDERR "\t1.  Request terminal\n";
    print STDERR "\t2.  Send input\n";
    print STDERR "\t3.  Get terminal attributes\n";
    print STDERR "\t4.  Get terminal attribute\n";
    print STDERR "\t5.  Set terminal attribute\n";
    print STDERR "\t6.  Remove terminal attribute\n";
    print STDERR "\t7.  Resize terminal\n";
    print STDERR "\t8.  Change buffer capacity\n";
    print STDERR "\t9.  Get terminal content\n";
    print STDERR "\t10. Disconnect terminal connection\n";
    print STDERR "\t11. Close terminal\n";
    print STDERR "\t12. Take terminal ownership\n";
    print STDERR "\t13. Get server attributes\n";
    print STDERR "\t14. Get server attribute\n";
    print STDERR "\t15. Set server attribute\n";
    print STDERR "\t16. Remove server attribute\n";
    print STDERR "\t17. Toggle soft scroll lock\n";
    print STDERR "\t18. Send signal\n";
    print STDERR "\t19. Time request\n";
    print STDERR "\t20. Upload file\n";
    print STDERR "\t21. Delete file\n";
    print STDERR "\t22. Rename file\n";
    print STDERR "\t23. Download file\n";
    print STDERR "\t24: Run command\n";
    print STDERR "\t25: Run connector\n";
    print STDERR "\t26. Become slow client\n";
    print STDERR "\t27: Get image content\n";
    print STDERR "\t28: Download image content\n";
    print STDERR "\t29: Create region\n";
    print STDERR "\t30: Get region\n";
    print STDERR "\t31: Remove region\n";
    print STDERR "\t32: Send monitor input\n";
    print STDERR "\t33: Get owner attribute\n";
    print STDERR "\t34: Set keepalive time\n";
    print STDERR "\t35: Stop replying to keepalive\n";
    print STDERR "\t99. Quit\n";
}

sub get_region {
    print "buffer: ";
    my $bufid = '';
    my $rc = sysread(STDIN, $bufid, 256);
    die unless defined($rc) and $bufid =~ m/^\d+$/;
    chomp($bufid);

    print "region: ";
    my $rid = '';
    $rc = sysread(STDIN, $rid, 256);
    die unless defined($rc) and $rid =~ m/^\d+$/;
    chomp($rid);

    return (int($bufid), int($rid));
}

sub get_term {
    my %reversehash;
    my $i = 0;
    my $termhash = $c->termhash();

    my @keys = keys(%$termhash);
    die "No terminals to select\n" unless scalar(@keys) > 0;
    if (@keys == 1) {
        printf("\tSelected terminal %s\n", uuid_to_string($keys[0]));
        return $keys[0];
    }

    for (keys(%$termhash)) {
        $reversehash{$i} = $_;
        printf("\t%d: %s\n", $i, uuid_to_string($_));
        ++$i;
    }

    my $input = '';
    my $rc = sysread(STDIN, $input, 256);

    die unless defined($rc) and $rc > 1;
    chomp($input);
    die unless exists($reversehash{int($input)});
    return $reversehash{int($input)};
}

sub get_server {
    my %reversehash;
    my $i = 0;
    my $servhash = $c->servhash();

    my @keys = keys(%$servhash);
    die unless scalar(@keys) > 0;
    if (@keys == 1) {
        printf("\tSelected server %s\n", uuid_to_string($keys[0]));
        return $keys[0];
    }

    for (keys(%$servhash)) {
        $reversehash{$i} = $_;
        printf("\t%d: %s\n", $i, uuid_to_string($_));
        ++$i;
    }

    my $input = '';
    my $rc = sysread(STDIN, $input, 256);

    die unless defined($rc) and $rc > 1;
    chomp($input);
    die unless exists($reversehash{int($input)});
    return $reversehash{int($input)};
}

sub print_attributes {
    my $attrs = shift;

    for (sort { $a cmp $b } keys %$attrs) {
        printf("\t%s => %s\n", $_, $$attrs{$_});
    }
}

#
## Commands
#
sub client_announce {
    my %attrs = (
        host => 'ahost',
        name => '1.2.3.4',
        product => 'client.pl 1-RC',
        started => '0',
        user => 'itest',
        userfull => 'Interactive Tester',
        uid => '1234',
        gid => '5678',
    );

    $c->client_announce(\%attrs);
}

sub client_set_keepalive {
    print 'timeout: ';
    my $time = '';
    my $rc = sysread(STDIN, $time, 256);
    die unless defined($rc) && $time =~ m/^\d+$/;
    $c->client_set_keepalive($time);
}

sub term_create {
    my $uuid = get_server;
    $c->term_create($uuid);
}

sub term_send_input {
    my $uuid = get_term;
    print 'input: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);

    $c->term_send_input($uuid, $input);
}

sub term_get_attrs {
    my $uuid = get_term;
    $c->term_get_attrs($uuid);
}

sub term_get_attr {
    my $uuid = get_term;
    print 'attribute: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);

    $c->term_get_attr($uuid, $input);
}

sub term_set_attr {
    my $uuid = get_term;
    print 'attribute: ';
    my $key = '';
    my $rc = sysread(STDIN, $key, 256);
    die unless defined($rc);
    chomp($key);
    print 'value: ';
    my $value = '';
    $rc = sysread(STDIN, $value, 256);
    die unless defined($rc);
    chomp($value);

    $c->term_set_attr($uuid, $key, $value);
}

sub term_del_attr {
    my $uuid = get_term;
    print 'attribute: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);

    $c->term_del_attr($uuid, $input);
}

sub term_resize {
    my $uuid = get_term;
    print 'columns: ';
    my $w = '';
    my $rc = sysread(STDIN, $w, 256);
    die unless defined($rc);
    chomp($w);
    print 'rows: ';
    my $h = '';
    $rc = sysread(STDIN, $h, 256);
    die unless defined($rc);
    chomp($h);

    $c->term_resize($uuid, int($w), int($h));
}

sub term_caporder {
    my $uuid = get_term;
    print 'caporder: ';
    my $caporder = '';
    my $rc = sysread(STDIN, $caporder, 256);
    die unless defined($rc);
    chomp($caporder);

    $c->term_caporder($uuid, int($caporder));
}

sub term_fetch {
    my $uuid = get_term;
    print 'range: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);
    my ($start, $end) = split('-', $input);

    $c->term_fetch($uuid, int($start), int($end));
}

sub term_disconnect {
    my $uuid = get_term;
    $c->term_disconnect($uuid);
}

sub term_close {
    my $uuid = get_term;
    $c->term_close($uuid);
}

sub term_set_owner {
    my $uuid = get_term;
    $c->term_set_owner($uuid);
}

sub term_get_owner_attribute {
    my $uuid = get_term;
}

sub term_scroll_lock {
    my $uuid = get_term;
    $c->term_scroll_lock($uuid);
}

sub term_signal {
    my $uuid = get_term;
    print 'signal: ';
    my $sig = '';
    my $rc = sysread(STDIN, $sig, 256);
    die unless defined($rc) && $sig =~ m/^\d+$/;
    chomp($sig);

    $c->term_signal($uuid, int($sig));
}

sub term_get_content {
    my $uuid = get_term;
    print 'id: ';
    my $id = '';
    my $rc = sysread(STDIN, $id, 256);
    die unless defined($rc) && $id =~ m/^\d+$/;;
    chomp($id);

    $c->term_get_content($uuid, int($id));
}

sub term_download_content {
    my $uuid = get_term;
    print 'id: ';
    my $id = '';
    my $rc = sysread(STDIN, $id, 256);
    die unless defined($rc) && $id =~ m/^\d+$/;;
    chomp($id);

    print 'target file: ';
    my $dst = '';
    $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);

    open(my $fh, '>:raw', $dst) or die "$dst: $!\n";
    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = $fh;
    $tasktypes{$task} = 0;
    $c->term_download_content($uuid, $task, int($id), 4096, 8);
}

sub server_get_attrs {
    my $uuid = get_server;
    $c->server_get_attrs($uuid);
}

sub server_get_attr {
    my $uuid = get_server;
    print 'attribute: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);

    $c->server_get_attr($uuid, $input);
}

sub server_set_attr {
    my $uuid = get_server;
    print 'attribute: ';
    my $key = '';
    my $rc = sysread(STDIN, $key, 256);
    die unless defined($rc);
    chomp($key);
    print 'value: ';
    my $value = '';
    $rc = sysread(STDIN, $value, 256);
    die unless defined($rc);
    chomp($value);

    $c->server_set_attr($uuid, $key, $value);
}

sub server_del_attr {
    my $uuid = get_server;
    print 'attribute: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);

    $c->server_del_attr($uuid, $input);
}

sub server_get_time {
    my $uuid = get_server;
    $c->server_get_time($uuid);
}

sub server_upload_file {
    my $uuid = get_server;

    print 'source file: ';
    my $src = '';
    my $rc = sysread(STDIN, $src, 256);
    die unless defined($rc);
    chomp($src);
    die unless -f $src && -r $src;

    print 'target file: ';
    my $dst = '';
    $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);

    print 'confirmation setting? ';
    my $config = '';
    $rc = sysread(STDIN, $config, 256);
    die unless defined($rc) && $config =~ m/^\d+$/;
    $config = int($config);

    print 'mkpath? ';
    my $mkpath = '';
    $rc = sysread(STDIN, $mkpath, 256);
    die unless defined($rc) && $mkpath =~ m/^[01]$/;
    $config |= int($mkpath) << 31;

    open(my $fh, '<:raw', $src) or die "$src: $!\n";
    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = $fh;
    $tasktypes{$task} = 1;
    $c->server_upload_file($uuid, $task, 4096, 0644, $config, $dst);
}

sub server_remove_file {
    my $uuid = get_server;

    print 'target file: ';
    my $dst = '';
    my $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);

    print 'confirmation setting? ';
    my $config = '';
    $rc = sysread(STDIN, $config, 256);
    die unless defined($rc) && $config =~ m/^\d+$/;
    $config = int($config);

    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = undef;
    $tasktypes{$task} = 2;
    $c->server_remove_file($uuid, $task, $config, $dst);
}

sub server_rename_file {
    my $uuid = get_server;

    print 'source file: ';
    my $src = '';
    my $rc = sysread(STDIN, $src, 256);
    die unless defined($rc);
    chomp($src);

    print 'target file: ';
    my $dst = '';
    $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);

    print 'confirmation setting? ';
    my $config = '';
    $rc = sysread(STDIN, $config, 256);
    die unless defined($rc) && $config =~ m/^\d+$/;
    $config = int($config);

    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = undef;
    $tasktypes{$task} = 3;
    $c->server_rename_file($uuid, $task, $config, $src, $dst);
}

sub server_download_file {
    my $uuid = get_server;

    print 'source file: ';
    my $src = '';
    my $rc = sysread(STDIN, $src, 256);
    die unless defined($rc);
    chomp($src);

    print 'target file: ';
    my $dst = '';
    $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);

    open(my $fh, '>:raw', $dst) or die "$dst: $!\n";
    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = $fh;
    $tasktypes{$task} = 0;
    $c->server_download_file($uuid, $task, 4096, 8, $src);
}

sub server_run_command {
    my $uuid = get_server;
    my $fh;

    print 'command: ';
    my $cmdline = '';
    my $rc = sysread(STDIN, $cmdline, 256);
    die unless defined($rc);
    chomp($cmdline);

    print 'output file: ';
    my $dst = '';
    $rc = sysread(STDIN, $dst, 256);
    die unless defined($rc);
    chomp($dst);
    my $o = ($dst ne '1');
    if ($o) {
        open($fh, '>:raw', $dst) or die "$dst: $!\n";
    }

    $cmdline = "/bin/sh\x1f/bin/sh\x1f-c\x1f" . $cmdline;

    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = $fh;
    $tasktypes{$task} = 4;
    $c->server_run_command($uuid, $task, $o * 4096, $o * 8, $cmdline);
}

sub server_run_connect {
    my $uuid = get_server;

    print 'command: ';
    my $cmdline = '';
    my $rc = sysread(STDIN, $cmdline, 256);
    die unless defined($rc);
    chomp($cmdline);
    $cmdline =~ tr/ /\x1f/s;

    my $task = create_uuid(UUID_RANDOM);
    $tasks{$task} = undef;
    $tasktypes{$task} = 5;
    $c->server_run_connect($uuid, $task, $cmdline);
}

sub server_monitor_input {
    my $uuid = get_server;
    print 'input: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);

    $c->server_monitor_input($uuid, $input);
}

sub server_get_client_attr {
    my $uuid = get_server;
    print 'client: ';
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);
    die unless is_uuid_string($input);
    my $clientid = string_to_uuid($input);

    print 'attribute: ';
    $input = '';
    $rc = sysread(STDIN, $input, 256);
    die unless defined($rc);
    chomp($input);

    $c->server_get_client_attr($uuid, $clientid, $input);
}

sub region_create_region {
    my $uuid = get_term;

    print "buffer: ";
    my $bufid = '';
    my $rc = sysread(STDIN, $bufid, 256);
    die unless defined($rc) and $bufid =~ m/^\d+$/;
    chomp($bufid);

    print "type: ";
    my $type = '';
    $rc = sysread(STDIN, $type, 256);
    die unless defined($rc) and $type =~ m/^\d+$/;
    chomp($type);

    print "start row: ";
    my $srow = '';
    $rc = sysread(STDIN, $srow, 256);
    die unless defined($rc) and $srow =~ m/^\d+$/;
    chomp($srow);

    print "end row: ";
    my $erow = '';
    $rc = sysread(STDIN, $erow, 256);
    die unless defined($rc) and $erow =~ m/^\d+$/;
    chomp($erow);

    print "start col: ";
    my $scol = '';
    $rc = sysread(STDIN, $scol, 256);
    die unless defined($rc) and $scol =~ m/^\d+$/;
    chomp($scol);

    print "end col: ";
    my $ecol = '';
    $rc = sysread(STDIN, $ecol, 256);
    die unless defined($rc) and $ecol =~ m/^\d+$/;
    chomp($ecol);

    $c->region_create_region($uuid, $bufid, $type, $srow, $erow, $scol, $ecol);
}

sub region_get_region {
    my $uuid = get_term;
    my ($bufid, $regid) = get_region;
    $c->region_get_region($uuid, $bufid, $regid);
}

sub region_remove_region {
    my $uuid = get_term;
    my ($bufid, $regid) = get_region;
    $c->region_remove_region($uuid, $bufid, $regid);
}

#
## Server Responses
#
sub handle_server_announce {
    my $args = shift;
    my $serv = $$args{server};
    my $hop = $$args{hop};
    my $version = $$args{version};
    my $hopcount = $$args{hopcount};
    my $termcount = $$args{termcount};

    printf("Server %s added (version %u, %d terminals)\n", uuid_to_string($serv),
           $version, $termcount);
    printf("\tConnected via %s (%u hops)\n", uuid_to_string($hop), $hopcount);
    print_attributes($$args{attrs});
}

sub handle_server_attributes {
    my $args = shift;
    my $serv = $$args{server};

    printf("Got attributes for server %s\n", uuid_to_string($serv));
    print_attributes($$args{attrs});
}

sub handle_server_attribute {
    my ($args, $type) = @_;
    my $serv = $$args{server};
    my $key = $$args{key};
    my $value = $$args{value};

    if ($type == TSQ_GET_SERVER_ATTRIBUTE) {
        printf("Attribute change for server %s\n", uuid_to_string($serv));
    } else {
        printf("Server attribute response for %s\n", uuid_to_string($serv));
    }

    if (defined($value)) {
        printf("\t%s => %s\n", $key, $value);
    } else {
        printf("\t%s [removed]\n", $key);
    }
}

sub handle_server_removed {
    my $args = shift;
    my $serv = $$args{server};
    my $reason = $$args{reason};

    printf("Server %s removed (code %x)\n", uuid_to_string($serv), $reason);
}

sub handle_server_time {
    my $args = shift;
    my $serv = $$args{server};
    my $time = $$args{time};

    printf("Server %s time is %lu\n", uuid_to_string($serv), $time);
}

sub handle_task_question {
    my $args = shift;
    my $serv = $$args{server};
    my $task = $$args{task};
    my $question = $$args{question};

    die unless exists $tasks{$task};

    printf("TASK QUESTION %s: query #%d\n", uuid_to_string($task), $question);
    print 'response: ';
    my $answer = '';
    my $rc = sysread(STDIN, $answer, 256);
    die unless defined($rc) && $answer =~ m/^\d+$/;

    $c->server_task_answer($serv, $task, int($answer));
}

sub handle_misc_task_output {
    my ($serv, $task, $data) = @_;
    my $status = parse_number(\$data);

    if ($status == TSQ_TASK_ERROR) {
        my $code = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("TASK ERROR %s: %u %s\n", uuid_to_string($task), $code, $msg);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_FINISHED) {
        printf("TASK FINISHED %s\n", uuid_to_string($task));
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    else {
        die;
    }
}

sub handle_connect_task_output {
    my ($serv, $task, $data) = @_;
    my $status = parse_number(\$data);

    if ($status == TSQ_TASK_STARTING) {
        my $pid = parse_number(\$data);
        printf("CONNECT STARTING %s (pid %u)\n", uuid_to_string($task), $pid);
    }
    elsif ($status == TSQ_TASK_RUNNING) {
        printf("CONNECT OUTPUT %s: %s", uuid_to_string($task), $data);
        my $response = '';
        ReadMode('noecho');
        my $rc = sysread(STDIN, $response, 256);
        ReadMode(0);
        die unless defined($rc);
        print "****\n";
        $c->server_task_input($serv, $task, $response);
    }
    elsif ($status == TSQ_TASK_ERROR) {
        my $exitcode = parse_number(\$data);
        my $subcode = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("CONNECT FAILED %s (%u, %u, %s)\n", uuid_to_string($task),
               $exitcode, $subcode, $msg);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_FINISHED) {
        my $id = parse_uuid(\$data);
        printf("COMMAND FINISHED %s: (connid %s)\n", uuid_to_string($task), uuid_to_string($id));
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    else {
        die;
    }
}

sub handle_command_task_output {
    my ($serv, $task, $data) = @_;
    my $status = parse_number(\$data);

    if ($status == TSQ_TASK_STARTING) {
        my $pid = parse_number(\$data);
        printf("COMMAND STARTING %s (pid %u)\n", uuid_to_string($task), $pid);
        if (defined($tasks{$task})) {
            my $buf = pack('Vx8', TSQ_TASK_ACKING);
            $c->server_task_input($serv, $task, $buf);
        }
    }
    elsif ($status == TSQ_TASK_RUNNING) {
        syswrite($tasks{$task}, $data) or die;
        my $pos = sysseek($tasks{$task}, 0, 1);
        printf("COMMAND DATA %s (%lu)\n", uuid_to_string($task), $pos);
        my $buf = pack('VVV', TSQ_TASK_ACKING, $pos, $pos >> 32);
        $c->server_task_input($serv, $task, $buf);
    }
    elsif ($status == TSQ_TASK_ERROR) {
        my $outcome = parse_number(\$data);
        my $exitcode = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("COMMAND FAILED %s (%u, %u, %s)\n", uuid_to_string($task),
               $outcome, $exitcode, $msg);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_FINISHED) {
        my $outcome = parse_number(\$data);
        my $exitcode = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("COMMAND FINISHED %s (%u, %u, %s)\n", uuid_to_string($task),
               $outcome, $exitcode, $msg);
        close $tasks{$task} if defined($tasks{$task});
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    else {
        die;
    }
}

sub handle_upload_task_output {
    my ($serv, $task, $data) = @_;
    my $status = parse_number(\$data);
    my $written = parse_number64(\$data);

    if ($status == TSQ_TASK_ERROR) {
        my $code = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("TASK ERROR %s (%lu): %u %s\n", uuid_to_string($task), $written,
               $code, $msg);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_FINISHED) {
        printf("TASK FINISHED %s (%lu)\n", uuid_to_string($task), $written);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_ACKING) {
        printf("TASK ACK %s (%lu)\n", uuid_to_string($task), $written);
        my $buf = '';
        my $rc = read($tasks{$task}, $buf, 4096);
        die $! unless defined($rc);
        $c->server_task_input($serv, $task, $buf) if ($rc > 0);
        $c->server_task_input($serv, $task, '') if ($rc < 4096);
    }
    elsif ($status == TSQ_TASK_STARTING) {
        my $msg = parse_string(\$data);
        printf("TASK RENAME %s (%lu): %s\n", uuid_to_string($task), $written, $msg);
    }
    else {
        die;
    }
}

sub handle_download_task_output {
    my ($serv, $task, $data) = @_;
    my $status = parse_number(\$data);

    if ($status == TSQ_TASK_ERROR) {
        my $code = parse_number(\$data);
        my $msg = parse_string(\$data);
        printf("DOWNLOAD ERROR %s: %u %s\n", uuid_to_string($task), $code, $msg);
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_STARTING) {
        my $mode = parse_number(\$data);
        my $total = parse_number64(\$data);
        printf("DOWNLOAD STARTING %s: %o %lu\n", uuid_to_string($task), $mode, $total);
        my $buf = pack('x8');
        $c->server_task_input($serv, $task, $buf);
    }
    elsif ($status == TSQ_TASK_RUNNING && length($data) == 0) {
        printf("DOWNLOAD FINISHED %s\n", uuid_to_string($task));
        close $tasks{$task};
        delete $tasks{$task};
        delete $tasktypes{$task};
    }
    elsif ($status == TSQ_TASK_RUNNING) {
        syswrite($tasks{$task}, $data) or die;
        my $pos = sysseek($tasks{$task}, 0, 1);
        printf("DOWNLOAD DATA %s (%lu)\n", uuid_to_string($task), $pos);
        my $buf = pack('VV', $pos, $pos >> 32);
        $c->server_task_input($serv, $task, $buf);
    }
    else {
        die;
    }
}

sub handle_task_output {
    my $args = shift;
    my $serv = $$args{server};
    my $task = $$args{task};
    my $data = $$args{data};

    die unless exists $tasktypes{$task};

    if ($tasktypes{$task} == 0) {
        handle_download_task_output($serv, $task, $data);
    } elsif ($tasktypes{$task} == 1) {
        handle_upload_task_output($serv, $task, $data);
    } elsif ($tasktypes{$task} == 4) {
        handle_command_task_output($serv, $task, $data);
    } elsif ($tasktypes{$task} == 5) {
        handle_connect_task_output($serv, $task, $data);
    } else {
        handle_misc_task_output($serv, $task, $data);
    }
}

sub handle_throttle_pause {
    my $args = shift;
    my $dest = $$args{dest};
    my $term = $$args{term};

    printf("SET THROTTLE for destination %s via terminal %s\n",
           uuid_to_string($dest), uuid_to_string($term));
    printf("\tlimit: %u actual: %u\n", $$args{limit}, $$args{size});
}

sub handle_throttle_resume {
    my $args = shift;
    my $term = $$args{term};

    printf("UNSET THROTTLE for terminal %s\n", uuid_to_string($term));
    print_attributes($$args{attrs});
}

sub handle_disconnect {
    my $args = shift;

    printf("Server disconnected (code %d)\n", $$args{reason});
}

sub handle_keepalive {
    if ($ignore_keepalives) {
        printf("KEEPALIVE (ignored)\n");
    } else {
        printf("KEEPALIVE\n");
        $c->client_keepalive();
    }
}

#
## Term Responses
#
sub handle_term_announce {
    my $args = shift;
    my $term = $$args{term};
    my $hop = $$args{hop};
    my $hopcount = $$args{hopcount};
    my $width = $$args{width};
    my $height = $$args{height};

    printf("Terminal %s added (%dx%d)\n", uuid_to_string($term), $width, $height);
    printf("\tOn server %s (%d hops)\n", uuid_to_string($hop), $hopcount);
    print_attributes($$args{attrs});
}

sub handle_conn_announce {
    my $args = shift;
    my $term = $$args{term};
    my $hop = $$args{hop};
    my $hopcount = $$args{hopcount};

    printf("Connection %s added\n", uuid_to_string($term));
    printf("\tOn server %s (%d hops)\n", uuid_to_string($hop), $hopcount);
    print_attributes($$args{attrs});
}

sub handle_term_attributes {
    my ($args, $type) = @_;
    my $term = $$args{term};

    if ($type == TSQ_GET_TERM_ATTRIBUTES_RESPONSE) {
        printf("Got attributes for terminal %s\n", uuid_to_string($term));
    } else {
        printf("Got attributes for connection %s\n", uuid_to_string($term));
    }

    print_attributes($$args{attrs});
}

sub handle_term_attribute {
    my ($args, $type) = @_;
    my $term = $$args{term};
    my $key = $$args{key};
    my $value = $$args{value};

    if ($type == TSQ_GET_TERM_ATTRIBUTE) {
        printf("Attribute change for terminal %s\n", uuid_to_string($term));
    } elsif ($type == TSQ_GET_CONN_ATTRIBUTE) {
        printf("Attribute change for connection %s\n", uuid_to_string($term));
    } elsif ($type == TSQ_GET_TERM_ATTRIBUTE_RESPONSE) {
        printf("Terminal attribute response for %s\n", uuid_to_string($term));
    } else {
        printf("Connection attribute response for %s\n", uuid_to_string($term));
    }

    if (defined($value)) {
        printf("\t%s => %s\n", $key, $value);
    } else {
        printf("\t%s [removed]\n", $key);
    }
}

sub handle_term_closed {
    my $args = shift;
    my $term = $$args{term};
    my $reason = $$args{reason};

    printf("Terminal %s removed (code %x)\n", uuid_to_string($term), $reason);
}

sub handle_begin_output {
    my ($args, $type) = @_;
    my $term = $$args{term};

    if ($type == TSQ_BEGIN_OUTPUT) {
        printf("BEGIN OUTPUT for Terminal %s\n", uuid_to_string($term));
    } else {
        printf("BEGIN OUTPUT RESPONSE for Terminal %s\n", uuid_to_string($term));
    }
}

sub handle_flags_changed {
    my $args = shift;
    my $flags = $$args{flags};

    printf("    FLAGS CHANGED to %x\n", $flags);
}

sub handle_buffer_capacity {
    my $args = shift;
    my $size = $$args{size};
    my $bufid = $$args{bufid};
    my $caporder = $$args{caporder};

    printf("    BUFFER %d CAPACITY (size=%d, caporder=%d)\n", $bufid, $size, $caporder);
}

sub handle_buffer_length {
    my $args = shift;
    my $size = $$args{size};
    my $bufid = $$args{bufid};

    printf("    BUFFER %d LENGTH (size=%d)\n", $bufid, $size);
}

sub handle_buffer_switched {
    my $args = shift;
    my $bufid = $$args{bufid};

    printf("    BUFFER SWITCH TO %d\n", $bufid);
}

sub handle_size_changed {
    my $args = shift;
    my $width = $$args{width};
    my $height = $$args{height};
    my $marginx = $$args{marginx};
    my $marginy = $$args{marginy};
    my $marginw = $$args{marginw};
    my $marginh = $$args{marginh};

    printf("    SIZE CHANGE to (%d,%d) MARGINS (%d,%d,%d,%d)\n",
           $width, $height, $marginx, $marginy, $marginw, $marginh);
}

sub handle_cursor_moved {
    my $args = shift;
    my $x = $$args{x};
    my $y = $$args{y};
    my $pos = $$args{pos};
    my $subpos = $$args{subpos};
    my $flags = $$args{flags};

    printf("    CURSOR MOVE to (%d,%d) (%d,%d) %x\n", $x, $y, $pos, $subpos, $flags);
}

sub handle_bell_rang {
    my $args = shift;
    my $code = $$args{code};
    my $count = $$args{count};

    printf("    DING-A-LING (code %d, count %d)\n", $code, $count);
}

sub handle_output_row {
    my $args = shift;
    my $rownum = $$args{rownum};
    my $bufid = $$args{bufid};
    my $flags = $$args{flags};
    my $modtime = $$args{modtime};
    my $ranges = $$args{ranges};

    printf("    ROW OUTPUT (%d,%d,%x)+%d ", $rownum, $bufid, $flags, $modtime);

    for (my $i = 0; $i < @$ranges; ++$i) {
        printf("(%d:%d,%d,%x,%x,%x,%x) ", $i, @{$$ranges[$i]});
    }

    printf("'%s'\n", $$args{content});
}

sub handle_region_update {
    my $args = shift;
    my $regid = $$args{regid};
    my $type = $$args{type};
    my $flags = $$args{flags};
    my $parent = $$args{parent};
    my $srow = $$args{srow};
    my $erow = $$args{erow};
    my $scol = $$args{scol};
    my $ecol = $$args{ecol};

    printf("    REGION UPDATE (%d,%x,%x,%d, [%lu,%d],[%lu,%d])\n",
           $regid, $type, $flags, $parent, $srow, $scol, $erow, $ecol);

    print_attributes($$args{attrs});
}

sub handle_directory_update {
    my $args = shift;
    my $name = $$args{name};

    printf("    DIRECTORY UPDATE %s\n", $name);

    print_attributes($$args{attrs});
}

sub handle_file_update {
    my $args = shift;
    my $mode = $$args{mode};
    my $mtime = $$args{mtime};
    my $size = $$args{size};
    my $uid = $$args{uid};
    my $gid = $$args{gid};
    my $name = $$args{name};

    printf("    FILE UPDATE (%x,%x,%x,%d,%d) %s\n", $mode, $mtime,
           $size, $uid, $gid, $name);

    print_attributes($$args{attrs});
}

sub handle_file_removed {
    my $args = shift;
    my $mtime = $$args{mtime};
    my $name = $$args{name};

    printf("    FILE REMOVED (%x) %s\n", $mtime, $name);

    print_attributes($$args{attrs});
}

sub handle_end_output {
    my ($args, $type) = @_;
    my $term = $$args{term};

    if ($type == TSQ_END_OUTPUT) {
        printf("END OUTPUT for Terminal %s\n", uuid_to_string($term));
    } else {
        printf("END OUTPUT RESPONSE for Terminal %s\n", uuid_to_string($term));
    }
}

sub handle_mouse_moved {
    my ($args, $type) = @_;
    my $term = $$args{term};
    my $x = $$args{x};
    my $y = $$args{y};

    printf("MOUSE MOVE for Terminal %s: %d,%d\n", uuid_to_string($term), $x, $y);
}

sub handle_image_content_response {
    my ($args, $type) = @_;
    my $term = $$args{term};
    my $id = $$args{id};

    printf("IMAGE CONTENT for Terminal %s\n", uuid_to_string($term));
    printf("\tid: %llu, length: %u bytes\n", $id, length($$args{data}));
}

sub handle_client_attribute {
    my ($args, $type) = @_;
    my $client = $$args{client};
    my $key = $$args{key};
    my $value = $$args{value};

    printf("Client attribute response for client %s\n", uuid_to_string($client));

    if (defined($value)) {
        printf("\t%s => %s\n", $key, $value);
    } else {
        printf("\t%s [removed]\n", $key);
    }
}

#
## Handling
#
sub handle_input {
    my $input = '';
    my $rc = sysread(STDIN, $input, 256);

    die unless defined($rc);
    chomp($input);
    return 1 if $input eq '';

    term_create if $input eq      '1';
    term_send_input if $input eq  '2';
    term_get_attrs if $input eq   '3';
    term_get_attr if $input eq    '4';
    term_set_attr if $input eq    '5';
    term_del_attr if $input eq    '6';
    term_resize if $input eq      '7';
    term_caporder if $input eq    '8';
    term_fetch if $input eq       '9';
    term_disconnect if $input eq  '10';
    term_close if $input eq       '11';
    term_set_owner if $input eq   '12';
    server_get_attrs if $input eq '13';
    server_get_attr if $input eq  '14';
    server_set_attr if $input eq  '15';
    server_del_attr if $input eq  '16';
    term_scroll_lock if $input eq '17';
    term_signal if $input eq      '18';
    server_get_time if $input eq  '19';
    server_upload_file if $input eq     '20';
    server_remove_file if $input eq     '21';
    server_rename_file if $input eq     '22';
    server_download_file if $input eq   '23';
    server_run_command if $input eq     '24';
    server_run_connect if $input eq     '25';
    $amslow = 1 if $input eq            '26';
    term_get_content if $input eq       '27';
    term_download_content if $input eq  '28';
    region_create_region if $input eq   '29';
    region_get_region if $input eq      '30';
    region_remove_region if $input eq   '31';
    server_monitor_input if $input eq   '32';
    server_get_client_attr if $input eq '33';
    client_set_keepalive if $input eq   '34';
    $ignore_keepalives = 1 if $input eq '35';

    return 0 if $input eq '99';

    print_menu;
    return 1;
}

sub callback {
    my ($type, $args) = @_;

    handle_server_announce($args) if $type == TSQ_ANNOUNCE_SERVER;
    handle_server_attributes($args) if $type == TSQ_GET_SERVER_ATTRIBUTES_RESPONSE;
    handle_server_attribute($args, $type) if $type == TSQ_GET_SERVER_ATTRIBUTE;
    handle_server_attribute($args, $type) if $type == TSQ_GET_SERVER_ATTRIBUTE_RESPONSE;
    handle_server_removed($args) if $type == TSQ_REMOVE_SERVER;
    handle_server_time($args) if $type == TSQ_GET_SERVER_TIME_RESPONSE;
    handle_task_question($args) if $type == TSQ_TASK_QUESTION;
    handle_task_output($args) if $type == TSQ_TASK_OUTPUT;
    handle_throttle_pause($args) if $type == TSQ_THROTTLE_PAUSE;
    handle_throttle_resume($args) if $type == TSQ_THROTTLE_RESUME;

    handle_term_announce($args) if $type == TSQ_ANNOUNCE_TERM;
    handle_conn_announce($args) if $type == TSQ_ANNOUNCE_CONN;
    handle_term_attributes($args, $type) if $type == TSQ_GET_TERM_ATTRIBUTES_RESPONSE;
    handle_term_attributes($args, $type) if $type == TSQ_GET_CONN_ATTRIBUTES_RESPONSE;
    handle_term_attribute($args, $type) if $type == TSQ_GET_TERM_ATTRIBUTE;
    handle_term_attribute($args, $type) if $type == TSQ_GET_CONN_ATTRIBUTE;
    handle_term_attribute($args, $type) if $type == TSQ_GET_TERM_ATTRIBUTE_RESPONSE;
    handle_term_attribute($args, $type) if $type == TSQ_GET_CONN_ATTRIBUTE_RESPONSE;
    handle_term_closed($args) if $type == TSQ_REMOVE_TERM;
    handle_begin_output($args, $type) if $type == TSQ_BEGIN_OUTPUT;
    handle_begin_output($args, $type) if $type == TSQ_BEGIN_OUTPUT_RESPONSE;
    handle_flags_changed($args) if $type == TSQ_FLAGS_CHANGED;
    handle_buffer_capacity($args) if $type == TSQ_BUFFER_CAPACITY;
    handle_buffer_length($args) if $type == TSQ_BUFFER_LENGTH;
    handle_buffer_switched($args) if $type == TSQ_BUFFER_SWITCHED;
    handle_size_changed($args) if $type == TSQ_SIZE_CHANGED;
    handle_cursor_moved($args) if $type == TSQ_CURSOR_MOVED;
    handle_bell_rang($args) if $type == TSQ_BELL_RANG;
    handle_output_row($args, $type) if $type == TSQ_ROW_CONTENT;
    handle_output_row($args, $type) if $type == TSQ_ROW_CONTENT_RESPONSE;
    handle_region_update($args, $type) if $type == TSQ_REGION_UPDATE;
    handle_region_update($args, $type) if $type == TSQ_REGION_UPDATE_RESPONSE;
    handle_directory_update($args, $type) if $type == TSQ_DIRECTORY_UPDATE;
    handle_file_update($args, $type) if $type == TSQ_FILE_UPDATE;
    handle_file_removed($args, $type) if $type == TSQ_FILE_REMOVED;
    handle_end_output($args, $type) if $type == TSQ_END_OUTPUT;
    handle_end_output($args, $type) if $type == TSQ_END_OUTPUT_RESPONSE;
    handle_mouse_moved($args, $type) if $type == TSQ_MOUSE_MOVED;
    handle_image_content_response($args, $type) if $type == TSQ_IMAGE_CONTENT_RESPONSE;
    handle_client_attribute($args, $type) if $type == TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE;

    handle_disconnect($args) if $type == TSQ_DISCONNECT;
    handle_keepalive($args) if $type == TSQ_KEEPALIVE;
    return $type != TSQ_DISCONNECT;
}

sub run {
    sleep 1 if $amslow;

    my $rin = '';
    vec($rin, 0, 1) = 1;
    vec($rin, fileno($sd), 1) = 1;

    my $rout;
    my $rc = select($rout=$rin, undef, undef, undef);
    die "select: $!\n" unless defined($rc);

    return handle_input if vec($rout, 0, 1);
    return $c->handle_socket if vec($rout, fileno($sd), 1);
    return 1;
}

$sd = IO::Socket::UNIX->new( Type => SOCK_STREAM, Peer => SOCK_PATH );
die "connect: $!\n" unless defined($sd);
$sd->autoflush(1);
binmode($sd, ':raw');

$c = TermyClient->new($sd, \&callback);
printf "I am client %s\n", uuid_to_string($c->id());
$c->perform_handshake();
client_announce;

print_menu;
1 while run;

close($sd);
exit;

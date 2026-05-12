#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use File::Spec;
use File::Basename;
use URI::Escape;

# Usage: perl localhost.pl [port] [root_directory]
my $port = shift || 8080;
my $root = shift || '.';
$root = File::Spec->rel2abs($root);

print "Starting free local host server on http://127.0.0.1:$port/\n";
print "Serving files from: $root\n";

my $server = IO::Socket::INET->new(
    LocalAddr => '127.0.0.1',
    LocalPort => $port,
    Proto     => 'tcp',
    Listen    => 5,
    Reuse     => 1,
) or die "Unable to start server on port $port: $!\n";

while (my $client = $server->accept()) {
    $client->autoflush(1);
    my $request = <$client>;
    next unless $request;
    my ($method, $path) = split ' ', $request;
    $path = uri_unescape($path);
    $path =~ s/\?.*//;
    $path =~ s/\././g;
    $path =~ s/\/\.+/\//g;
    $path =~ s/^\///;
    my $file_path = File::Spec->catfile($root, $path);
    if (-d $file_path) {
        $file_path = File::Spec->catfile($file_path, 'index.html');
    }

    if (-e $file_path && -r $file_path) {
        open my $fh, '<', $file_path;
        binmode $fh;
        my $content = do { local $/; <$fh> };
        close $fh;
        print $client "HTTP/1.1 200 OK\r\n";
        print $client "Content-Type: text/html; charset=UTF-8\r\n";
        print $client "Content-Length: " . length($content) . "\r\n";
        print $client "Connection: close\r\n\r\n";
        print $client $content;
    } else {
        my $body = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>";
        print $client "HTTP/1.1 404 Not Found\r\n";
        print $client "Content-Type: text/html; charset=UTF-8\r\n";
        print $client "Content-Length: " . length($body) . "\r\n";
        print $client "Connection: close\r\n\r\n";
        print $client $body;
    }
    close $client;
}

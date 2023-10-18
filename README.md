# Trivial Torrent Project: Client and Server

## Trivial Torrent Client

Use the file `src/ttorrent.c`.

The `ttorrent` command shall work as follows:

~~~{.diff}
$ bin/ttorrent file.ttorrent
~~~

## Trivial Torrent Server

Use the file `src/ttorrent.c`.

The `ttorrent` command shall be extended to work as follows:

~~~{.diff}
$ bin/ttorrent -l 8080 file.ttorrent
~~~

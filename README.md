# Trivial Torrent Project: Client and Server
Este proyecto es una imitación de un cliente torrent simple que puede conectarse a tres seeds diferentes simultáneamente para descargar archivos torrent. Está diseñado para ser una implementación básica pero funcional de un cliente torrent que muestra el proceso de conexión a seeds y descarga de archivos.

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

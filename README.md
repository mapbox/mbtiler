mbtiler
=======

A demo of MBTile writing in C/C++

This module and program have been developed on OS X. GDAL's CPL module and the
SQLite library are cross-platform and porting this code to Windows should not
be difficult, but no compilation or testing on Windows has been done.

The mbtiler demo program takes 2 arguments: MBTiles dataset name and
description. It reads one image tile from the tests/data directory and
inserts that into the MBTiles dataset.

```
$ ./mbtiler "test" "MBTiler test dataset"
```

The resulting MBTile dataset metadata:

```
$ echo "select * from metadata;" | sqlite3 test.mbtiles;
name|test
type|overlay
version|1.0.0
description|MBTiler test dataset
format|png
```

The resulting tiles:

```
$echo "select * from tiles;" | sqlite3 test.mbtiles
15|16979|21196|�PNG
```

NB: Traversal of a tile tree is platform-specific and has not been implemented.

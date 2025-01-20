Denemo | Free and Open Music Notation Editor
============================================

[![Build Status](https://travis-ci.org/denemo/denemo.svg?branch=master)](https://travis-ci.org/denemo/denemo)

See [the official website](https://www.denemo.org) for more information.

Install:
--------
```
apt-get install build-essential guile-2.0-dev libaubio-dev portaudio19-dev libfftw3-dev libgtk-3-dev libxml2-dev automake libtool libgtksourceview-3.0-dev libfluidsynth-dev autoconf libsmf-dev autopoint librsvg2-dev libportmidi-dev libsndfile1-dev libatrilview-dev libatrildocument-dev librsvg2-dev librubberband-dev intltool gtk-doc-tools
```

Compilation
-----------


```
$ ./autogen.sh
$ ./configure
$ make
# make install
```

See ```configure``` options with ```./configure --help```.

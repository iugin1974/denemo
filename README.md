Denemo | Free and Open Music Notation Editor
============================================

[![Build Status](https://travis-ci.org/denemo/denemo.svg?branch=master)](https://travis-ci.org/denemo/denemo)

See [the official website](https://www.denemo.org) for more information.

This version of Denemo introduces the following changes:
<ul>
    <li>Use of keys 1-7 to input note values, similar to MuseScore and Finale</li>
    <li>Exporting lyrics in XML format (not yet fully tested)</li>
    <li>Exporting to LilyPond with added | at the end of each measure and numbering every measure (unlike Denemo, which numbers every five measures)</li>
      </ul>
    
Install:
<<<<<<< HEAD
--------
=======
>>>>>>> d6a69f9a0 (Il codice lilypond viene ora scritto con relative invece di absolute)
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

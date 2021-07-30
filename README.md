# Impulse Tracker Music Player

This project is a means for me to hone my Modern C++ and TDD/Unit Testing skills through 
the creation of a cross-platform (Windows/Linux/Mac) music player that supports the
[Impulse Tracker](https://en.wikipedia.org/wiki/Impulse_Tracker) (.IT) file format. 
The Impulse Tracker format is itself a descendent (and superset) of the [MOD](https://en.wikipedia.org/wiki/Module_file)
that originated on the Amiga platform.

Many will be familiar with the MP3 and MIDI file formats. While MP3 stores a highly compressed digital
recording of a piece of music, MIDI stores a sequence of commands (note-on, note-off, pitch bend, etc)
and requires a MIDI device to produce actual audio. The MOD formats can be thought of as a combination
of these two. MODules store digital samples (typically of individual notes being played), along with
instructions for how to play these samples. Music is produced by altering the pitch, volume, and 
panning of the samples over time in accordance with the instructions.

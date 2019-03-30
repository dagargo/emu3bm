# emu3bm

emu3bm is an EMU3 bank manager. It allows to create banks, add and extract mono or stereo samples and edit some preset parameters.

## Installation

Simply run `make && sudo make install`.

## Features not available in the sampler

* Bulk edition.
* Set the pitch bend range beyond the 12 semitones limit.

## Examples

Create a new bank.
```
$ emu3bm -n bank
```

Import a sample.
```
$ emu3bm -s bd.wav bank
```

Import a looped sample.
```
$ emu3bm -S saw.wav bank
```

Create a new preset.
```
$ emu3bm -p BassDrum bank
```

Add a primary layer to preset 0 from sample 001 with original key F#2 from C2 to C3.
```
$ emu3bm -z 1,pri,F#2,C2,C3,0 bank
```

## Bugs

Due to the reverse engineering nature of the project, there might be errors.
This has only be tested on an EMU ESI-2000.

### Related project

[emu3fs](https://github.com/dagargo/emu3fs) is a Linux kernel module that allows to read from and write to disks formatted in an E-MU emu3 sampler family filesystem.

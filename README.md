# emu3bm

emu3bm is an E-Mu EIII and EIV bank manager. It allows to create banks, add and extract mono or stereo samples and edit some preset parameters.

Although Emulator samplers from series EIII and EIV use the same filesystem, the bank format is very different and only banks for the EIII series are supported.

On EIV banks, content listing and sample extraction is possible by using the `emu4bm` binary included in this project. Note that `emu4bm` is very experimental.

## Installation

As with other autotools project, you need to run the following commands.

```
autoreconf --install
./configure
make
sudo make install
```

## Features not available in the sampler

* Bulk edition.
* Set the pitch bend range beyond the 12 semitones limit.

## Examples

Use `-v` for additional information and use it more than once for even more information.

List presets and samples in a bank.

```
$ emu3bm Vintage+InstrmtX
Preset 001: Grand Piano
Preset 002: Slack Piano
Preset 003: Tight Piano
[...]
Sample 001: puls98c0
Sample 002: puls96c0
Sample 003: puls94c0
[...]

$ emu4bm 619\ Grooved
Sample 001: 619 Grvd Kick 1
Sample 002: 619 Grvd Kick 2
Sample 003: 619 Grvd Snare 1
[...]
```

Extract samples from existing bank.

```
$ emu3bm -x bank
$ emu4bm -x bank
```

Create a new bank.

```
$ emu3bm -d esi2000 -n bank
$ emu3bm -d emu3x -n bank
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

Add a primary layer to preset 0 from sample 001 with original key F#2 from C2 to C3. Upper and lower case note names are allowed.

```
$ emu3bm -e 0 -z 1,pri,F#2,c2,c3 bank
```

The same thing can be accomplished by using key numbers.

```
$ emu3bm -e 0 -Z 1,pri,33,27,38 bank
```

Set preset 0 filter to the first one.

```
$ emu3bm -e 0 -f 0 bank
```

Set preset 0 cutoff to 200.

```
$ emu3bm -e 0 -c 200 bank
```

Set preset 0 Q to 10.

```
$ emu3bm -e 0 -q 10 bank
```

Set preset 0 VCA level to 100%.

```
$ emu3bm -e 0 -l 100 bank
```

Set preset 0 pitch bend range to 24.

```
$ emu3bm -e 0 -b 24 bank
```

Set all presets realtime controllers. In this case, we are setting:
- Pitch Control to Pitch
- Mod Control to LFO -> Pitch
- Pressure Control to Attack
- Pedal Control to Crossfade
- MIDI A Control to VCF Cutoff
- MIDI B Control to VCF NoteOn Q
- Footswitch 1 to Off
- Footswitch 2 to Off

```
$ emu3bm -r 1,4,8,9,2,10,0,0 bank
```

## Related projects

* [emu3fs](https://github.com/dagargo/emu3fs) is a Linux kernel module that allows to read from and write to block devices formatted as E-Mu EIII filesystem.
* [Elektroid](https://github.com/dagargo/elektroid) is a sample and MIDI device manager with support for MIDI SDS.

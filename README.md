# emu3bm

emu3bm is an E-Mu EIII and EIV bank manager. It allows to create banks, add and extract mono or stereo samples and edit some preset parameters.

Although Emulator samplers from series EIII and EIV use the same filesystem, the bank format is very different and only banks for the EIII series are supported.

On EIV banks, content listing and sample extraction is possible by using the `emu4bm` binary included in this project. Note that `emu4bm` is very experimental.

Other interesting features are:

* Preset import from SFZ files
* Bulk edition
* Setting the pitch bend range beyond the 12 semitones limit

## Installation

As with other autotools project, you need to run the following commands.

```
autoreconf --install
./configure
make
sudo make install
```

The package dependencies for Debian-based distributions are:
- automake
- libtool
- build-essential
- libsndfile1-dev
- libsamplerate0-dev
- flex
- bison

You can easily install them by running this.

```
sudo apt install automake libtool build-essential libsndfile1-dev libsamplerate0-dev flex bison`
```

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

Extract samples from existing bank including the loop points and the loop enabled option. Use `-X` to prepend the sample number.

```
$ emu3bm -x bank
$ emu4bm -x bank
```

Create a new bank.

```
$ emu3bm -d esi2000 -n bank
$ emu3bm -d emu3x -n bank
$ emu4bm -n bank
```

Import a sample including the loop points and the loop enabled in it.

```
$ emu3bm -s bd.wav bank
$ emu4bm -s bd.wav bank
```

Add a new preset from a SFZ file.

```
$ emu3bm -S marimba.sfz bank
```

Some notes on SFZ support

* Implemented opcodes:
  - `ampeg_attack`, `ampeg_decay`, `ampeg_hold`, `ampeg_release` and `ampeg_sustain`
  - `amp_veltrack`
  - `cutoff` and `resonance`
  - `bend_up` and `bendup` (alias)
  - `fileg_attack`, `fileg_decay`, `fileg_hold`, `fileg_release` and `fileg_sustain`
  - `fil_veltrack`
  - `hivel` and `lovel`
  - `key`, `lokey`, `hikey` and `pitch_keycenter`
  - `sample`
* The basic unit of an SFZ instrument is the region, which is equivalent to a zone in the EIII bank terminology. However, not all opcodes are available as zone parameters, such as the pitch bend. To overcome this, these opcodes will be processed only if they are set in a higher level such in `<global>` or `<group>`.
* As a zone can only have 2 layers, velocity ranges are limited to 2 samples. Instead of using this approach, it has been opted for using linked presets, as this allows as many velocity ranges as MIDI notes. Notice, that the preset to be used should be the one ending with `L0`.

When adding samples with any of these methods, it is possible to limit the sample rate with `-R` and to limit the bit depth with `B`.

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

## Implementation details and device limitations

This section includes some notes on implementation details and device limitations that are worth sharing even though they might have nothing to do with the code in the project.

### Loop points

* Loop start must be greater or equal than `2`.
* Loop end must be lower than `lenth - 2`
* Loop length need to be 10 samples at least.
* Based on all this constraints, `emu3bm` will try its best to convert loop points. Also, if loop start equals loop end, loop is disabled.

### MIDI notes

* Device note range go from `A-1` (MIDI note `21`) to `C7` (MIDI note `108`). Notice that other vendors call the same MIDI notes `A0` and `C8`.
* Based on this, `emu3bm` will not be able to set the note outside this range regardless of the name, and exporting might change the note number.

### Other details

* SDS upload seems to add a zero sample at the beginning and remove the last one. This means that on every upload-download pair of operations, the last sample would be shifted out.

## Related projects

* [emu3fs](https://github.com/dagargo/emu3fs) is a Linux kernel module that allows to read from and write to block devices formatted as E-Mu EIII filesystem.
* [Elektroid](https://github.com/dagargo/elektroid) is a sample and MIDI device manager with support for MIDI SDS.

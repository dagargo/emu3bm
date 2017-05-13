/*
 *	emu3bm.c
 *	Copyright (C) 2016 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   EMU3 Filesystem Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EMU3 Filesystem Tool is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with emu3bm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emu3bm.h"

extern int verbosity;

const char *RT_CONTROLS_SRC[] = {
	"Pitch Control",
	"Mod Control",
	"Pressure Control",
	"Pedal Control",
	"MIDI A Control",
	"MIDI B Control"
};

const int RT_CONTROLS_SRC_SIZE = sizeof(RT_CONTROLS_SRC) / sizeof(char *);

const char *RT_CONTROLS_DST[] = {
	"Off",
	"Pitch",
	"VCF Cutoff",
	"VCA Level",
	"LFO -> Pitch",
	"LFO -> Cutoff",
	"LFO -> VCA",
	"Pan",
	"Attack",
	"Crossfade",
	"VCF NoteOn Q"
};

const int RT_CONTROLS_DST_SIZE = sizeof(RT_CONTROLS_DST) / sizeof(char *);

const char *RT_CONTROLS_FS_SRC[] = {
	"Footswitch 1",
	"Footswitch 2",
};

const int RT_CONTROLS_FS_SRC_SIZE =
	sizeof(RT_CONTROLS_FS_SRC) / sizeof(char *);

const char *RT_CONTROLS_FS_DST[] = {
	"Off",
	"Sustain",
	"Cross-Switch",
	"Unused 1",
	"Unused 2",
	"Unused 3",
	"Unused A",
	"Unused B",
	"Preset Increment",
	"Preset Decrement"
};

const int RT_CONTROLS_FS_DST_SIZE =
	sizeof(RT_CONTROLS_FS_DST) / sizeof(char *);

const char *LFO_SHAPE[] = {
	"triangle",
	"sine",
	"sawtooth",
	"square"
};

const char *VCF_TYPE[] = {
	"2 Pole Lowpass",
	"4 Pole Lowpass",
	"6 Pole Lowpass",
	"2nd Ord Hipass",
	"4th Ord Hipass",
	"2nd O Bandpass",
	"4th O Bandpass",
	"Contrary BandP",
	"Swept EQ 1 oct",
	"Swept EQ 2->1",
	"Swept EQ 3->1",
	"Phaser 1",
	"Phaser 2",
	"Bat-Phaser",
	"Flanger Lite",
	"Vocal Ah-Ay-Ee",
	"Vocal Oo-Ah",
	"Bottom Feeder",
	"ESi/E3x Lopass",
	"Unknown"
};

const int VCF_TYPE_SIZE = sizeof(VCF_TYPE) / sizeof(char *);

char *
emu3_e3name_to_filename(const char *objname)
{
	int i, size;
	const char *index = &objname[NAME_SIZE - 1];
	char *fname;

	for (size = NAME_SIZE; size > 0; size--) {
		if (*index != ' ')
			break;
		index--;
	}
	fname = (char*)malloc(size + 1);
	strncpy(fname, objname, size);
	fname[size] = '\0';
	for (i = 0; i < size; i++)
		if (fname[i] == '/')
			fname[i] = '?';

	return fname;
}

char *
emu3_e3name_to_wav_filename(const char *e3name)
{
	char *fname = emu3_e3name_to_filename(e3name);
	char *wname = malloc(strlen(fname) + 5);

	strcpy(wname, fname);
	strcat(wname, SAMPLE_EXT);
	return wname;
}

//TODO: complete add non case sensitive
char *
emu3_wav_filename_to_filename(const char *wav_file)
{
	char *filename = malloc(strlen(wav_file) + 1);

	strcpy(filename, wav_file);
	char *ext = strrchr(wav_file, '.');
	if (strcmp(ext, SAMPLE_EXT) == 0) {
		free(filename);
		int len_wo_ext = strlen(wav_file) - strlen(SAMPLE_EXT);
		filename = malloc(len_wo_ext + 1);
		strncpy(filename, wav_file, len_wo_ext);
		filename[len_wo_ext] = '\0';
	}
	return filename;
}

char *
emu3_str_to_e3name(const char *src)
{
	int len = strlen(src);
	char *e3name = malloc(len + 1);

	strcpy(e3name, src);
	char *c = e3name;
	for (int i = 0; i < len; i++, c++)
		if (!isalnum(*c) && *c != ' ' && *c != '#')
			*c = '?';
	return e3name;
}

void
emu3_cpystr(char *dst, const char *src)
{
	int len = strlen(src);

	memcpy(dst, src, NAME_SIZE);
	memset(&dst[len], ' ', NAME_SIZE - len);
}


int
emu3_get_sample_channels(struct emu3_sample *sample)
{
	int channels;

	switch (sample->format) {
	case STEREO_SAMPLE_1:
	case STEREO_SAMPLE_2:
		channels = 2;
		break;
	case MONO_SAMPLE_1:
	case MONO_SAMPLE_2:
	case MONO_SAMPLE_EMULATOR_3X_1:
	case MONO_SAMPLE_EMULATOR_3X_2:
	case MONO_SAMPLE_EMULATOR_3X_3:
	case MONO_SAMPLE_EMULATOR_3X_4:
	case MONO_SAMPLE_EMULATOR_3X_5:
	default:
		channels = 1;
		break;
	}
	return channels;
}

void
emu3_print_sample_info(struct emu3_sample *sample, sf_count_t nframes)
{
	for (int i = 0; i < SAMPLE_PARAMETERS; i++)
		log(2, "0x%08x ", sample->parameters[i]);
	log(2, "\n");
	log(1, "Channels: %d\n", emu3_get_sample_channels(sample));
	log(1, "Frames: %" PRId64 "\n", nframes);
	log(1, "Sample rate: %dHz\n", sample->sample_rate);
}

void
emu3_print_zone_info(struct emu3_preset_zone *zone)
{
	//Level: [0, 0x7f] -> [0, 100]
	log(1, "VCA level: %d\n", (int)(zone->vca_level * 100 / 127));
	//Pan: [0, 0x80] -> [-100, +100]
	log(1, "VCA pan: %d\n", (int)((zone->vca_pan - 0x40) * 1.5625));
	int vcf_type = (unsigned char)zone->vcf_type_lfo_shape >> 3;
	if (vcf_type > VCF_TYPE_SIZE - 1)
		vcf_type = VCF_TYPE_SIZE - 1;
	log(1, "VCF type (%d): %s\n", vcf_type, VCF_TYPE[vcf_type]);
	//Cutoff: [0, 255] -> [26, 74040]
	int cutoff = zone->vcf_cutoff;
	log(1, "VCF cutoff: %d\n", cutoff);
	//Q: [0x80, 0xff] -> [0, 100]
	int q = (zone->vcf_q - 0x80) * 100 / 127;
	log(1, "VCF Q: %d\n", q);
	log(1, "LFO shape: %s\n", LFO_SHAPE[zone->vcf_type_lfo_shape & 0x3]);
}

void
emu3_print_preset_info(struct emu3_preset *preset)
{
	for (int i = 0; i < RT_CONTROLS_SRC_SIZE; i++) {
		int dst = 0;
		for (int j = 0; j < RT_CONTROLS_SIZE; j++) {
			if (preset->rt_controls[j] == i + 1) {
				dst = j + 1;
				break;
			}
		}
		log(1, "Mapping: %s - %s\n", RT_CONTROLS_SRC[i], RT_CONTROLS_DST[dst]);
	}
	for (int i = 0; i < RT_CONTROLS_FS_SIZE; i++) {
		log(1, "Mapping: %s - %s\n",
		    RT_CONTROLS_FS_SRC[i],
		    RT_CONTROLS_FS_DST[preset->rt_controls[RT_CONTROLS_SIZE + i]]);
	}
}

void
emu3_set_preset_rt_control(struct emu3_preset *preset, int src, int dst)
{
	if (dst >= 0 && dst < RT_CONTROLS_DST_SIZE) {
		log(0, "Setting controller %s to %s...\n",
		    RT_CONTROLS_SRC[src], RT_CONTROLS_DST[dst]);
		if (dst >= 0) {
			for (int i = 0; i < RT_CONTROLS_SIZE; i++)
				if (preset->rt_controls[i] == src + 1)
					preset->rt_controls[i] = 0;
			if (dst > 0)
				preset->rt_controls[dst - 1] = src + 1;
		}
	}else
		fprintf(stderr, "Invalid destination %d for %s\n", dst,
			RT_CONTROLS_SRC[src]);
}

void
emu3_set_preset_rt_control_fs(struct emu3_preset *preset, int src, int dst)
{
	if (dst >= 0 && dst < RT_CONTROLS_FS_DST_SIZE) {
		log(0, "Setting controller %s to %s...\n",
		    RT_CONTROLS_FS_SRC[src], RT_CONTROLS_FS_DST[dst]);
		preset->rt_controls[src + RT_CONTROLS_FS_DST_SIZE] = dst;
	}else
		fprintf(stderr, "Invalid destination %d for %s\n", dst,
			RT_CONTROLS_FS_SRC[src]);
}

void
emu3_set_preset_level(struct emu3_preset_zone *zone, int level)
{
	if (level < 0 || level > 100)
		fprintf(stderr, "Value %d for level not in range [0, 100]\n", level);
	else{
		log(0, "Setting level to %d...\n", level);
		zone->vca_level = (unsigned char)(level * 127 / 100);
	}
}

void
emu3_set_preset_cutoff(struct emu3_preset_zone *zone, int cutoff)
{
	if (cutoff < 0 || cutoff > 255)
		fprintf(stderr, "Value for cutoff %d not in range [0, 255]\n", cutoff);
	else{
		log(0, "Setting cutoff to %d...\n", cutoff);
		zone->vcf_cutoff = (unsigned char)cutoff;
	}
}

void
emu3_set_preset_q(struct emu3_preset_zone *zone, int q)
{
	if (q < 0 || q > 100)
		fprintf(stderr, "Value %d for Q not in range [0, 100]\n", q);
	else{
		log(0, "Setting Q to %d...\n", q);
		zone->vcf_q = (unsigned char)((q * 127 / 100) + 0x80);
	}
}

void
emu3_set_preset_filter(struct emu3_preset_zone *zone, int filter)
{
	if (filter < 0 || filter > VCF_TYPE_SIZE - 2)
		fprintf(stderr, "Value %d for filter not in range [0, %d]\n", filter,
			VCF_TYPE_SIZE - 2);
	else{
		log(0, "Setting filter to %s...\n", VCF_TYPE[filter]);
		zone->vcf_type_lfo_shape =
			((unsigned char)filter) << 3 | zone->vcf_type_lfo_shape & 0x3;
	}
}

void
emu3_set_preset_rt_controls(struct emu3_preset *preset, char *rt_controls)
{
	char *token;
	char *endtoken;
	int i;
	int controller;

	log(0, "Setting realtime controls...\n");
	i = 0;
	while (i < RT_CONTROLS_SIZE && (token = strsep(&rt_controls, ",")) != NULL) {
		if (*token == '\0')
			fprintf(stderr, "Empty value\n");
		else{
			controller = strtol(token, &endtoken, 10);
			if (*endtoken == '\0') {
				if (i < RT_CONTROLS_SRC_SIZE)
					emu3_set_preset_rt_control(preset, i, controller);
				else if (i < RT_CONTROLS_SRC_SIZE + RT_CONTROLS_FS_SRC_SIZE) {
					emu3_set_preset_rt_control_fs(preset,
								      i - RT_CONTROLS_SRC_SIZE,
								      controller);
				}else
					fprintf(stderr, "Too many controls\n");
			}else
				fprintf(stderr, "'%s' not a number\n", token);
		}
		i++;
	}
}

void
emu3_init_sample_descriptor(struct emu3_sample_descriptor *sd,
			    struct emu3_sample *sample, int frames)
{
	sd->sample = sample;

	sd->l_channel = sample->frames;
	if (sample->format == STEREO_SAMPLE_1)
		//We consider the 4 shorts padding that the left channel has
		sd->r_channel = sample->frames + frames + 4;
}

void
emu3_write_frame(struct emu3_sample_descriptor *sd, short int frame[])
{
	*sd->l_channel = frame[0];
	sd->l_channel++;
	if (sd->sample->format == STEREO_SAMPLE_1) {
		*sd->r_channel = frame[1];
		sd->r_channel++;
	}
}

//returns the sample size in bytes that the the sample takes in the bank
int
emu3_add_sample(char *path, struct emu3_sample *sample,
		unsigned int address, int sample_id)
{
	SF_INFO input_info;
	SNDFILE *input;
	int size = 0;
	unsigned int data_size;
	short int frame[2];
	short int zero[] = { 0, 0 };
	int mono_size;
	const char *filename;
	struct emu3_sample_descriptor sd;

	input_info.format = 0;
	input = sf_open(path, SFM_READ, &input_info);
	//TODO: add more formats
	if ((input_info.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV)
		fprintf(stderr, "Sample not in a valid format. Skipping...\n");
	else if (input_info.channels > 2)
		fprintf(stderr, "Sample neither mono nor stereo. Skipping...\n");
	else{
		filename = basename(path);
		log(1, "Appending sample %s... (%" PRId64 " frames, %d channels)\n",
		    filename, input_info.frames, input_info.channels);
		//Sample header initialization
		char *name = emu3_wav_filename_to_filename(filename);
		char *e3name = emu3_str_to_e3name(name);
		emu3_cpystr(sample->name, e3name);
		free(name);
		free(e3name);

		data_size = sizeof(short int) * (input_info.frames + 4);
		mono_size = sizeof(struct emu3_sample) + data_size;
		size = mono_size + (input_info.channels == 1 ? 0 : data_size);
		sample->parameters[0] = 0;
		//Start of left channel
		sample->parameters[1] = sizeof(struct emu3_sample);
		//Start of right channel
		sample->parameters[2] = input_info.channels == 1 ? 0 : mono_size;
		//Last sample of left channel
		sample->parameters[3] = mono_size - 2;
		//Last sample of right channel
		sample->parameters[4] = input_info.channels == 1 ? 0 : size - 2;

		int loop_start = 4; //This is an arbitrary value
		//Example (mono and stereo): Loop start @ 9290 sample is stored as ((9290 + 2) * 2) + sizeof (struct emu3_sample)
		sample->parameters[5] =
			((loop_start + 2) * 2) + sizeof(struct emu3_sample);
		//Example
		//Mono: Loop start @ 9290 sample is stored as (9290 + 2) * 2
		//Stereo: Frames * 2 + parameters[5] + 8
		sample->parameters[6] =
			input_info.channels ==
			1 ? (loop_start + 2) * 2 : input_info.frames * 2 +
			sample->parameters[5] + 8;

		int loop_end = input_info.frames - 10; //This is an arbitrary value
		//Example (mono and stereo): Loop end @ 94008 sample is stored as ((94008 + 1) * 2) + sizeof (struct emu3_sample)
		sample->parameters[7] =
			((loop_end + 1) * 2) + sizeof(struct emu3_sample);
		//Example
		//Mono: Loop end @ 94008 sample is stored as ((94008 + 1) * 2)
		//Stereo: Frames * 2 + parameters[7] + 8
		sample->parameters[8] =
			input_info.channels ==
			1 ? (loop_end + 1) * 2 : input_info.frames * 2 +
			sample->parameters[7] + 8;

		sample->sample_rate = DEFAULT_SAMPLING_FREQ;

		sample->format =
			input_info.channels == 1 ? MONO_SAMPLE_1 : STEREO_SAMPLE_1;

		for (int i = 0; i < MORE_SAMPLE_PARAMETERS; i++)
			sample->more_parameters[i] = 0;

		emu3_init_sample_descriptor(&sd, sample, input_info.frames);

		//2 first frames set to 0
		emu3_write_frame(&sd, zero);
		emu3_write_frame(&sd, zero);

		for (int i = 0; i < input_info.frames; i++) {
			sf_readf_short(input, frame, 1);
			emu3_write_frame(&sd, frame);
		}

		//2 end frames set to 0
		emu3_write_frame(&sd, zero);
		emu3_write_frame(&sd, zero);

		log(1, "Appended %dB (0x%08x).\n", size, size);
	}
	sf_close(input);

	return size;
}

void
emu3_write_sample_file(struct emu3_sample *sample, sf_count_t nframes)
{
	SF_INFO output_info;
	SNDFILE *output;
	char *wav_file;
	short *l_channel, *r_channel;
	short frame[2];
	char *schannels;
	int channels = emu3_get_sample_channels(sample);
	int samplerate = sample->sample_rate;

	output_info.frames = nframes;
	output_info.samplerate = samplerate;
	output_info.channels = channels;
	output_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	wav_file = emu3_e3name_to_wav_filename(sample->name);
	schannels = channels == 1 ? "mono" : "stereo";
	log(0, "Extracting %s sample %s...\n", schannels, wav_file);

	output = sf_open(wav_file, SFM_WRITE, &output_info);
	l_channel = sample->frames + 2;
	if (channels == 2)
		r_channel = sample->frames + nframes + 6;
	for (int i = 0; i < nframes; i++) {
		frame[0] = *l_channel;
		l_channel++;
		if (channels == 2) {
			frame[1] = *r_channel;
			r_channel++;
		}
		if (!sf_writef_short(output, frame, 1)) {
			fprintf(stderr, "Error: %s\n", sf_strerror(output));
			break;
		}
	}
	sf_close(output);
}

unsigned int
emu3_get_preset_address(const char *format, unsigned int offset)
{
	unsigned int address = 0;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		address = PRESET_START_EMU_3X + offset;
	else if (format == EMULATOR_THREE_DEF)
		address = PRESET_START_EMU_THREE + offset - PRESET_OFFSET_EMU_THREE;
	return address;
}

unsigned int
emu3_get_sample_start_address(const char *format, unsigned int offset)
{
	unsigned int address = 0;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		//There is always a 0xee byte before the samples
		address = PRESET_START_EMU_3X + offset + 1;
	else if (format == EMULATOR_THREE_DEF)
		//There is always a 0x00 byte before the samples
		address = PRESET_START_EMU_THREE + offset + 1 - PRESET_OFFSET_EMU_THREE;
	return address;
}

unsigned int *
emu3_get_sample_addresses(const char *format, char *memory)
{
	unsigned int *addresses = NULL;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		addresses = (unsigned int*)&(memory[SAMPLE_ADDR_START_EMU_3X]);
	if (format == EMULATOR_THREE_DEF)
		addresses = (unsigned int*)&(memory[SAMPLE_ADDR_START_EMU_THREE]);
	return addresses;
}

unsigned int *
emu3_get_preset_addresses(const char *format, char *memory)
{
	unsigned int *addresses = NULL;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		addresses = (unsigned int*)&(memory[PRESET_SIZE_ADDR_START_EMU_3X]);
	if (format == EMULATOR_THREE_DEF)
		addresses =
			(unsigned int*)&(memory[PRESET_SIZE_ADDR_START_EMU_THREE]);
	return addresses;
}

char *
emu3_get_bank_format(struct emu3_bank *bank)
{
	char *format = NULL;

	if (strncmp(EMULATOR_3X_DEF, bank->signature, SIGNATURE_SIZE) == 0)
		format = EMULATOR_3X_DEF;
	else if (strncmp(ESI_32_V3_DEF, bank->signature, SIGNATURE_SIZE) == 0)
		format = ESI_32_V3_DEF;
	else if (strncmp(EMULATOR_THREE_DEF, bank->signature, SIGNATURE_SIZE) == 0)
		format = EMULATOR_THREE_DEF;
	return format;
}

int
emu3_get_max_samples(const char *format)
{
	int max_samples = 0;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		max_samples = MAX_SAMPLES_EMU_3X;
	if (format == EMULATOR_THREE_DEF)
		max_samples = MAX_SAMPLES_EMU_THREE;
	return max_samples;
}

int
emu3_get_max_presets(const char *format)
{
	int max_presets = 0;

	if (format == EMULATOR_3X_DEF || format == ESI_32_V3_DEF)
		max_presets = MAX_PRESETS_EMU_3X;
	if (format == EMULATOR_THREE_DEF)
		max_presets = MAX_PRESETS_EMU_THREE;
	return max_presets;
}

int
emu3_process_bank(const char *ifile, int aflg, char *afile, int xflg,
		  char *rt_controls, int level, int cutoff, int q,
		  int filter)
{
	char *memory;
	FILE *file;
	int size, i, channels;
	size_t fsize;
	struct emu3_bank *bank;
	char *name;
	unsigned int *addresses;
	unsigned int sample_start_addr;
	unsigned int next_sample_addr;
	unsigned int address;
	struct emu3_sample *sample;
	char *format = NULL;
	struct emu3_preset *preset;
	struct emu3_preset_zone *zones;
	int max_samples;

	file = fopen(ifile, "r");

	if (file == NULL) {
		fprintf(stderr, "Error: file %s could not be opened.\n", ifile);
		return -1;
	}
	memory = (char*)malloc(MEM_SIZE);

	fsize = fread(memory, 1, MEM_SIZE, file);
	fclose(file);

	log(1, "Bank fsize: %zuB\n", fsize);

	bank = (struct emu3_bank *)memory;

	format = emu3_get_bank_format(bank);

	if (!format) {
		fprintf(stderr, "Bank format not supported.\n");
		return 1;
	}

	log(0, "Bank name: %.*s\n", NAME_SIZE, bank->name);
	log(1, "Bank format: %s\n", bank->signature);

	log(2, "Geometry:\n");
	for (i = 0; i < BANK_PARAMETERS; i++)
		log(2, "Parameter %2d: 0x%08x (%d)\n", i, bank->parameters[i],
		    bank->parameters[i]);
	log(2, "Objects (p 0): %d\n", bank->parameters[0] + 1);
	log(2, "Unknown (p 3): 0x%08x\n", bank->parameters[3]);
	log(2, "Unknown (p 4): 0x%08x\n", bank->parameters[4]);
	log(2, "Next    (p 5): 0x%08x\n", bank->parameters[5]);
	log(2, "Unknown (p 7): 0x%08x\n", bank->parameters[7]);
	log(2, "Unknown (p 8): 0x%08x\n", bank->parameters[8]);
	log(2, "Unknown (p10): 0x%08x\n", bank->parameters[10]);

	if (bank->parameters[7] + bank->parameters[8] != bank->parameters[10])
		log(2, "Kind of checksum error.\n");

	if (strncmp(bank->name, bank->name_copy, NAME_SIZE))
		log(2, "Bank name is different than previously found.\n");

	log(2, "More geometry:\n");
	for (i = 0; i < MORE_BANK_PARAMETERS; i++)
		log(2, "Parameter %d: 0x%08x (%d)\n", i, bank->more_parameters[i],
		    bank->more_parameters[i]);

	log(2, "Current preset: %d\n", bank->more_parameters[0]);
	log(2, "Current sample: %d\n", bank->more_parameters[1]);

	addresses = emu3_get_preset_addresses(format, memory);

	for (i = 0; i < emu3_get_max_presets(format); i++) {
		if (addresses[0] != addresses[1]) {
			address = emu3_get_preset_address(format, addresses[0]);
			preset = (struct emu3_preset *)&memory[address];
			log(0, "Preset %3d, %.*s", i, NAME_SIZE, preset->name);
			log(1, " @ 0x%08x", address);
			log(0, "\n");
			if (rt_controls)
				emu3_set_preset_rt_controls(preset, rt_controls);
			zones = (struct emu3_preset_zone *)
				&memory[address + sizeof(struct emu3_preset) +
					preset->nzones * 4];
			emu3_print_preset_info(preset);
			log(1, "Zones: %d\n", preset->nzones);
			for (int j = 0; j < preset->nzones; j++) {
				log(1, "Zone %d\n", j);
				if (level != -1)
					emu3_set_preset_level(&zones[j], level);
				if (cutoff != -1)
					emu3_set_preset_cutoff(&zones[j], cutoff);
				if (q != -1)
					emu3_set_preset_q(&zones[j], q);
				if (filter != -1)
					emu3_set_preset_filter(&zones[j], filter);
				emu3_print_zone_info(&zones[j]);
			}
		}
		addresses++;
	}

	sample_start_addr = emu3_get_sample_start_address(format, addresses[0]);
	log(1, "Sample start: 0x%08x\n", sample_start_addr);

	max_samples = emu3_get_max_samples(format);
	addresses = emu3_get_sample_addresses(format, memory);
	log(1, "Start with offset: 0x%08x\n", addresses[0]);
	log(1, "Next with offset: 0x%08x\n", addresses[max_samples - 1]);
	next_sample_addr =
		sample_start_addr + addresses[max_samples - 1] - SAMPLE_OFFSET;
	log(1, "Next sample: 0x%08x\n", next_sample_addr);

	for (i = 0; i < max_samples; i++) {
		if (addresses[i] == 0x0)
			break;
		address = sample_start_addr + addresses[i] - SAMPLE_OFFSET;
		if (addresses[i + 1] == 0x0)
			size = next_sample_addr - address;
		else
			size = addresses[i + 1] - addresses[i];
		sample = (struct emu3_sample *)&memory[address];
		channels = emu3_get_sample_channels(sample);
		//We divide between the bytes per frame (number of channels * 2 bytes)
		//The 16 or 8 bytes are the 4 or 8 short int used for padding.
		sf_count_t nframes =
			(size - sizeof(struct emu3_sample) -
			 (8 * channels)) / (2 * channels);
		log(0, "Sample %3d, %.*s", i + 1, NAME_SIZE, sample->name);
		log(1, " @ 0x%08x (size %dB, frames %" PRId64 ")", address, size, nframes);
		log(0, "\n");
		emu3_print_sample_info(sample, nframes);

		if (xflg)
			emu3_write_sample_file(sample, nframes);
	}

	if (aflg) {
		sample = (struct emu3_sample *)&memory[next_sample_addr];
		size =
			emu3_add_sample(afile, sample,
					next_sample_addr - sample_start_addr, i + 1);
		if (size) {
			addresses[i] = addresses[max_samples - 1];
			addresses[max_samples - 1] =
				next_sample_addr + size - sample_start_addr + SAMPLE_OFFSET;
			bank->parameters[0]++;
			bank->parameters[5] = next_sample_addr + size - sample_start_addr;
		}else
			fprintf(stderr, "Error while adding sample.\n");
	}

	if ((aflg && size) || rt_controls || level != -1 || cutoff != -1 || q != -1
	    || filter != -1) {
		file = fopen(ifile, "w");
		if (file) {
			fwrite(memory, 1, next_sample_addr + size, file);
			fclose(file);
		} else {
			fprintf(stderr, "Error while opening file for writing\n");
		}
	}

	return EXIT_SUCCESS;
}

int
emu3_create_bank(const char *ifile)
{
	struct emu3_bank bank;
	char *name = emu3_str_to_e3name(ifile);
	char *fname = emu3_e3name_to_filename(name);
	char *path =
		malloc(strlen(DATADIR) + strlen(PACKAGE) + strlen(EMPTY_BANK) + 3);
	int ret = sprintf(path, "%s/%s/%s", DATADIR, PACKAGE, EMPTY_BANK);

	if (ret < 0) {
		fprintf(stderr, "Error while creating full path");
		return EXIT_FAILURE;
	}

	char buf[BUFSIZ];
	size_t size;

	FILE *src = fopen(path, "rb");
	if (!src) {
		fprintf(stderr, "Error while opening %s for input\n", path);
		return EXIT_FAILURE;
	}

	FILE *dst = fopen(fname, "w+b");
	if (!dst) {
		fprintf(stderr, "Error while opening %s for output\n", fname);
		return EXIT_FAILURE;
	}

	while (size = fread(buf, 1, BUFSIZ, src))
		fwrite(buf, 1, size, dst);

	fclose(src);

	rewind(dst);

	if (fread(&bank, sizeof(struct emu3_bank), 1, dst)) {
		emu3_cpystr(bank.name, name);
		emu3_cpystr(bank.name_copy, name);
		rewind(dst);
		fwrite(&bank, sizeof(struct emu3_bank), 1, dst);
	}

	fclose(dst);

	free(name);
	free(fname);
	free(path);

	return EXIT_SUCCESS;
}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sndfile.h>

#include "fft.hh"

using namespace std;

#define FNAME "input.wav"
#define FOUT "out.wav"
#define SAMPLERATE 44100

#define NYQIST_FREQ (SAMPLERATE / 2)

//#define BUF_SIZE 1024

//44100 / 16384 ~= 4, so that makes 4 notes per second
#define BUF_SIZE 16384

#define NUM_CHANNELS 1

#define FREQ_MIN 256
#define FREQ_MAX 1500

#define FREQ_TO_BIN(f) (f / (1.0 * NYQIST_FREQ / BUF_SIZE))
#define BIN_TO_FREQ(b) (b * (1.0 * NYQIST_FREQ / BUF_SIZE))

static int FreqToNote(float freq)
{
	//def to_idx(freq); (12 * Math::log(freq / 440.0) / Math::log(2)).ceil;end
	float idx = log(freq / 440.0) / log(2.0);
	return ceil(idx * 12);
}

static void FreqIdxToName(char *buf, int idx)
{
	enum {
		NOTE_COUNT = 12,
	};
	const char *names[NOTE_COUNT] = {
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#",
		"A",
		"A#",
		"B",
	};
	
	int uidx = idx;
	while (uidx < 0) {
		uidx += NOTE_COUNT;
	}

	int name_idx = uidx % NOTE_COUNT;
	int bank_idx = 4 + (idx / NOTE_COUNT);
	sprintf(buf, "%s%d", names[name_idx], bank_idx);
}

typedef float sample_t;

static sample_t buf[NUM_CHANNELS * BUF_SIZE];
static sample_t buf_out[BUF_SIZE];
static complex<double> fft_buf[BUF_SIZE];

static void x_fft(sample_t *in, sample_t *out, size_t count, bool ifft)
{
	memset(fft_buf, 0, sizeof(complex<double>) * BUF_SIZE);
	for (size_t i = 0; i < count; i++) {
		fft_buf[i] = in[i];
	}

	fft(fft_buf, count, ifft);

	for (size_t i = 0; i < count / 2; i++) {
		out[i] = fft_buf[i].real();
	}
}

/* cut out unwanted range - a poor man's bandpass */
static void filter(sample_t *buf, size_t count)
{
	for (size_t idx = 0;
			idx < (size_t)fmin(count,
				ceil(FREQ_TO_BIN(FREQ_MIN)));
			idx++)
	{
		buf[idx] = 0;
	}
	for (size_t idx = (size_t)fmin(count - 1,
				floor(FREQ_TO_BIN(FREQ_MAX)));
			idx < count;
			idx++)
	{
		buf[idx] = 0;
	}
}

/* convert stereo interleaved to mono */
static void split(sample_t *buf, size_t count)
{
	for (size_t idx = 0; idx < count; idx++) {
		buf[idx] = buf[idx * 2];
	}
}

static void print_note(sample_t *buf, size_t count)
{
	size_t imax = 0;
	sample_t max = -99999999;
	for (size_t idx = 0; idx < count; idx++) {
		if (buf[idx] < max) {
			continue;
		}
		max = buf[idx];
		imax = idx;
	}

	//printf("MAX idx=%d freq=%f\n", imax, BIN_TO_FREQ(imax));
	char tmp[128] = {0};
	float freq = BIN_TO_FREQ(imax);
	int note_idx = FreqToNote(freq);
	FreqIdxToName(tmp, note_idx);
	printf("MAX idx=%d note=%s\n", imax, tmp);
}

int main() {
	SF_INFO info = {
		format: SF_FORMAT_WAV | SF_FORMAT_FLOAT,
		samplerate: SAMPLERATE,
		channels: NUM_CHANNELS,
	};
	SNDFILE *file = sf_open(FNAME, SFM_READ, &info);
	assert(!sf_error(0));


	assert(info.samplerate == SAMPLERATE);
	assert(info.channels == NUM_CHANNELS);
	
	SF_INFO info_out = info;
	info_out.channels = 1;
	SNDFILE *fout = sf_open(FOUT, SFM_WRITE, &info_out);

	assert(!sf_error(0));

	while (1) {
		memset(buf, 0, NUM_CHANNELS * BUF_SIZE * sizeof(sample_t));
		memset(buf_out, 0, BUF_SIZE * sizeof(sample_t));
		size_t ns = sf_read_raw(file, buf, BUF_SIZE);
		if (ns <= 0) {
			exit(-1);
		}
		size_t d = BUF_SIZE - ns;

		if (NUM_CHANNELS == 2) {
			split(buf, BUF_SIZE);
		}
		x_fft(buf, buf_out, BUF_SIZE, false);
		filter(buf_out, BUF_SIZE);
		
		print_note(buf_out, BUF_SIZE);

		x_fft(buf_out, buf_out, BUF_SIZE, true);
		sf_write_raw(fout, buf_out, BUF_SIZE);
	}

	sf_close(file);

	return 0;
}

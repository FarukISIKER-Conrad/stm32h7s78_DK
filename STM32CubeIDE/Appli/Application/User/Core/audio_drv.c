/*
 * audio_drv.c
 *
 *  Created on: 18 Ara 2025
 *      Author: Faruk Isıker
 */
#include "audio_drv.h"
#include "math.h"
#include "string.h"
#include "mp3_decoder.h"

// Global değişkenler

// MP3 decoder internal buffer (decoder'ın kendi işlemleri için)
#define MP3_DECODER_BUFFER_SIZE (2048 * 2)  // 2x ping-pong için
ALIGN_32BYTES (int16_t mp3_decoder_internal_buffer[MP3_DECODER_BUFFER_SIZE])
    __attribute__((section("BufferSection")));


mp3_decoder_streaming_t mp3_decoder;

extern const uint8_t mp3_file_data[];
extern const size_t mp3_file_size;


static void audio_drv_tx_half_callback(void* self);
static void audio_drv_tx_callback(void* self);
static void audio_drv_fill_sine_wave(audio_drv_t *self, int16_t* pData, size_t len);

int audio_drv_init(audio_drv_t *self)
{
	self->semaphore = xSemaphoreCreateBinary();
	if (self->semaphore == NULL)
		return -1;

	self->callback.tx_cplt = audio_drv_tx_callback;
	self->callback.tx_half_cplt = audio_drv_tx_half_callback;


	if (self->sine.p_tx_data== NULL || self->sine.tx_data_size == 0)
	{
		return -3;
	}


	audio_drv_update_frequency(self, self->sine.frequency);

	if (self->type == __SINE_WAVE)
	{
		audio_drv_fill_sine_wave(self, self->sine.p_tx_data, self->sine.tx_data_size);
	}
	else
	{
		HAL_SAI_DeInit(self->hsai);
		self->hsai->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
		HAL_SAI_Init(self->hsai);

		// 1. Initialize decoder
		mp3_decoder_streaming_init(&mp3_decoder,mp3_decoder_internal_buffer, self->mp3.tx_data_size / 2);

		// 2. Load MP3 data
		mp3_decoder_streaming_load(&mp3_decoder, (uint8_t *)mp3_file_data, mp3_file_size);

		// 3. Enable loop (optional)
		mp3_decoder_streaming_set_loop(&mp3_decoder, 1);

		// 4. Start - get first chunk
		mp3_decoder_streaming_start(&mp3_decoder);
		if (mp3_decoder.current_chunk == NULL) {
			// Error
			return -1;  // Hata kodu -1 olmalı (0 değil, 0 başarılı anlamına gelir)
		}

		// 5. İlk chunk'ı buffer'a kopyala (DMA başlamadan önce)
		size_t initial_samples;
		int16_t *first_chunk = mp3_decoder_streaming_next_chunk(&mp3_decoder, &initial_samples);
		if (first_chunk != NULL) {
			size_t bytes_to_copy = initial_samples * sizeof(int16_t);
			memcpy(self->mp3.p_tx_data, first_chunk, bytes_to_copy);
		}
		else {
			return -2;  // İlk chunk alınamadı
		}
	}

	if (self->is_circular_dma_enabled)
	{

	}

	return 0;
}

int audio_drv_start_dma(audio_drv_t* self)
{
	if (self->type == __SINE_WAVE )
	{
		HAL_SAI_Transmit_DMA(self->hsai, (uint8_t *)self->sine.p_tx_data, self->sine.tx_data_size);
	}
	else
	{
		HAL_SAI_Transmit_DMA(self->hsai, (uint8_t*)self->mp3.p_tx_data, self->mp3.tx_data_size);
	}
	return 0;
}

void audio_drv_update_frequency(audio_drv_t* self, float frequency)
{
	if ((frequency <= 0) || (frequency > 1000.0f))
	{
		frequency = 100.0f;
	}

	if (self->sampling_frequency <= 0.0f)
		self->sampling_frequency = 48000.0f;


	self->sine.frequency = frequency;
	self->sine.phase_inc = M_TWOPI * self->sine.frequency / self->sampling_frequency;
}

static void audio_drv_fill_sine_wave(audio_drv_t *self, int16_t* pData, size_t len)
{

	int16_t sample = 0;

	if(self->semaphore == NULL)
		return;

	for (uint16_t i = 0; i < len ; i+=2)
	{
		sample = (int16_t)(sinf(self->sine.phase) * 32767.0f); // Scale to int16 range
		pData[i] = sample;
		pData[i+1] = sample;

		self->sine.phase = self->sine.phase + self->sine.phase_inc;

		if (self->sine.phase >= M_TWOPI)
		{
			self->sine.phase -= M_TWOPI;
		}

	}
}


static void audio_drv_tx_half_callback(void* self)
{
	audio_drv_t *audio_drv = (audio_drv_t *)self;
	if (audio_drv->type == __SINE_WAVE )
	{
		audio_drv_fill_sine_wave(audio_drv, audio_drv->sine.p_tx_data, audio_drv->sine.tx_data_size / 2);
	}
	else
	{
		// DMA ilk yarıyı oynatıyor, ikinci yarıyı doldur
		size_t samples;
		int16_t *next_chunk = mp3_decoder_streaming_next_chunk(&mp3_decoder, &samples);

		if (next_chunk == NULL) {
			// End of file (loop disabled)
			HAL_SAI_DMAStop(audio_drv->hsai);
		}
		else {
			// İkinci yarıya kopyala (buffer'ın ortasından başla)
			size_t bytes_to_copy = samples * sizeof(int16_t);


			memcpy(audio_drv->mp3.p_tx_data,
			       next_chunk,
			       bytes_to_copy);
		}
	}
}

static void audio_drv_tx_callback(void* self)
{
	audio_drv_t *audio_drv = (audio_drv_t *)self;
	if (audio_drv->type == __SINE_WAVE )
	{
		audio_drv_fill_sine_wave(audio_drv, &audio_drv->sine.p_tx_data[audio_drv->sine.tx_data_size / 2], audio_drv->sine.tx_data_size / 2);
	}
	else
	{
		// DMA ikinci yarıyı oynatıyor, ilk yarıyı doldur
		size_t samples;
		int16_t *next_chunk = mp3_decoder_streaming_next_chunk(&mp3_decoder, &samples);

		if (next_chunk == NULL) {
			// End of file (loop disabled)
			HAL_SAI_DMAStop(audio_drv->hsai);
		}
		else {
			// İlk yarıya kopyala (buffer'ın başından)
			size_t bytes_to_copy = samples * sizeof(int16_t);
			memcpy(&audio_drv->mp3.p_tx_data[audio_drv->mp3.tx_data_size / 2],
			       next_chunk,
			       bytes_to_copy);
		}
	}
}


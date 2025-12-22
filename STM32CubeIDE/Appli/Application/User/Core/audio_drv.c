/*
 * audio_drv.c
 *
 *  Created on: 18 Ara 2025
 *      Author: Faruk Isıker
 */
#include "audio_drv.h"
#include "math.h"
#include "string.h"

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
	audio_drv_fill_sine_wave(self, self->sine.p_tx_data, self->sine.tx_data_size);

	if (self->is_circular_dma_enabled)
	{

	}

	return 0;
}

int audio_drv_start_dma(audio_drv_t* self)
{
	HAL_SAI_Transmit_DMA(self->hsai, (uint8_t *)self->sine.p_tx_data, self->sine.tx_data_size);
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
	audio_drv_fill_sine_wave(audio_drv, audio_drv->sine.p_tx_data, audio_drv->sine.tx_data_size / 2);
}

static void audio_drv_tx_callback(void* self)
{
	audio_drv_t *audio_drv = (audio_drv_t *)self;
	audio_drv_fill_sine_wave(audio_drv, &audio_drv->sine.p_tx_data[audio_drv->sine.tx_data_size / 2], audio_drv->sine.tx_data_size / 2);
}



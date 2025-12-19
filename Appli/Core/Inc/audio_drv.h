#ifndef __AUDIO_DRV_H
#define __AUDIO_DRV_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef struct
{
	size_t tx_data_size;
	int16_t* p_tx_data;
	float phase;
	volatile float phase_inc;
	float frequency;
} audio_drv_sine_wave_t;

typedef struct
{
	void (*tx_half_cplt)(void *self);
	void (*tx_cplt)(void *self);
} audio_drv_callback_t;

typedef struct
{
	SAI_HandleTypeDef *hsai;
	SemaphoreHandle_t semaphore;
	TaskHandle_t task_handle;
	char task_name[20];

	uint8_t is_circular_dma_enabled;
	audio_drv_callback_t callback;
	audio_drv_sine_wave_t sine;
	float sampling_frequency;
} audio_drv_t;


int audio_drv_init(audio_drv_t *self);
int audio_drv_start_dma(audio_drv_t* self);
void audio_drv_update_frequency(audio_drv_t* self, float frequency);
#endif /* __AUDIO_DRV_H */

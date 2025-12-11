#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
#include "cmsis_os2.h"
extern osMessageQueueId_t queue_gpioHandle;
#endif

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
	if (osMessageQueueGet(queue_gpioHandle, (uint16_t *)&queue_res, NULL, 0) == osOK)
	{

		modelListener->handleKeyEvent(queue_res);
	}
}

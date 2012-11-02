/*
 Copyright (C) 2009 Erik Hjortsberg

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "TaskExecutor.h"
#include "TaskQueue.h"
#include "TaskExecutionContext.h"
#include "TaskUnit.h"
#include "framework/LoggingInstance.h"

#include <thread>

namespace Ember
{

namespace Tasks
{
TaskExecutor::TaskExecutor(TaskQueue& taskQueue) :
	mTaskQueue(taskQueue), mActive(true)
{
	mThread = new std::thread([&](){this->run();});
}

TaskExecutor::~TaskExecutor()
{
	delete mThread;
}

void TaskExecutor::run()
{
	while (mActive) {
		TaskUnit* taskUnit = mTaskQueue.fetchNextTask();
		//If the queue returns a null pointer, it means that the queue is being shut down, and this executor is expected to exit its main processing loop.
		if (taskUnit) {
			try {
				TaskExecutionContext context(*this, *taskUnit);
				taskUnit->executeInBackgroundThread(context);
				mTaskQueue.addProcessedTask(taskUnit);
			} catch (const std::exception& ex) {
				S_LOG_CRITICAL("Error when executing task in background." << ex);
				delete taskUnit;
			} catch (...) {
				S_LOG_CRITICAL("Unknown error when executing task in background.");
				delete taskUnit;
			}
		} else {
			break;
		}
	}
}

void TaskExecutor::setActive(bool active)
{
	mActive = active;
}

void TaskExecutor::join()
{
	mThread->join();
}

}
}

/*
  Copyright (C) 2008 Romulo Fernandes Machado (nightz)
  Copyright (C) 2008 Erik Hjortsberg <erik.hjortsberg@gmail.com>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SoundGroup.h"
#include "SoundDefinition.h"

#include "framework/LoggingInstance.h"


#include "services/EmberServices.h"
#include "services/sound/SoundService.h"
#include "services/sound/SoundInstance.h"
#include "services/sound/SoundSource.h"
#include "services/sound/SoundSample.h"

#ifndef _MSC_VER
// Need to find a cross platform solution
// to this
#include <sys/time.h>
#endif

namespace Ember
{
namespace OgreView
{

SoundGroupBinding::SoundGroupBinding(SoundSource& source, SoundGroup& soundGroup)
: SoundBinding(source), mSoundGroup(soundGroup)
{
	const SoundGroup::SampleStore& samples = mSoundGroup.getSamples();
	std::unique_ptr<ALuint> buffers(new ALuint[samples.size()]);
	ALuint* pbuffers = buffers.get();
	int i = 0;
	//get the buffers and bind the source to them
	for (SoundGroup::SampleStore::const_iterator I = samples.begin(); I != samples.end(); ++I) 
	{
		BaseSoundSample::BufferStore sampleBuffers = (*I)->getBuffers();
		pbuffers[i] = *(sampleBuffers.begin());
		++i;
	}
	alSourceQueueBuffers(source.getALSource(), i, pbuffers);
// 	alSourcei(source.getALSource(), AL_BUFFER, sample.getBuffer());

}

SoundGroupBinding::~SoundGroupBinding()
{
}

void SoundGroupBinding::update()
{
	//TODO: add support for streaming sounds
}



	SoundGroup::SoundGroup()
	{
		mSamples.clear();
// 		mLastPlayed = 0;
		mIsPlaying = false;
	}

	SoundGroup::~SoundGroup()
	{
	}

// 	void SoundGroup::setFrequency(const unsigned int freq)
// 	{
// 		mFrequency = freq;
// 	}

	void SoundGroup::setPlayOrder(const unsigned int playO)
	{
		mPlayOrder = playO;
// 		switch(mPlayOrder)
// 		{
// 			case PLAY_LINEAR:
// 				mNextToPlay = 0;
// 				break;
// 			case PLAY_INVERSE:
// 				mNextToPlay = mSamples.size() - 1; 
// 				break;
// 			case PLAY_RANDOM:
// 				getNextToPlay();
// 				break;
// 		};
	}

// 	void SoundGroup::getNextToPlay()
// 	{
// 		if (!mSamples.size())
// 		{
// 			S_LOG_INFO("Group has no samples to play");
// 			return;
// 		}
// 
// 		switch(mPlayOrder)
// 		{
// 			case PLAY_LINEAR:
// 				mNextToPlay++; 
// 				break;
// 			case PLAY_INVERSE:
// 				mNextToPlay--; 
// 				break;
// 			case PLAY_RANDOM:
// 				srand(time(NULL));
// 				mNextToPlay = rand() % mSamples.size();
// 				break;
// 		};
// 	}
// 
// 	void SoundGroup::resetClock()
// 	{
// 		gettimeofday(&start, NULL);
// 	}
// 
// 	float SoundGroup::getTime()
// 	{
// 		struct timeval end;
// 		gettimeofday(&end, NULL);
// 
// 		return (end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000;
// 	}

// 	void SoundGroup::update()
// 	{
// 		if (getTime() >= 1000.0f/mFrequency)
// 		{
// 			play();
// 			resetClock();
// 		}
// 	}
	
	void SoundGroup::addSound(const SoundDefinition& soundDef)
	{
		BaseSoundSample* soundSample = EmberServices::getSingleton().getSoundService().createOrRetrieveSoundSample(soundDef.getFilename());
		if (soundSample)
		{
			mSamples.push_back(soundSample);
		}
	}
	
	bool SoundGroup::bindToInstance(SoundInstance* instance)
	{
		SoundGroupBinding* binding = new SoundGroupBinding(instance->getSource(), *this);
		instance->bind(binding);
		return true;
	}
	
	
	
	const SoundGroup::SampleStore& SoundGroup::getSamples() const
	{
		return mSamples;
	}
	

}
}

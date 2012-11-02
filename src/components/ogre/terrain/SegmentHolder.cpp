/*
 Copyright (C) 2010 Erik Hjortsberg <erik.hjortsberg@gmail.com>

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

#include "SegmentHolder.h"
#include "SegmentReference.h"
#include "Segment.h"
#include "SegmentManager.h"

#include <cassert>

namespace Ember
{
namespace OgreView
{

namespace Terrain
{

SegmentHolder::SegmentHolder(Segment* segment, SegmentManager& segmentManager) :
	mSegment(segment), mSegmentManager(segmentManager), mRefCount(0)
{

}

SegmentHolder::~SegmentHolder()
{
	delete mSegment;
}

std::shared_ptr<SegmentReference> SegmentHolder::getReference()
{
	std::unique_lock<std::mutex> l(mRefCountMutex);
	mRefCount++;
	//If mRefCount is 1 we're guaranteed to be the only one interacting with the segment, so it's thread safe to call Mercator::Segment::isValid
	if (mRefCount == 1 && mSegment->getMercatorSegment().isValid()) {
		mSegmentManager.unmarkHolder(this);
	}
	return std::shared_ptr<SegmentReference>(new SegmentReference(*this));
}

void SegmentHolder::returnReference()
{
	std::unique_lock<std::mutex> l(mRefCountMutex);
	assert(mRefCount > 0);
	mRefCount--;
	//If mRefCount is 0 we're guaranteed to be the only one interacting with the segment, so it's thread safe to call Mercator::Segment::isValid
	if (mRefCount == 0 && mSegment->getMercatorSegment().isValid()) {
		mSegmentManager.markHolderAsDirtyAndUnused(this);
		l.unlock();
		mSegmentManager.pruneUnusedSegments();
	}
}

bool SegmentHolder::isUnused()
{
	std::unique_lock<std::mutex> l(mRefCountMutex);
	return mRefCount == 0;
}

Segment& SegmentHolder::getSegment()
{
	return *mSegment;
}

}

}
}

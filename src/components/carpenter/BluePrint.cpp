//
// C++ Implementation: BluePrint
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2005
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.//
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "BluePrint.h"
#include "Carpenter.h"

namespace Carpenter {

BluePrint::BluePrint(const std::string & name, Carpenter* carpenter)
: mName(name), mCarpenter(carpenter)
{
}


// BluePrint::~BluePrint()
// {
// }

BuildingBlock::BuildingBlock()
: mPosition(0,0,0), mAttached(false), mChildBindings(0)

{
mOrientation.identity();

}

void BuildingBlock::removeBoundPoint(const AttachPoint* point )
{
	std::vector<const AttachPoint*>::iterator pos = mBoundPoints.end();
	std::vector<const AttachPoint*>::iterator I = mBoundPoints.begin();
	std::vector<const AttachPoint*>::iterator I_end = mBoundPoints.end();
	for (; I!=I_end; ++I) {
		if ((*I) == point) {
			pos = I;
			break;
		}
	}
	if (pos != mBoundPoints.end()) {
		mBoundPoints.erase(pos);
	}

}



void BluePrint::doBindingsForBlock(BuildingBlock *block)
{
	std::map<BuildingBlock* , std::vector<BuildingBlockBinding*>> relatedBindings;

	std::list< BuildingBlockBinding>::iterator I = mBindings.begin();
	std::list< BuildingBlockBinding>::iterator I_end = mBindings.end();

	for (;I != I_end; ++I) {
		BuildingBlock* unboundBlock = 0;
		const AttachPair* pair = 0;
		BuildingBlock* block1 = &mBuildingBlocks.find((*I).mBlock1->getName())->second;
		BuildingBlock* block2 = &mBuildingBlocks.find((*I).mBlock2->getName())->second;
		if (block1 == block && !block2->isAttached()) {
			unboundBlock = block2;
			pair = (*I).mPoint2->getAttachPair();
		} else if (block2 == block && !block1->isAttached()) {
			unboundBlock = block1;
			pair = (*I).mPoint1->getAttachPair();
		}

		if (unboundBlock) {
			relatedBindings[unboundBlock].push_back(&(*I));
			if (relatedBindings[unboundBlock].size() > 1 && !unboundBlock->isAttached()) {
				placeBindings(unboundBlock, relatedBindings[unboundBlock]);
				doBindingsForBlock(unboundBlock);
			}
		}
	}
}

bool BluePrint::isRemovable(const BuildingBlock* bblock) const
{
	//cannot remove the starting block
	if (bblock == mStartingBlock) {
		return false;
	}

	//make sure the block exists in the blueprint
	if (mBuildingBlocks.find(bblock->getName()) == mBuildingBlocks.end()) {
		return false;
	}

	return bblock->mChildBindings == 0;
/*	//if the bblock has max two points attached to it it can be removed
	//HACK: here should be a better check, some kind of graph walking
	size_t size = bblock->mBoundPoints.size();
	return size < 3;*/
}

bool BluePrint::remove(const BuildingBlock* _bblock)
{
	if (!isRemovable(_bblock)) {
		return false;
	}
	BuildingBlock* bblock = &mBuildingBlocks.find(_bblock->getName())->second;

	std::list< BuildingBlockBinding>::iterator I = mBindings.begin();
	std::list< BuildingBlockBinding>::iterator I_end = mBindings.end();
	std::list< BuildingBlockBinding>::iterator I_remove = mBindings.end();

	for (; I != I_end; ++I) {
		if (I_remove != mBindings.end()) {
			mBindings.erase(I_remove);
			I_remove = mBindings.end();
		}

		if ((*I).getBlock1() == bblock) {
			BuildingBlock* boundBlock = &mBuildingBlocks.find((*I).mBlock2->getName())->second;
			boundBlock->removeBoundPoint((*I).getAttachPoint2());
			//decrease the number of child bindings for the parent block
			--(boundBlock->mChildBindings);
			I_remove = I;
		} else if ((*I).getBlock2() == bblock) {
			BuildingBlock* boundBlock = &mBuildingBlocks.find((*I).mBlock1->getName())->second;
			boundBlock->removeBoundPoint((*I).getAttachPoint1());
			--(boundBlock->mChildBindings);
			I_remove = I;
		}
	}
	if (I_remove != mBindings.end()) {
		mBindings.erase(I_remove);
		I_remove = mBindings.end();
	}

	//check if it's in the attached blocks vector and remove it
	std::vector<BuildingBlock*>::iterator pos = mAttachedBlocks.end();
	std::vector<BuildingBlock*>::iterator J = mAttachedBlocks.begin();
	std::vector<BuildingBlock*>::iterator J_end = mAttachedBlocks.end();
	for (; J!=J_end; ++J) {
		if ((*J) == bblock) {
			pos = J;
			break;
		}
	}
	if (pos != mAttachedBlocks.end()) {
		mAttachedBlocks.erase(pos);
	}


	mBuildingBlocks.erase(bblock->getName());
	return true;

}


const std::string& BuildingBlockBinding::getType() const
{
	return mPoint1->getAttachPair()->getType();
}


const AttachPair* BuildingBlock::getAttachPair(const std::string& name)
{
	const BlockSpec* spec = mBuildingBlockSpec->getBlockSpec();
	return spec->getAttachPair(name);

}

const BlockSpec* BuildingBlock::getBlockSpec() const
{
	return mBuildingBlockSpec->getBlockSpec();
}

void BluePrint::compile()
{

	mAttachedBlocks.clear();

// 	BuildingBlock* baseBlock = mStartingBlock;
	mStartingBlock->mAttached = true;
	mAttachedBlocks.push_back(mStartingBlock);
	doBindingsForBlock(mStartingBlock);

// 	std::vector< BuildingBlockBinding>::iterator I = mBindings.begin();
// 	std::vector< BuildingBlockBinding>::iterator I_end = mBindings.end();
//
// 	baseBlock = (*I).mBlock1;
// 	baseBlock->mPosition = WFMath::Point<3>(0,0,0);
// 	baseBlock->mRotation.identity();
//
// 	int bound = 0;
//
// 	placeBinding(&(*I));
// 	++bound;
//
// 	int boundBeforeIteration = bound;
// 	bool doContinue;
//
// 	while (doContinue) {
// 		for (;I != I_end; ++I) {
// 			if ((*I).mBlock1->isAttached()) {
// 				placeBinding(&(*I));
// 				++bound;
// 			}
//
// 		}
// 		if (bound > boundBeforeIteration) {
// 			doContinue = true;
// 		} else {
// 			doContinue = false;
// 		}
// 	}

	//now iterate over them all and create the SceneNodes
	//TODO: move this to another class



}

const std::vector< BuildingBlock*> BluePrint::getAttachedBlocks() const
{
	return mAttachedBlocks;
}

const std::list< BuildingBlockBinding>* BluePrint::getBindings() const
{
	return &mBindings;
}


// void BluePrint::deleteBuildingBlock(const std::string & name)
// {
// 	mBuildingBlocks.erase(name);
// }

BuildingBlock* BluePrint::createBuildingBlock(BuildingBlockDefinition definition)
{

	mBuildingBlocks[definition.mName];
	BuildingBlockSpec *buildingBlockSpec = mCarpenter->getBuildingBlockSpec(definition.mBuildingBlockSpec);
	assert(buildingBlockSpec);
	mBuildingBlocks[definition.mName].mBlockDefinition = definition;
	mBuildingBlocks[definition.mName].mBuildingBlockSpec = buildingBlockSpec;
	return &mBuildingBlocks[definition.mName];

}

void BluePrint::setStartingBlock(const std::string& name)
{
	mStartingBlock = &mBuildingBlocks.find(name)->second;
	mStartingBlock->mAttached = true;
}

WFMath::Point<3> BuildingBlock::getWorldPositionForPoint(const AttachPoint* point)
{
	WFMath::Vector<3> worldPoint = point->getPosition() - WFMath::Point<3>(0,0,0);
	worldPoint.rotate(mOrientation);
	return mPosition + worldPoint;
}


BuildingBlockBinding* BluePrint::addBinding(BuildingBlockBindingDefinition definition)
{
	//BuildingBlockBinding binding;
//	binding.mDefinition = definition;

	BuildingBlock* block1 = &mBuildingBlocks[definition.mBlock1Name];
	BuildingBlock* block2 = &mBuildingBlocks[definition.mBlock2Name];

	if (!block1 || !block2)
		return 0;

	const AttachPair *pair1 = block1->getAttachPair(definition.mPair1Name);
	const AttachPair *pair2 = block2->getAttachPair(definition.mPair2Name);
	if (!pair1 || !pair2) return 0;

	const AttachPoint* point1 = pair1->getAttachPoint(definition.mPoint1Name);
	const AttachPoint* point2 = pair2->getAttachPoint(definition.mPoint2Name);
	if (!point1 || !point2) return 0;

	return addBinding(block1, point1, block2, point2);
}

BuildingBlockBinding* BluePrint::addBinding(const BuildingBlock* block1, const AttachPoint* point1, const BuildingBlock* block2,	const AttachPoint* point2)
{
	BuildingBlockBinding binding(block1, point1, block2, point2);

	mBindings.push_back(binding);
	return &mBindings.back();

}

void BluePrint::placeBindings(BuildingBlock* unboundBlock, std::vector<BuildingBlockBinding*> bindings)
{

	//we place each unvound block through
	//1) first rotate it so the vector of the two unbound points matches that of the vector of the two bound points
	//2) then we rotate the unbound block along this vector so the normals of the attach points of the unbound block matches the inverse of the normals of the attach points of the bound block


	std::vector<BuildingBlockBinding*>::iterator I = bindings.begin();
	BuildingBlockBinding* binding1 = *I;
	BuildingBlockBinding* binding2 = *(++I);

	BuildingBlock * boundBlock, *boundBlock_2;

	const AttachPoint* boundPoint1;
	const AttachPoint* boundPoint2;
	const AttachPoint* unboundPoint1;
	const AttachPoint* unboundPoint2;

	//find out which block is unbound
	if (binding1->mBlock1->isAttached()) {
		boundBlock = &mBuildingBlocks.find(binding1->mBlock1->getName())->second;
		boundPoint1 = binding1->mPoint1;
		unboundPoint1 = binding1->mPoint2;
	} else {
		boundBlock = &mBuildingBlocks.find(binding1->mBlock2->getName())->second;
		boundPoint1 = binding1->mPoint2;
		unboundPoint1 = binding1->mPoint1;
	}

	//find out which block is unbound
	if (binding2->mBlock1->isAttached()) {
		boundBlock_2 = &mBuildingBlocks.find(binding2->mBlock1->getName())->second;
		boundPoint2 = binding2->mPoint1;
		unboundPoint2 = binding2->mPoint2;
	} else {
		boundBlock_2 = &mBuildingBlocks.find(binding2->mBlock2->getName())->second;
		boundPoint2 = binding2->mPoint2;
		unboundPoint2 = binding2->mPoint1;
	}

	WFMath::Vector<3> boundPointNormal = boundPoint1->getPosition() - boundPoint2->getPosition();
	boundPointNormal.rotate(boundBlock->getOrientation());
	boundPointNormal.normalize();
	WFMath::Vector<3> unboundPointNormal = unboundPoint1->getPosition() - unboundPoint2->getPosition();
	unboundPointNormal.normalize();

	//we need the quaternion needed to rotate unboundPointNormal (and thus the whole unboundBlock) to point in the same direction as boundPointNormal
	WFMath::Quaternion neededRotation;
	neededRotation.identity();
	try {
		neededRotation.rotation(unboundPointNormal, boundPointNormal);
	} catch (const WFMath::ColinearVectors<3> &) {
			//colinear eh? we need to flip the block

			//use one of the point normals for flipping
			WFMath::Vector<3> flipVector = unboundPoint1->getNormal();
			neededRotation = WFMath::Quaternion(flipVector, WFMath::Pi );

	}

	//do the first rotation
	unboundBlock->setOrientation(unboundBlock->getOrientation() * neededRotation);

	//now we must rotate around the unboundPointNormal so the normals of the point line up (i.e. the normal of an unbound point should be the inverse of a normal of a bound point)
	WFMath::Vector<3> worldNormalOfBoundPoint1 = boundPoint1->getNormal();
	worldNormalOfBoundPoint1.rotate(boundBlock->getOrientation());

	WFMath::Vector<3> worldNormalOfUnboundPoint1 = unboundPoint1->getNormal();
	worldNormalOfUnboundPoint1.rotate(unboundBlock->getOrientation());
	try {
		//rotate through the normals of the points
		neededRotation.rotation(-worldNormalOfBoundPoint1, worldNormalOfUnboundPoint1);
		//neededRotation.inverse();


	} catch (const WFMath::ColinearVectors<3> &)
	{
		//colinear eh? we need to flip the block

		//use one of the point normals for flipping
		WFMath::Vector<3> flipVector = unboundPoint1->getPosition() - unboundPoint2->getPosition();
		flipVector.rotate(unboundBlock->getOrientation());
		flipVector.normalize();
		neededRotation = WFMath::Quaternion(flipVector, WFMath::Pi );
	}
	//do the second rotation
	unboundBlock->setOrientation(unboundBlock->getOrientation() * neededRotation.inverse());


	unboundBlock->mAttached = true;
	mAttachedBlocks.push_back(unboundBlock);

	//we now have to position the block
	WFMath::Vector<3> distance = boundBlock->getWorldPositionForPoint(boundPoint1) - unboundBlock->getWorldPositionForPoint(unboundPoint1);
	unboundBlock->mPosition = unboundBlock->mPosition + distance;


	unboundBlock->mBoundPoints.push_back(unboundPoint1);
	unboundBlock->mBoundPoints.push_back(unboundPoint2);
	boundBlock->mBoundPoints.push_back(boundPoint1);
	boundBlock_2->mBoundPoints.push_back(boundPoint2);

	//increase the number of child bindings for the bound blocks (which should be the same block in most cases)
	++(boundBlock->mChildBindings);
	++(boundBlock_2->mChildBindings);




}

const std::vector< const AttachPoint * > BuildingBlock::getAllPoints( ) const
{
	return getBlockSpec()->getAllPoints();
}

BuildingBlockBinding::BuildingBlockBinding( const BuildingBlock * block1, const AttachPoint * point1, const BuildingBlock * block2, const AttachPoint * point2)
: mBlock1(block1), mPoint1(point1), mBlock2(block2), mPoint2(point2)
{
}

};




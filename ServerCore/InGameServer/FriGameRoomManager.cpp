#include "stdafx.h"
#include "FriGameRoomManager.h"


FriGameRoomManager::FriGameRoomManager()
{
}


FriGameRoomManager::~FriGameRoomManager()
{
}


BOOL FriGameRoomManager::Begin(DWORD maxRoomNum)
{
	if (maxRoomNum <= 0)
		return FALSE;

	mMaxRoomNum = maxRoomNum;

	for (DWORD i = 0; i<mMaxRoomNum; i++)
	{
		FriendshipGameRoom *room = new FriendshipGameRoom();

		if (room->Begin(i))

			mRoomVector.push_back(room);
		else
		{
			End();

			return FALSE;
		}
	}

	return TRUE;
}

BOOL FriGameRoomManager::End(VOID)
{
	ThreadSync sync;

	for (DWORD i = 0; i<mRoomVector.size(); i++)
	{
		FriendshipGameRoom *room = mRoomVector[i];

		room->End();

		delete room;
	}

	mRoomVector.clear();

	return TRUE;
}

FriendshipGameRoom* FriGameRoomManager::QuickJoin(UserInfo *userInfo, USHORT &slotNumber)
{
	ThreadSync sync;

	if (!userInfo)
		return NULL;

	for (DWORD i = 0; i<mRoomVector.size(); i++)
	{
		FriendshipGameRoom *room = mRoomVector[i];

		if (!room->GetIsFull() && !room->GetIsEmpty() && !room->GetIsReadyComplete())
		{
			if (!room->JoinUser(userInfo, slotNumber))
				continue;
			else
				return room;
		}
	}

	for (DWORD i = 0; i<mRoomVector.size(); i++)
	{
		FriendshipGameRoom *room = mRoomVector[i];

		if (room->GetIsEmpty())
		{
			if (!room->JoinUser(userInfo, slotNumber))
				continue;
			else
				return room;
		}
	}
	return NULL;
}
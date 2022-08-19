#pragma once

#include "CoreMinimal.h"

class UIcarusOcclusionInterface;
class AIcarusVolume;

class IcarusSystem
{
	friend class FIcarusModule;

public:

	static IcarusSystem& Get()
	{
		return *Instance;
	}

	//void SetOcclusionInterface(TSharedPtr<UIcarusOcclusionInterface> newOcclusionInterface) { OcclusionInterface = newOcclusionInterface; };

	void AddVolume(AIcarusVolume* ToAdd);
	void RemoveVolume(AIcarusVolume* ToRemove);
private:
	IcarusSystem();
	static IcarusSystem* Instance;



	TArray<AIcarusVolume*> Volumes;
	//TSharedPtr<UIcarusOcclusionInterface> OcclusionInterface;
};
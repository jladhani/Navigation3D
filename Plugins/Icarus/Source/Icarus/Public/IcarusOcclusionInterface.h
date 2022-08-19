#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class UIcarusOcclusionInterface : public TSharedFromThis<UIcarusOcclusionInterface>
{

public:
	virtual ~UIcarusOcclusionInterface() = 0;
	virtual bool IsOccluded(FVector Location, FVector Extends) = 0;
};

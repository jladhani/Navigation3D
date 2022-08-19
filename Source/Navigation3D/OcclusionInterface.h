#pragma once


#include "CoreMinimal.h"


class NAVIGATION3D_API OcclusionInterface : public TSharedFromThis<OcclusionInterface>
{
public:
	virtual ~OcclusionInterface() {};
	virtual double GetTotalOpenVolume() = 0;
	virtual float IntersectTet(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) = 0;
	virtual bool IntersectTetTest(const FVector& v1, const FVector& v2, const FVector& v3, const FVector& v4) = 0;

	virtual bool IsOccupied(const FBox& TestBounds) = 0;
	virtual FVector GetRandomLocation(FRandomStream& RandomStream) const = 0;
	virtual void Save(const FString& name)=0;
	virtual bool Load(const FString& name)=0;
	virtual void DrawDebug(UWorld* world) {};
};
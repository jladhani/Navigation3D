// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Path.h"
#include "SceneDrawer.generated.h"

class OcclusionInterface;
class ANav3DVolume;

UCLASS()
class NAVIGATION3D_API ASceneDrawer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASceneDrawer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable)
	void LoadMaze(const FString& Name);

	UFUNCTION(BlueprintCallable)
		void LoadAsteroid(const FString& Name);

	UFUNCTION(BlueprintCallable)
		void Clear();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		void DrawDebugPath(const FString& Name, bool IsSVO, bool IsMaze,int VoxelSetting);

	UFUNCTION(BlueprintCallable)
		void ClearDebugPath();

	UFUNCTION(BlueprintCallable)
		void SetNav3DVolume(ANav3DVolume* val) {Nav3DVolume = val;}

	UFUNCTION(BlueprintCallable)
		void LoadNav3DVolume(FString path);

	UFUNCTION(BlueprintCallable)
		void SetDrawNav3DVolume(bool draw);

private:
	TSharedPtr< OcclusionInterface> SceneToRender;
	Path DrawPath;
	bool bDrawPathValid = false;
	bool bDrawNav3D;
	TArray<FBoxCenterAndExtent> VoxelsToRender;
	UPROPERTY()
		ANav3DVolume* Nav3DVolume;
};

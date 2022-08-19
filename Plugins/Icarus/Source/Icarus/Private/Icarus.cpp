// Copyright Epic Games, Inc. All Rights Reserved.

#include "Icarus.h"
#include "IcarusSystem.h"
#define LOCTEXT_NAMESPACE "FIcarusModule"

void FIcarusModule::StartupModule()
{
	IcarusSystemPtr = new IcarusSystem();
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FIcarusModule::ShutdownModule()
{
	if (IcarusSystemPtr) delete IcarusSystemPtr;
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FIcarusModule, Icarus)
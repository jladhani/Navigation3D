#include <IcarusSystem.h>
#include <IcarusOcclusionInterface.h>
IcarusSystem* IcarusSystem::Instance = nullptr;

IcarusSystem::IcarusSystem()
{
	Instance = this;
}
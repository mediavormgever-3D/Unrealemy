#pragma once

#include "CoreMinimal.h"
#include "CurriculumTypes.h"

namespace CurriculumLoader
{
	/** Leest curriculum.json, alle tracks, hoofdstukken en UI-profielen onder RootDir. */
	FCurriculumData Load(const FString& RootDir);
}

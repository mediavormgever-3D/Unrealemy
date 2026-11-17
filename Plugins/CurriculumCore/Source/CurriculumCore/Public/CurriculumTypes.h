#pragma once

#include "CoreMinimal.h"

/** Waar de highlight naar wijst: een tab (ID) of een widget met een tag. */
struct FCurriculumHighlight
{
	FName Tab;
	FName Tag;

	bool IsSet() const { return !Tab.IsNone() || !Tag.IsNone(); }
};

/**
 * Wat de C++-kant van een stap nodig heeft. De validatie, setup en solve
 * leest Python zelf uit dezelfde JSON-bestanden.
 */
struct FCurriculumStep
{
	FString Id;
	FString Title;
	FString Instruction;
	TArray<FString> Hints;
	FString UiProfile;
	FCurriculumHighlight Highlight;
};

struct FCurriculumChapter
{
	FString Id;
	int32 Number = 0;
	FString Title;
	FString Summary;
	FString UiProfile;
	TArray<FCurriculumStep> Steps;
};

struct FCurriculumTrack
{
	FString Id;
	int32 Number = 0;
	FString Title;
	FString Summary;
	TArray<FCurriculumChapter> Chapters;
};

struct FCurriculumUiProfile
{
	FString Id;
	TArray<FName> HideTabs;
	TArray<FName> ShowTabs;
};

struct FCurriculumData
{
	TArray<FCurriculumTrack> Tracks;
	TMap<FString, FCurriculumUiProfile> UiProfiles;

	/** Leesfouten in de JSON; de hub toont ze, zodat een docent ze direct ziet. */
	TArray<FString> Errors;
};

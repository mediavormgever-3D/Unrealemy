#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SHorizontalBox;
class SVerticalBox;
class UCurriculumSubsystem;

/** Paneel naast de viewport: één micro-stap met status, hints en de knoppen Vorige, Reset en Volgende. */
class SCurriculumStepPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCurriculumStepPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCurriculumStepPanel() override;

private:
	void Rebuild();
	void RebuildHints();
	static UCurriculumSubsystem* GetSubsystem();

	TSharedRef<SWidget> MakeEmptyState();
	TSharedRef<SWidget> MakeActiveState();

	TSharedPtr<SHorizontalBox> Segments;
	TSharedPtr<SVerticalBox> Hints;
	FString ShownStepId;
	int32 HintsShown = 0;
	FDelegateHandle ChangedHandle;
};

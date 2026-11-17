#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SUniformGridPanel;
class SVerticalBox;
class UCurriculumSubsystem;
struct FCurriculumChapter;
struct FCurriculumTrack;

/** Curriculum Hub: alle leerlijnen als raster, daaronder de hoofdstukken van de gekozen leerlijn. */
class SCurriculumHub : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCurriculumHub) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCurriculumHub() override;

private:
	void Rebuild();
	TSharedRef<SWidget> MakeTrackCard(const FCurriculumTrack& Track);
	TSharedRef<SWidget> MakeChapterRow(const FCurriculumChapter& Chapter);
	static UCurriculumSubsystem* GetSubsystem();

	TSharedPtr<SVerticalBox> ErrorList;
	TSharedPtr<SUniformGridPanel> TrackGrid;
	TSharedPtr<SVerticalBox> ChapterList;
	FString SelectedTrackId;
	FDelegateHandle ChangedHandle;
};

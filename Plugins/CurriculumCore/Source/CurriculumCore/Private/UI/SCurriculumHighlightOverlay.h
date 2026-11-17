#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class SWindow;

/**
 * Doorzichtige laag over het hoofdvenster van de editor. Tekent een rode,
 * pulserende rand met het label HIER rond het doel van de actieve stap:
 * een tab (ID) of een widget met een tag. Klikken gaan er gewoon doorheen.
 */
class SCurriculumHighlightOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCurriculumHighlightOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InOwnerWindow);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Update(double InCurrentTime, float InDeltaTime);
	void ResolveTarget();

	TWeakPtr<SWindow> OwnerWindow;
	TWeakPtr<SWidget> Target;
	double LastResolveTime = -1.0;
};

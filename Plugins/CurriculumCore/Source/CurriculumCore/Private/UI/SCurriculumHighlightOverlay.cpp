#include "SCurriculumHighlightOverlay.h"

#include "CurriculumStyle.h"
#include "CurriculumSubsystem.h"
#include "CurriculumTabs.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SWindow.h"

namespace
{
	constexpr double ResolveInterval = 0.5;
	constexpr int32 MaxWidgetsToSearch = 50000;

	TSharedPtr<SWidget> FindWidgetWithTag(const TSharedRef<SWidget>& Root, FName Tag)
	{
		TArray<TSharedRef<SWidget>> Stack;
		Stack.Add(Root);
		int32 Visited = 0;
		while (Stack.Num() > 0 && Visited++ < MaxWidgetsToSearch)
		{
			const TSharedRef<SWidget> Widget = Stack.Pop(EAllowShrinking::No);
			if (const TSharedPtr<FTagMetaData> Meta = Widget->GetMetaData<FTagMetaData>())
			{
				if (Meta->Tag == Tag)
				{
					return Widget;
				}
			}
			if (FChildren* Children = Widget->GetChildren())
			{
				for (int32 Index = 0; Index < Children->Num(); ++Index)
				{
					Stack.Add(Children->GetChildAt(Index));
				}
			}
		}
		return nullptr;
	}
}

void SCurriculumHighlightOverlay::Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InOwnerWindow)
{
	OwnerWindow = InOwnerWindow;
	SetVisibility(EVisibility::HitTestInvisible);
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SCurriculumHighlightOverlay::Update));
}

FVector2D SCurriculumHighlightOverlay::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D::ZeroVector;
}

EActiveTimerReturnType SCurriculumHighlightOverlay::Update(double InCurrentTime, float InDeltaTime)
{
	if (InCurrentTime - LastResolveTime >= ResolveInterval)
	{
		LastResolveTime = InCurrentTime;
		ResolveTarget();
	}
	if (Target.IsValid())
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	return EActiveTimerReturnType::Continue;
}

void SCurriculumHighlightOverlay::ResolveTarget()
{
	Target.Reset();

	const UCurriculumSubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UCurriculumSubsystem>() : nullptr;
	const TSharedPtr<SWindow> Window = OwnerWindow.Pin();
	if (!Subsystem || !Window.IsValid())
	{
		return;
	}

	const FCurriculumHighlight Highlight = Subsystem->GetActiveHighlight();
	TSharedPtr<SWidget> Found;
	if (!Highlight.Tab.IsNone())
	{
		const TSharedPtr<SDockTab> Tab = CurriculumTabs::FindLiveTab(Highlight.Tab);
		if (Tab.IsValid() && Tab->IsForeground())
		{
			Found = Tab->GetContent();
		}
	}
	else if (!Highlight.Tag.IsNone())
	{
		Found = FindWidgetWithTag(Window.ToSharedRef(), Highlight.Tag);
	}

	// Alleen tekenen als het doel zichtbaar in dit venster staat (niet in een zwevend venster).
	if (Found.IsValid() && Found->GetVisibility().IsVisible()
		&& FSlateApplication::Get().FindWidgetWindow(Found.ToSharedRef()) == Window)
	{
		Target = Found;
	}
}

int32 SCurriculumHighlightOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const TSharedPtr<SWidget> Widget = Target.Pin();
	if (!Widget.IsValid())
	{
		return LayerId;
	}

	const FGeometry& TargetGeometry = Widget->GetPaintSpaceGeometry();
	const FVector2D AbsolutePosition = TargetGeometry.GetAbsolutePosition();
	const FVector2D AbsoluteSize = TargetGeometry.GetAbsoluteSize();
	if (AbsoluteSize.X < 4.0 || AbsoluteSize.Y < 4.0)
	{
		return LayerId;
	}

	const float Scale = AllottedGeometry.Scale > 0.f ? AllottedGeometry.Scale : 1.f;
	const FVector2D Inset(3.0, 3.0);
	const FVector2D LocalPosition = FVector2D(AllottedGeometry.AbsoluteToLocal(AbsolutePosition)) + Inset;
	const FVector2D LocalSize = AbsoluteSize / Scale - Inset * 2.0;

	const double Time = FSlateApplication::Get().GetCurrentTime();
	const float Pulse = 0.55f + 0.45f * FMath::Sin(static_cast<float>(Time) * 4.f);
	FLinearColor Tint = FLinearColor::White;
	Tint.A = Pulse;

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform(LocalPosition)),
		FCurriculumStyle::Brush("Unrealemy.Highlight"),
		ESlateDrawEffect::None,
		Tint);

	// Label "HIER" linksboven in het doel.
	const FVector2D LabelSize(54.0, 22.0);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(LocalPosition)),
		FCurriculumStyle::Brush("Unrealemy.White"),
		ESlateDrawEffect::None,
		FCurriculumStyle::Red());

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 3,
		AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(LocalPosition + FVector2D(9.0, 3.0))),
		FString(TEXT("HIER")),
		FCurriculumStyle::Label(9),
		ESlateDrawEffect::None,
		FLinearColor::White);

	return LayerId + 3;
}

#include "CurriculumStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedPtr<FSlateStyleSet> FCurriculumStyle::Instance;

FLinearColor FCurriculumStyle::Paper() { return FLinearColor::FromSRGBColor(FColor(0xF4, 0xF3, 0xEF)); }
FLinearColor FCurriculumStyle::Ink() { return FLinearColor::FromSRGBColor(FColor(0x11, 0x11, 0x11)); }
FLinearColor FCurriculumStyle::Red() { return FLinearColor::FromSRGBColor(FColor(0xE3, 0x06, 0x13)); }
FLinearColor FCurriculumStyle::Grey() { return FLinearColor::FromSRGBColor(FColor(0x8C, 0x8C, 0x88)); }
FLinearColor FCurriculumStyle::Rule() { return FLinearColor::FromSRGBColor(FColor(0xD2, 0xD1, 0xCB)); }

void FCurriculumStyle::Initialize()
{
	if (Instance.IsValid())
	{
		return;
	}

	Instance = MakeShared<FSlateStyleSet>(TEXT("UnrealemyStyle"));
	FSlateStyleSet& Style = *Instance;

	Style.Set("Unrealemy.Paper", new FSlateColorBrush(Paper()));
	Style.Set("Unrealemy.White", new FSlateColorBrush(FLinearColor::White));
	Style.Set("Unrealemy.Highlight", new FSlateRoundedBoxBrush(FLinearColor::Transparent, 2.f, Red(), 3.f));

	const FMargin Padding(0.f);
	const FMargin Pressed(0.f, 1.f, 0.f, 0.f);

	// Tegel in de hub: papier, bij hover zwart met witte tekst.
	Style.Set("Unrealemy.Card", FButtonStyle()
		.SetNormal(FSlateColorBrush(Paper()))
		.SetHovered(FSlateColorBrush(Ink()))
		.SetPressed(FSlateColorBrush(Red()))
		.SetDisabled(FSlateColorBrush(Paper()))
		.SetNormalForeground(Ink())
		.SetHoveredForeground(FLinearColor::White)
		.SetPressedForeground(FLinearColor::White)
		.SetDisabledForeground(Grey())
		.SetNormalPadding(Padding)
		.SetPressedPadding(Pressed));

	// Rij in de hoofdstukkenlijst: zelfde gedrag, lichtere hover.
	Style.Set("Unrealemy.Row", FButtonStyle()
		.SetNormal(FSlateColorBrush(Paper()))
		.SetHovered(FSlateColorBrush(FLinearColor::White))
		.SetPressed(FSlateColorBrush(Rule()))
		.SetDisabled(FSlateColorBrush(Paper()))
		.SetNormalForeground(Ink())
		.SetHoveredForeground(Ink())
		.SetPressedForeground(Ink())
		.SetDisabledForeground(Grey())
		.SetNormalPadding(Padding)
		.SetPressedPadding(Pressed));

	Style.Set("Unrealemy.Button.Primary", FButtonStyle()
		.SetNormal(FSlateColorBrush(Ink()))
		.SetHovered(FSlateColorBrush(Red()))
		.SetPressed(FSlateColorBrush(Red()))
		.SetDisabled(FSlateColorBrush(Rule()))
		.SetNormalForeground(FLinearColor::White)
		.SetHoveredForeground(FLinearColor::White)
		.SetPressedForeground(FLinearColor::White)
		.SetDisabledForeground(Grey())
		.SetNormalPadding(Padding)
		.SetPressedPadding(Pressed));

	Style.Set("Unrealemy.Button.Accent", FButtonStyle()
		.SetNormal(FSlateColorBrush(Red()))
		.SetHovered(FSlateColorBrush(Ink()))
		.SetPressed(FSlateColorBrush(Ink()))
		.SetDisabled(FSlateColorBrush(Rule()))
		.SetNormalForeground(FLinearColor::White)
		.SetHoveredForeground(FLinearColor::White)
		.SetPressedForeground(FLinearColor::White)
		.SetDisabledForeground(Grey())
		.SetNormalPadding(Padding)
		.SetPressedPadding(Pressed));

	Style.Set("Unrealemy.Button.Ghost", FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor::Transparent, 0.f, Ink(), 2.f))
		.SetHovered(FSlateColorBrush(Ink()))
		.SetPressed(FSlateColorBrush(Red()))
		.SetDisabled(FSlateRoundedBoxBrush(FLinearColor::Transparent, 0.f, Rule(), 2.f))
		.SetNormalForeground(Ink())
		.SetHoveredForeground(FLinearColor::White)
		.SetPressedForeground(FLinearColor::White)
		.SetDisabledForeground(Grey())
		.SetNormalPadding(Padding)
		.SetPressedPadding(Pressed));

	FSlateStyleRegistry::RegisterSlateStyle(Style);
}

void FCurriculumStyle::Shutdown()
{
	if (Instance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
		Instance.Reset();
	}
}

const ISlateStyle& FCurriculumStyle::Get()
{
	check(Instance.IsValid());
	return *Instance;
}

const FSlateBrush* FCurriculumStyle::Brush(FName Name)
{
	return Get().GetBrush(Name);
}

FSlateFontInfo FCurriculumStyle::Display(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
}

FSlateFontInfo FCurriculumStyle::Label(int32 Size)
{
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
	Font.LetterSpacing = 180;
	return Font;
}

FSlateFontInfo FCurriculumStyle::Strong(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
}

FSlateFontInfo FCurriculumStyle::Body(int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
}

namespace CurriculumUi
{
	TSharedRef<SWidget> MakeButton(const TAttribute<FText>& Label, EButton Kind, FOnClicked OnClicked, const TAttribute<bool>& IsEnabled)
	{
		const FName StyleName = Kind == EButton::Primary ? FName("Unrealemy.Button.Primary")
			: Kind == EButton::Accent ? FName("Unrealemy.Button.Accent")
			: FName("Unrealemy.Button.Ghost");

		return SNew(SButton)
			.ButtonStyle(&FCurriculumStyle::Get().GetWidgetStyle<FButtonStyle>(StyleName))
			.ContentPadding(FMargin(18.f, 10.f))
			.IsEnabled(IsEnabled)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCurriculumStyle::Label(10))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	TSharedRef<SWidget> MakeRule(const TAttribute<FSlateColor>& Color, float Thickness)
	{
		return SNew(SBox)
			.HeightOverride(Thickness)
			[
				SNew(SImage)
				.Image(FCurriculumStyle::Brush("Unrealemy.White"))
				.ColorAndOpacity(Color)
			];
	}

	TSharedRef<SWidget> MakeProgressBar(int32 Done, int32 Total)
	{
		const float Fraction = Total > 0 ? FMath::Clamp(float(Done) / float(Total), 0.f, 1.f) : 0.f;
		return SNew(SBox)
			.HeightOverride(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(FMath::Max(Fraction, 0.0001f))
				[
					SNew(SImage).Image(FCurriculumStyle::Brush("Unrealemy.White")).ColorAndOpacity(FCurriculumStyle::Red())
				]
				+ SHorizontalBox::Slot().FillWidth(FMath::Max(1.f - Fraction, 0.0001f))
				[
					SNew(SImage).Image(FCurriculumStyle::Brush("Unrealemy.White")).ColorAndOpacity(FCurriculumStyle::Rule())
				]
			];
	}

	FText Number(int32 Value)
	{
		return FText::AsCultureInvariant(FString::Printf(TEXT("%02d"), Value));
	}
}

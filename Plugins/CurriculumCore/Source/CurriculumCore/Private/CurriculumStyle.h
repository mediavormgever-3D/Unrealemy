#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/SlateDelegates.h"
#include "Misc/Attribute.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateStyle.h"

class SWidget;

/**
 * Swiss / International Typographic Style: papierwit, inktzwart en één
 * signaalkleur (rood), strak raster, grote cijfers, links uitgelijnd.
 */
class FCurriculumStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static const ISlateStyle& Get();

	static FLinearColor Paper();
	static FLinearColor Ink();
	static FLinearColor Red();
	static FLinearColor Grey();
	static FLinearColor Rule();

	static const FSlateBrush* Brush(FName Name);

	/** Grote koppen en cijfers. */
	static FSlateFontInfo Display(int32 Size);
	/** Kleine kapitalen-achtige labels met extra letterafstand. */
	static FSlateFontInfo Label(int32 Size);
	static FSlateFontInfo Strong(int32 Size);
	static FSlateFontInfo Body(int32 Size);

private:
	static TSharedPtr<FSlateStyleSet> Instance;
};

namespace CurriculumUi
{
	enum class EButton : uint8
	{
		Primary, // zwart vlak, witte tekst
		Ghost,   // omlijnd
		Accent   // rood vlak
	};

	TSharedRef<SWidget> MakeButton(const TAttribute<FText>& Label, EButton Kind, FOnClicked OnClicked,
		const TAttribute<bool>& IsEnabled = true);

	/** Horizontale lijn; het Swiss-raster leunt op deze lijnen in plaats van kaders. */
	TSharedRef<SWidget> MakeRule(const TAttribute<FSlateColor>& Color, float Thickness);

	TSharedRef<SWidget> MakeProgressBar(int32 Done, int32 Total);

	/** "01", "02", ... */
	FText Number(int32 Value);
}

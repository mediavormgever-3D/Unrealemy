#include "SCurriculumHub.h"

#include "CurriculumStyle.h"
#include "CurriculumSubsystem.h"
#include "CurriculumTabs.h"
#include "Editor.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UnrealemyHub"

namespace
{
	constexpr int32 GridColumns = 3;
}

UCurriculumSubsystem* SCurriculumHub::GetSubsystem()
{
	return GEditor ? GEditor->GetEditorSubsystem<UCurriculumSubsystem>() : nullptr;
}

void SCurriculumHub::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCurriculumStyle::Brush("Unrealemy.Paper"))
		.Padding(FMargin(56.f, 40.f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				// Kopregel: merknaam links, uitgang rechts.
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Brand", "UNREALEMY"))
						.Font(FCurriculumStyle::Label(11))
						.ColorAndOpacity(FCurriculumStyle::Ink())
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						CurriculumUi::MakeButton(LOCTEXT("FullEditor", "VOLLEDIGE EDITOR"), CurriculumUi::EButton::Ghost,
							FOnClicked::CreateLambda([]()
							{
								if (UCurriculumSubsystem* Subsystem = GetSubsystem())
								{
									Subsystem->LeaveCurriculum();
								}
								CurriculumTabs::CloseHub();
								return FReply::Handled();
							}))
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "Leerlijnen"))
					.Font(FCurriculumStyle::Display(60))
					.ColorAndOpacity(FCurriculumStyle::Ink())
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 24.f)
				[
					SNew(SBox)
					.MaxDesiredWidth(720.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Intro", "Kies een leerlijn en daarna een hoofdstuk. Elk hoofdstuk bestaat uit kleine stappen. De editor ziet zelf wanneer je een stap goed hebt gedaan."))
						.Font(FCurriculumStyle::Body(14))
						.ColorAndOpacity(FCurriculumStyle::Ink())
						.AutoWrapText(true)
					]
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					CurriculumUi::MakeRule(FCurriculumStyle::Ink(), 3.f)
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(ErrorList, SVerticalBox)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 32.f)
				[
					SAssignNew(TrackGrid, SUniformGridPanel)
					.SlotPadding(FMargin(0.f, 0.f, 24.f, 24.f))
					.MinDesiredSlotWidth(260.f)
				]

				+ SVerticalBox::Slot().AutoHeight()
				[
					CurriculumUi::MakeRule(FCurriculumStyle::Ink(), 3.f)
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)
				[
					SAssignNew(ChapterList, SVerticalBox)
				]
			]
		]
	];

	if (UCurriculumSubsystem* Subsystem = GetSubsystem())
	{
		ChangedHandle = Subsystem->OnChanged.AddSP(this, &SCurriculumHub::Rebuild);
		if (const FCurriculumTrack* Active = Subsystem->GetActiveTrack())
		{
			SelectedTrackId = Active->Id;
		}
	}
	Rebuild();
}

SCurriculumHub::~SCurriculumHub()
{
	if (UCurriculumSubsystem* Subsystem = GetSubsystem())
	{
		Subsystem->OnChanged.Remove(ChangedHandle);
	}
}

void SCurriculumHub::Rebuild()
{
	ErrorList->ClearChildren();
	TrackGrid->ClearChildren();
	ChapterList->ClearChildren();

	UCurriculumSubsystem* Subsystem = GetSubsystem();
	if (!Subsystem)
	{
		return;
	}
	const FCurriculumData& Data = Subsystem->GetData();

	for (const FString& Error : Data.Errors)
	{
		ErrorList->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Error))
			.Font(FCurriculumStyle::Strong(11))
			.ColorAndOpacity(FCurriculumStyle::Red())
			.AutoWrapText(true)
		];
	}

	if (!Subsystem->FindTrack(SelectedTrackId))
	{
		const FCurriculumTrack* First = Data.Tracks.FindByPredicate(
			[](const FCurriculumTrack& Track) { return Track.Chapters.Num() > 0; });
		SelectedTrackId = First ? First->Id : FString();
	}

	for (int32 Index = 0; Index < Data.Tracks.Num(); ++Index)
	{
		TrackGrid->AddSlot(Index % GridColumns, Index / GridColumns)
		[
			MakeTrackCard(Data.Tracks[Index])
		];
	}

	const FCurriculumTrack* Track = Subsystem->FindTrack(SelectedTrackId);
	if (!Track)
	{
		return;
	}

	ChapterList->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 20.f, 0.f)
		[
			SNew(STextBlock).Text(CurriculumUi::Number(Track->Number)).Font(FCurriculumStyle::Display(28)).ColorAndOpacity(FCurriculumStyle::Red())
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(STextBlock).Text(FText::FromString(Track->Title)).Font(FCurriculumStyle::Display(28)).ColorAndOpacity(FCurriculumStyle::Ink())
		]
	];

	for (const FCurriculumChapter& Chapter : Track->Chapters)
	{
		ChapterList->AddSlot().AutoHeight()
		[
			MakeChapterRow(Chapter)
		];
	}
}

TSharedRef<SWidget> SCurriculumHub::MakeTrackCard(const FCurriculumTrack& Track)
{
	UCurriculumSubsystem* Subsystem = GetSubsystem();
	const int32 Total = UCurriculumSubsystem::CountTotalSteps(Track);
	const int32 Done = Subsystem ? Subsystem->CountCompletedSteps(Track) : 0;
	const bool bAvailable = Track.Chapters.Num() > 0;
	const bool bSelected = Track.Id == SelectedTrackId;
	const FString TrackId = Track.Id;

	const FText Status = bAvailable
		? FText::Format(LOCTEXT("TrackProgress", "{0} / {1} stappen"), Done, Total)
		: LOCTEXT("TrackSoon", "In ontwikkeling");

	return SNew(SButton)
		.ButtonStyle(&FCurriculumStyle::Get().GetWidgetStyle<FButtonStyle>("Unrealemy.Card"))
		.ContentPadding(FMargin(0.f))
		.IsEnabled(bAvailable)
		.OnClicked_Lambda([this, TrackId]()
		{
			SelectedTrackId = TrackId;
			Rebuild();
			return FReply::Handled();
		})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				CurriculumUi::MakeRule(bSelected ? FCurriculumStyle::Red() : FCurriculumStyle::Ink(), bSelected ? 6.f : 3.f)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16.f, 14.f, 16.f, 0.f)
			[
				SNew(STextBlock)
				.Text(CurriculumUi::Number(Track.Number))
				.Font(FCurriculumStyle::Display(44))
				.ColorAndOpacity(bAvailable ? FCurriculumStyle::Red() : FCurriculumStyle::Grey())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16.f, 4.f, 16.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Track.Title))
				.Font(FCurriculumStyle::Strong(16))
				.ColorAndOpacity(FSlateColor::UseForeground())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16.f, 6.f, 16.f, 0.f)
			[
				SNew(SBox)
				.MinDesiredHeight(48.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Track.Summary))
					.Font(FCurriculumStyle::Body(11))
					.ColorAndOpacity(FSlateColor::UseForeground())
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16.f, 12.f, 16.f, 0.f)
			[
				CurriculumUi::MakeProgressBar(Done, Total)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16.f, 8.f, 16.f, 16.f)
			[
				SNew(STextBlock)
				.Text(Status)
				.Font(FCurriculumStyle::Label(9))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

TSharedRef<SWidget> SCurriculumHub::MakeChapterRow(const FCurriculumChapter& Chapter)
{
	UCurriculumSubsystem* Subsystem = GetSubsystem();
	const int32 Total = Chapter.Steps.Num();
	const int32 Done = Subsystem ? Subsystem->CountCompletedSteps(Chapter) : 0;
	const FString ChapterId = Chapter.Id;

	const FText Action = Done == 0 ? LOCTEXT("Start", "START")
		: Done < Total ? LOCTEXT("Continue", "GA VERDER")
		: LOCTEXT("Repeat", "HERHAAL");

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(&FCurriculumStyle::Get().GetWidgetStyle<FButtonStyle>("Unrealemy.Row"))
			.ContentPadding(FMargin(0.f, 16.f))
			.OnClicked_Lambda([ChapterId]()
			{
				if (UCurriculumSubsystem* Active = GetSubsystem())
				{
					Active->StartChapter(ChapterId);
				}
				CurriculumTabs::OpenStepPanel();
				CurriculumTabs::CloseHub();
				return FReply::Handled();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SBox)
					.WidthOverride(84.f)
					[
						SNew(STextBlock)
						.Text(CurriculumUi::Number(Chapter.Number))
						.Font(FCurriculumStyle::Display(30))
						.ColorAndOpacity(FCurriculumStyle::Red())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Chapter.Title))
						.Font(FCurriculumStyle::Strong(18))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Chapter.Summary))
						.Font(FCurriculumStyle::Body(12))
						.ColorAndOpacity(FSlateColor::UseForeground())
						.AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(24.f, 0.f, 8.f, 0.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ChapterProgress", "{0} / {1}"), Done, Total))
						.Font(FCurriculumStyle::Strong(14))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(Action)
						.Font(FCurriculumStyle::Label(10))
						.ColorAndOpacity(FCurriculumStyle::Red())
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CurriculumUi::MakeRule(FCurriculumStyle::Rule(), 1.f)
		];
}

#undef LOCTEXT_NAMESPACE

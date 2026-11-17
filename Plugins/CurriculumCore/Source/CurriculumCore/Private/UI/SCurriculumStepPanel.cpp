#include "SCurriculumStepPanel.h"

#include "CurriculumStyle.h"
#include "CurriculumSubsystem.h"
#include "CurriculumTabs.h"
#include "Editor.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "UnrealemyStepPanel"

UCurriculumSubsystem* SCurriculumStepPanel::GetSubsystem()
{
	return GEditor ? GEditor->GetEditorSubsystem<UCurriculumSubsystem>() : nullptr;
}

void SCurriculumStepPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCurriculumStyle::Brush("Unrealemy.Paper"))
		.Padding(FMargin(28.f, 24.f))
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([]()
			{
				const UCurriculumSubsystem* Subsystem = GetSubsystem();
				return Subsystem && Subsystem->HasActiveStep() ? 1 : 0;
			})
			+ SWidgetSwitcher::Slot()[ MakeEmptyState() ]
			+ SWidgetSwitcher::Slot()[ MakeActiveState() ]
		]
	];

	if (UCurriculumSubsystem* Subsystem = GetSubsystem())
	{
		ChangedHandle = Subsystem->OnChanged.AddSP(this, &SCurriculumStepPanel::Rebuild);
	}
	Rebuild();
}

SCurriculumStepPanel::~SCurriculumStepPanel()
{
	if (UCurriculumSubsystem* Subsystem = GetSubsystem())
	{
		Subsystem->OnChanged.Remove(ChangedHandle);
	}
}

TSharedRef<SWidget> SCurriculumStepPanel::MakeEmptyState()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EmptyTitle", "Geen hoofdstuk open"))
			.Font(FCurriculumStyle::Display(22))
			.ColorAndOpacity(FCurriculumStyle::Ink())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 20.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EmptyBody", "Kies in het overzicht een leerlijn en een hoofdstuk."))
			.Font(FCurriculumStyle::Body(13))
			.ColorAndOpacity(FCurriculumStyle::Ink())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
		[
			CurriculumUi::MakeButton(LOCTEXT("OpenHub", "NAAR HET OVERZICHT"), CurriculumUi::EButton::Primary,
				FOnClicked::CreateLambda([]() { CurriculumTabs::OpenHub(); return FReply::Handled(); }))
		];
}

TSharedRef<SWidget> SCurriculumStepPanel::MakeActiveState()
{
	auto Subsystem = []() { return GetSubsystem(); };

	return SNew(SVerticalBox)

		// Kruimelpad + inhoudsopgave
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FCurriculumStyle::Label(9))
				.ColorAndOpacity(FCurriculumStyle::Ink())
				.AutoWrapText(true)
				.Text_Lambda([Subsystem]()
				{
					const UCurriculumSubsystem* S = Subsystem();
					const FCurriculumTrack* Track = S ? S->GetActiveTrack() : nullptr;
					const FCurriculumChapter* Chapter = S ? S->GetActiveChapter() : nullptr;
					if (!Track || !Chapter)
					{
						return FText::GetEmpty();
					}
					return FText::FromString(FString::Printf(TEXT("%02d %s  /  %02d %s"),
						Track->Number, *Track->Title.ToUpper(), Chapter->Number, *Chapter->Title.ToUpper()));
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)
			[
				CurriculumUi::MakeButton(LOCTEXT("Contents", "INHOUD"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([]() { CurriculumTabs::OpenHub(); return FReply::Handled(); }))
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
		[
			CurriculumUi::MakeRule(FCurriculumStyle::Ink(), 3.f)
		]

		// Stap x / y
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Font(FCurriculumStyle::Label(10))
			.ColorAndOpacity(FCurriculumStyle::Red())
			.Text_Lambda([Subsystem]()
			{
				const UCurriculumSubsystem* S = Subsystem();
				const FCurriculumChapter* Chapter = S ? S->GetActiveChapter() : nullptr;
				return Chapter
					? FText::Format(LOCTEXT("StepOf", "STAP {0} / {1}"), S->GetActiveStepIndex() + 1, Chapter->Steps.Num())
					: FText::GetEmpty();
			})
		]

		// Voortgangsblokjes: zwart = gedaan, rood = nu, grijs = nog te doen
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(6.f)
			[
				SAssignNew(Segments, SHorizontalBox)
			]
		]

		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 20.f, 0.f, 0.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Font(FCurriculumStyle::Display(24))
					.ColorAndOpacity(FCurriculumStyle::Ink())
					.AutoWrapText(true)
					.Text_Lambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						const FCurriculumStep* Step = S ? S->GetActiveStep() : nullptr;
						return Step ? FText::FromString(Step->Title) : FText::GetEmpty();
					})
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(FCurriculumStyle::Body(15))
					.ColorAndOpacity(FCurriculumStyle::Ink())
					.AutoWrapText(true)
					.Text_Lambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						const FCurriculumStep* Step = S ? S->GetActiveStep() : nullptr;
						return Step ? FText::FromString(Step->Instruction) : FText::GetEmpty();
					})
				]

				// Status: rood blok zolang de stap loopt, zwart blok als hij voltooid is.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f)
				[
					SNew(SBorder)
					.BorderImage(FCurriculumStyle::Brush("Unrealemy.White"))
					.BorderBackgroundColor_Lambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						return FSlateColor(S && S->IsActiveStepPassed() ? FCurriculumStyle::Ink() : FCurriculumStyle::Rule());
					})
					.Padding(FMargin(14.f, 10.f))
					[
						SNew(STextBlock)
						.Font(FCurriculumStyle::Label(10))
						.ColorAndOpacity_Lambda([Subsystem]()
						{
							const UCurriculumSubsystem* S = Subsystem();
							return FSlateColor(S && S->IsActiveStepPassed() ? FLinearColor::White : FCurriculumStyle::Ink());
						})
						.Text_Lambda([Subsystem]()
						{
							const UCurriculumSubsystem* S = Subsystem();
							if (S && S->IsActiveChapterFinished())
							{
								return LOCTEXT("ChapterDone", "HOOFDSTUK VOLTOOID");
							}
							return S && S->IsActiveStepPassed()
								? LOCTEXT("Passed", "VOLTOOID")
								: LOCTEXT("Waiting", "BEZIG: VOER DE OPDRACHT UIT IN DE EDITOR");
						})
					]
				]

				// Feedback van de validator
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(FCurriculumStyle::Body(12))
					.ColorAndOpacity(FCurriculumStyle::Grey())
					.AutoWrapText(true)
					.Text_Lambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						return S ? FText::FromString(S->GetLastMessage()) : FText::GetEmpty();
					})
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
				[
					SAssignNew(Hints, SVerticalBox)
				]
			]
		]

		// Hoofdstuk klaar
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([Subsystem]()
			{
				const UCurriculumSubsystem* S = Subsystem();
				return S && S->IsActiveChapterFinished() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				CurriculumUi::MakeButton(LOCTEXT("Overview", "OVERZICHT"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([]() { CurriculumTabs::OpenHub(); return FReply::Handled(); }))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				CurriculumUi::MakeButton(LOCTEXT("NextChapter", "VOLGEND HOOFDSTUK"), CurriculumUi::EButton::Accent,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem())
						{
							if (const FCurriculumChapter* Next = S->GetNextChapter())
							{
								S->StartChapter(Next->Id);
							}
						}
						return FReply::Handled();
					}),
					TAttribute<bool>::CreateLambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						return S && S->GetNextChapter() != nullptr;
					}))
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 12.f)
		[
			CurriculumUi::MakeRule(FCurriculumStyle::Rule(), 1.f)
		]

		// Navigatie
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				CurriculumUi::MakeButton(LOCTEXT("Previous", "VORIGE"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem()) { S->PreviousStep(); }
						return FReply::Handled();
					}),
					TAttribute<bool>::CreateLambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						return S && S->CanGoPrevious();
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).Padding(8.f, 0.f)
			[
				CurriculumUi::MakeButton(LOCTEXT("Reset", "RESET STAP"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem()) { S->ResetActiveStep(); }
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				CurriculumUi::MakeButton(LOCTEXT("Next", "VOLGENDE"), CurriculumUi::EButton::Primary,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem()) { S->NextStep(); }
						return FReply::Handled();
					}),
					TAttribute<bool>::CreateLambda([Subsystem]()
					{
						const UCurriculumSubsystem* S = Subsystem();
						return S && S->CanGoNext();
					}))
			]
		]

		// Docentmodus
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([Subsystem]()
			{
				const UCurriculumSubsystem* S = Subsystem();
				return S && S->IsTeacherMode() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				CurriculumUi::MakeButton(LOCTEXT("Solve", "DOCENT: OPLOSSING"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem()) { S->SolveActiveStep(); }
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				CurriculumUi::MakeButton(LOCTEXT("Reload", "DOCENT: HERLAAD"), CurriculumUi::EButton::Ghost,
					FOnClicked::CreateLambda([Subsystem]()
					{
						if (UCurriculumSubsystem* S = Subsystem()) { S->ReloadCurriculum(); }
						return FReply::Handled();
					}))
			]
		];
}

void SCurriculumStepPanel::Rebuild()
{
	Segments->ClearChildren();

	const UCurriculumSubsystem* Subsystem = GetSubsystem();
	const FCurriculumChapter* Chapter = Subsystem ? Subsystem->GetActiveChapter() : nullptr;
	if (!Chapter)
	{
		ShownStepId.Reset();
		Hints->ClearChildren();
		return;
	}

	for (int32 Index = 0; Index < Chapter->Steps.Num(); ++Index)
	{
		const FString& StepId = Chapter->Steps[Index].Id;
		const bool bCurrent = Index == Subsystem->GetActiveStepIndex();
		const bool bDone = Subsystem->IsStepPassedThisSession(StepId) || (bCurrent && Subsystem->IsActiveStepPassed());
		const FLinearColor Color = bCurrent && !bDone ? FCurriculumStyle::Red()
			: bDone ? FCurriculumStyle::Ink()
			: FCurriculumStyle::Rule();

		Segments->AddSlot().FillWidth(1.f).Padding(0.f, 0.f, Index + 1 < Chapter->Steps.Num() ? 3.f : 0.f, 0.f)
		[
			SNew(SImage).Image(FCurriculumStyle::Brush("Unrealemy.White")).ColorAndOpacity(Color)
		];
	}

	const FString ActiveId = Subsystem->GetActiveStepId();
	if (ActiveId != ShownStepId)
	{
		ShownStepId = ActiveId;
		HintsShown = 0;
	}
	RebuildHints();
}

void SCurriculumStepPanel::RebuildHints()
{
	Hints->ClearChildren();

	const UCurriculumSubsystem* Subsystem = GetSubsystem();
	const FCurriculumStep* Step = Subsystem ? Subsystem->GetActiveStep() : nullptr;
	if (!Step)
	{
		return;
	}

	for (int32 Index = 0; Index < FMath::Min(HintsShown, Step->Hints.Num()); ++Index)
	{
		Hints->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("HintMark", "HINT")).Font(FCurriculumStyle::Label(9)).ColorAndOpacity(FCurriculumStyle::Red())
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(FText::FromString(Step->Hints[Index])).Font(FCurriculumStyle::Body(13))
				.ColorAndOpacity(FCurriculumStyle::Ink()).AutoWrapText(true)
			]
		];
	}

	if (HintsShown < Step->Hints.Num() && !Subsystem->IsActiveStepPassed())
	{
		Hints->AddSlot().AutoHeight().HAlign(HAlign_Left)
		[
			CurriculumUi::MakeButton(
				FText::Format(LOCTEXT("ShowHint", "TOON HINT ({0}/{1})"), HintsShown + 1, Step->Hints.Num()),
				CurriculumUi::EButton::Ghost,
				FOnClicked::CreateLambda([this]()
				{
					++HintsShown;
					RebuildHints();
					return FReply::Handled();
				}))
		];
	}
}

#undef LOCTEXT_NAMESPACE

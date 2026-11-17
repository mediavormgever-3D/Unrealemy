#include "CurriculumSubsystem.h"

#include "CurriculumLoader.h"
#include "CurriculumLog.h"
#include "CurriculumTabs.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "IPythonScriptPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* SettingsSection = TEXT("Unrealemy");
	const TCHAR* SuccessSound = TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue");

	/** Standaardindeling als de student de leerlijn verlaat. */
	const FName DefaultTabs[] = {
		FName(TEXT("LevelEditorSceneOutliner")),
		FName(TEXT("LevelEditorSelectionDetails")),
	};

	FString GetCurriculumDir()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CurriculumCore"));
		return Plugin.IsValid() ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Curriculum")) : FString();
	}

	/** Stap-id's zijn door het schema al beperkt tot [a-z0-9.]; dit is een tweede vangnet. */
	FString PyString(const FString& Value)
	{
		FString Safe;
		for (const TCHAR Char : Value)
		{
			if (FChar::IsAlnum(Char) || Char == TEXT('.') || Char == TEXT('_') || Char == TEXT('-'))
			{
				Safe.AppendChar(Char);
			}
		}
		return FString::Printf(TEXT("\"%s\""), *Safe);
	}
}

void UCurriculumSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ReadSettings();
	LoadCurriculumFiles();
	LoadProgress();

	if (!IsRunningCommandlet())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UCurriculumSubsystem::Tick), CheckInterval);
	}
}

void UCurriculumSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

void UCurriculumSubsystem::ReadSettings()
{
	if (GConfig)
	{
		GConfig->GetBool(SettingsSection, TEXT("bTeacherMode"), bTeacherMode, GEditorIni);
		GConfig->GetBool(SettingsSection, TEXT("bAllowSkip"), bAllowSkip, GEditorIni);
		GConfig->GetBool(SettingsSection, TEXT("bEnforceUiProfiles"), bEnforceUiProfiles, GEditorIni);
		GConfig->GetBool(SettingsSection, TEXT("bPlaySounds"), bPlaySounds, GEditorIni);
		GConfig->GetFloat(SettingsSection, TEXT("AutoAdvanceDelay"), AutoAdvanceDelay, GEditorIni);
		GConfig->GetFloat(SettingsSection, TEXT("CheckInterval"), CheckInterval, GEditorIni);
	}
	CheckInterval = FMath::Clamp(CheckInterval, 0.1f, 2.0f);

	// Docenten zetten op hun eigen Windows-account UNREALEMY_TEACHER=1.
	if (FPlatformMisc::GetEnvironmentVariable(TEXT("UNREALEMY_TEACHER")) == TEXT("1"))
	{
		bTeacherMode = true;
	}
}

void UCurriculumSubsystem::LoadCurriculumFiles()
{
	const FString Dir = GetCurriculumDir();
	Data = Dir.IsEmpty() ? FCurriculumData() : CurriculumLoader::Load(Dir);
	if (Dir.IsEmpty())
	{
		Data.Errors.Add(TEXT("Plugin CurriculumCore niet gevonden."));
	}
	for (const FString& Error : Data.Errors)
	{
		UE_LOG(LogCurriculum, Error, TEXT("%s"), *Error);
	}
	UE_LOG(LogCurriculum, Log, TEXT("%d leerlijnen geladen uit %s"), Data.Tracks.Num(), *Dir);
}

// --- Opzoeken ------------------------------------------------------------------------

const FCurriculumTrack* UCurriculumSubsystem::FindTrack(const FString& TrackId) const
{
	return Data.Tracks.FindByPredicate([&TrackId](const FCurriculumTrack& Track) { return Track.Id == TrackId; });
}

const FCurriculumChapter* UCurriculumSubsystem::FindChapter(const FString& ChapterId, const FCurriculumTrack** OutTrack) const
{
	for (const FCurriculumTrack& Track : Data.Tracks)
	{
		for (const FCurriculumChapter& Chapter : Track.Chapters)
		{
			if (Chapter.Id == ChapterId)
			{
				if (OutTrack)
				{
					*OutTrack = &Track;
				}
				return &Chapter;
			}
		}
	}
	return nullptr;
}

int32 UCurriculumSubsystem::CountCompletedSteps(const FCurriculumChapter& Chapter) const
{
	int32 Count = 0;
	for (const FCurriculumStep& Step : Chapter.Steps)
	{
		Count += Completed.Contains(Step.Id) ? 1 : 0;
	}
	return Count;
}

int32 UCurriculumSubsystem::CountCompletedSteps(const FCurriculumTrack& Track) const
{
	int32 Count = 0;
	for (const FCurriculumChapter& Chapter : Track.Chapters)
	{
		Count += CountCompletedSteps(Chapter);
	}
	return Count;
}

int32 UCurriculumSubsystem::CountTotalSteps(const FCurriculumTrack& Track)
{
	int32 Count = 0;
	for (const FCurriculumChapter& Chapter : Track.Chapters)
	{
		Count += Chapter.Steps.Num();
	}
	return Count;
}

const FCurriculumTrack* UCurriculumSubsystem::GetActiveTrack() const
{
	const FCurriculumTrack* Track = nullptr;
	FindChapter(ActiveChapterId, &Track);
	return Track;
}

const FCurriculumChapter* UCurriculumSubsystem::GetActiveChapter() const
{
	return ActiveChapterId.IsEmpty() ? nullptr : FindChapter(ActiveChapterId);
}

const FCurriculumStep* UCurriculumSubsystem::GetActiveStep() const
{
	const FCurriculumChapter* Chapter = GetActiveChapter();
	return Chapter && Chapter->Steps.IsValidIndex(ActiveStepIndex) ? &Chapter->Steps[ActiveStepIndex] : nullptr;
}

FString UCurriculumSubsystem::GetActiveStepId() const
{
	const FCurriculumStep* Step = GetActiveStep();
	return Step ? Step->Id : FString();
}

bool UCurriculumSubsystem::IsActiveChapterFinished() const
{
	const FCurriculumChapter* Chapter = GetActiveChapter();
	return Chapter && bActivePassed && ActiveStepIndex == Chapter->Steps.Num() - 1;
}

const FCurriculumChapter* UCurriculumSubsystem::GetNextChapter() const
{
	const FCurriculumTrack* Track = GetActiveTrack();
	if (!Track)
	{
		return nullptr;
	}
	for (int32 Index = 0; Index + 1 < Track->Chapters.Num(); ++Index)
	{
		if (Track->Chapters[Index].Id == ActiveChapterId)
		{
			return &Track->Chapters[Index + 1];
		}
	}
	return nullptr;
}

FCurriculumHighlight UCurriculumSubsystem::GetActiveHighlight() const
{
	const FCurriculumStep* Step = GetActiveStep();
	return Step && !bActivePassed ? Step->Highlight : FCurriculumHighlight();
}

bool UCurriculumSubsystem::CanGoNext() const
{
	const FCurriculumChapter* Chapter = GetActiveChapter();
	if (!Chapter || ActiveStepIndex + 1 >= Chapter->Steps.Num())
	{
		return false;
	}
	return bActivePassed || IsStepCompleted(GetActiveStepId()) || bAllowSkip || bTeacherMode;
}

// --- Navigatie -----------------------------------------------------------------------

void UCurriculumSubsystem::StartChapter(const FString& ChapterId)
{
	const FCurriculumChapter* Chapter = FindChapter(ChapterId);
	if (!Chapter || Chapter->Steps.Num() == 0)
	{
		return;
	}

	// Terug naar een hoofdstuk dat al open staat: de scène van de student blijft staan.
	if (ActiveChapterId == ChapterId && HasActiveStep())
	{
		OnChanged.Broadcast();
		return;
	}

	int32 FirstOpen = 0;
	while (FirstOpen < Chapter->Steps.Num() && Completed.Contains(Chapter->Steps[FirstOpen].Id))
	{
		++FirstOpen;
	}
	if (FirstOpen == Chapter->Steps.Num())
	{
		FirstOpen = 0; // Alles al af: opnieuw vanaf het begin.
	}

	ActiveChapterId = ChapterId;
	EnterStep(FirstOpen, true);
}

void UCurriculumSubsystem::NextStep()
{
	if (CanGoNext())
	{
		// Overslaan zonder de stap te halen: eerst de beginstaat van de volgende stap opbouwen.
		EnterStep(ActiveStepIndex + 1, !bActivePassed);
	}
}

void UCurriculumSubsystem::PreviousStep()
{
	if (CanGoPrevious())
	{
		// Terugbladeren verandert de scène niet; daarvoor is 'Reset stap'.
		EnterStep(ActiveStepIndex - 1, false);
	}
}

void UCurriculumSubsystem::ResetActiveStep()
{
	if (HasActiveStep())
	{
		EnterStep(ActiveStepIndex, true);
	}
}

void UCurriculumSubsystem::SolveActiveStep()
{
	if (bTeacherMode && HasActiveStep() && !bActivePassed)
	{
		bBusy = true;
		RunPython(FString::Printf(TEXT("import unrealemy.runner as r\nr.solve(%s)\n"), *PyString(GetActiveStepId())));
		bBusy = false;
	}
}

void UCurriculumSubsystem::LeaveCurriculum()
{
	ActiveChapterId.Reset();
	ActiveStepIndex = INDEX_NONE;
	bActivePassed = false;
	SessionPassed.Reset();
	LastMessage.Reset();
	AutoAdvanceAt = 0.0;
	for (const FName& TabId : DefaultTabs)
	{
		CurriculumTabs::OpenTab(TabId, false);
	}
	OnChanged.Broadcast();
}

void UCurriculumSubsystem::ReloadCurriculum()
{
	LoadCurriculumFiles();
	RunPython(TEXT("import unrealemy.runner as r\nr.reload()\n"));
	if (!HasActiveStep())
	{
		ActiveChapterId.Reset();
		ActiveStepIndex = INDEX_NONE;
	}
	OnChanged.Broadcast();
}

void UCurriculumSubsystem::EnterStep(int32 StepIndex, bool bResetScene)
{
	const FCurriculumChapter* Chapter = GetActiveChapter();
	if (!Chapter || !Chapter->Steps.IsValidIndex(StepIndex))
	{
		return;
	}

	ActiveStepIndex = StepIndex;
	const FString StepId = Chapter->Steps[StepIndex].Id;

	if (bResetScene)
	{
		// Na een reset zijn precies de eerdere stappen van dit hoofdstuk uitgevoerd.
		SessionPassed.Reset();
		for (int32 Index = 0; Index < StepIndex; ++Index)
		{
			SessionPassed.Add(Chapter->Steps[Index].Id);
		}
	}
	bActivePassed = SessionPassed.Contains(StepId);
	LastMessage.Reset();
	AutoAdvanceAt = 0.0;

	ApplyUiProfile(false);

	bBusy = true;
	const bool bPrepared = RunPython(FString::Printf(
		TEXT("import unrealemy.runner as r\nr.begin_step(%s, reset=%s)\n"),
		*PyString(StepId), bResetScene ? TEXT("True") : TEXT("False")));
	bBusy = false;
	if (!bPrepared && LastMessage.IsEmpty())
	{
		LastMessage = TEXT("Deze stap kon niet worden voorbereid. Vraag je docent om in de Output Log (LogPython) te kijken.");
	}

	const FCurriculumHighlight Highlight = GetActiveHighlight();
	if (!Highlight.Tab.IsNone())
	{
		CurriculumTabs::OpenTab(Highlight.Tab, true);
	}

	OnChanged.Broadcast();
}

// --- Controle via Python -------------------------------------------------------------

bool UCurriculumSubsystem::Tick(float DeltaTime)
{
	if (bBusy || !HasActiveStep())
	{
		return true;
	}

	if (bEnforceUiProfiles)
	{
		ApplyUiProfile(true);
	}

	if (bActivePassed)
	{
		if (AutoAdvanceAt > 0.0 && FPlatformTime::Seconds() >= AutoAdvanceAt)
		{
			AutoAdvanceAt = 0.0;
			NextStep();
		}
		return true;
	}

	bBusy = true;
	RunPython(FString::Printf(TEXT("import unrealemy.runner as r\nr.check(%s)\n"), *PyString(GetActiveStepId())));
	bBusy = false;
	return true;
}

void UCurriculumSubsystem::ReportCheckResult(const FString& StepId, bool bPassed, const FString& Message)
{
	if (StepId != GetActiveStepId() || bActivePassed)
	{
		return;
	}

	if (!bPassed)
	{
		if (Message != LastMessage)
		{
			LastMessage = Message;
			OnChanged.Broadcast();
		}
		return;
	}

	bActivePassed = true;
	LastMessage = Message;
	SessionPassed.Add(StepId);
	Completed.Add(StepId);
	SaveProgress();

	const FCurriculumChapter* Chapter = GetActiveChapter();
	const bool bLastStep = Chapter && ActiveStepIndex == Chapter->Steps.Num() - 1;
	AutoAdvanceAt = (!bLastStep && AutoAdvanceDelay > 0.f) ? FPlatformTime::Seconds() + AutoAdvanceDelay : 0.0;

	if (bPlaySounds && GEditor)
	{
		GEditor->PlayEditorSound(SuccessSound);
	}
	OnChanged.Broadcast();
}

bool UCurriculumSubsystem::RunPython(const FString& Code)
{
	IPythonScriptPlugin* Python = IPythonScriptPlugin::Get();
	if (!Python || !Python->IsPythonAvailable())
	{
		LastMessage = TEXT("Python staat uit. Zet de plugin 'Python Editor Script Plugin' aan.");
		return false;
	}

	FPythonCommandEx Command;
	Command.Command = Code;
	Command.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
	return Python->ExecPythonCommandEx(Command);
}

// --- UI-profielen --------------------------------------------------------------------

const FCurriculumUiProfile* UCurriculumSubsystem::GetActiveUiProfile() const
{
	const FCurriculumStep* Step = GetActiveStep();
	const FCurriculumChapter* Chapter = GetActiveChapter();
	if (!Step || !Chapter)
	{
		return nullptr;
	}
	const FString& ProfileId = Step->UiProfile.IsEmpty() ? Chapter->UiProfile : Step->UiProfile;
	return ProfileId.IsEmpty() ? nullptr : Data.UiProfiles.Find(ProfileId);
}

void UCurriculumSubsystem::ApplyUiProfile(bool bEnforceOnly) const
{
	const FCurriculumUiProfile* Profile = GetActiveUiProfile();
	if (!Profile)
	{
		return;
	}
	for (const FName& TabId : Profile->HideTabs)
	{
		CurriculumTabs::CloseTab(TabId);
	}
	if (!bEnforceOnly)
	{
		for (const FName& TabId : Profile->ShowTabs)
		{
			CurriculumTabs::OpenTab(TabId, false);
		}
	}
}

// --- Voortgang -----------------------------------------------------------------------

FString UCurriculumSubsystem::GetProgressPath()
{
	// %LOCALAPPDATA%\Unrealemy\progress.json: per Windows-gebruiker, ook op gedeelde school-pc's.
	return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("Unrealemy"), TEXT("progress.json"));
}

void UCurriculumSubsystem::LoadProgress()
{
	Completed.Reset();

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *GetProgressPath()))
	{
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogCurriculum, Warning, TEXT("Voortgangsbestand onleesbaar, begint opnieuw: %s"), *GetProgressPath());
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (Root->TryGetArrayField(TEXT("completed"), Items))
	{
		for (const TSharedPtr<FJsonValue>& Item : *Items)
		{
			FString StepId;
			if (Item.IsValid() && Item->TryGetString(StepId))
			{
				Completed.Add(StepId);
			}
		}
	}
}

void UCurriculumSubsystem::SaveProgress() const
{
	TArray<FString> Sorted = Completed.Array();
	Sorted.Sort();

	TArray<TSharedPtr<FJsonValue>> Items;
	for (const FString& StepId : Sorted)
	{
		Items.Add(MakeShared<FJsonValueString>(StepId));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), 1);
	Root->SetArrayField(TEXT("completed"), Items);

	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Root, Writer);

	if (!FFileHelper::SaveStringToFile(Text, *GetProgressPath()))
	{
		UE_LOG(LogCurriculum, Warning, TEXT("Voortgang niet opgeslagen: %s"), *GetProgressPath());
	}
}

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "CurriculumTypes.h"
#include "EditorSubsystem.h"

#include "CurriculumSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnCurriculumChanged);

/**
 * Houdt de leerlijnen, de voortgang van de student en de actieve stap bij.
 *
 * Zolang een stap open is, vraagt deze subsystem vier keer per seconde aan
 * Python (unrealemy.runner.check) of de stap klaar is. Python antwoordt via
 * ReportCheckResult. De camera van de viewport geeft geen editor-events, dus
 * polling is nodig; de kosten zijn klein bij lesscènes met weinig actors.
 */
UCLASS()
class CURRICULUMCORE_API UCurriculumSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Leerlijnen en voortgang ---
	const FCurriculumData& GetData() const { return Data; }
	const FCurriculumTrack* FindTrack(const FString& TrackId) const;
	const FCurriculumChapter* FindChapter(const FString& ChapterId, const FCurriculumTrack** OutTrack = nullptr) const;
	bool IsStepCompleted(const FString& StepId) const { return Completed.Contains(StepId); }
	int32 CountCompletedSteps(const FCurriculumChapter& Chapter) const;
	int32 CountCompletedSteps(const FCurriculumTrack& Track) const;
	static int32 CountTotalSteps(const FCurriculumTrack& Track);

	// --- Actieve stap ---
	bool HasActiveStep() const { return GetActiveStep() != nullptr; }
	const FCurriculumTrack* GetActiveTrack() const;
	const FCurriculumChapter* GetActiveChapter() const;
	const FCurriculumStep* GetActiveStep() const;
	int32 GetActiveStepIndex() const { return ActiveStepIndex; }
	bool IsActiveStepPassed() const { return bActivePassed; }
	bool IsStepPassedThisSession(const FString& StepId) const { return SessionPassed.Contains(StepId); }
	const FString& GetLastMessage() const { return LastMessage; }
	bool IsActiveChapterFinished() const;
	const FCurriculumChapter* GetNextChapter() const;
	FCurriculumHighlight GetActiveHighlight() const;

	bool CanGoNext() const;
	bool CanGoPrevious() const { return HasActiveStep() && ActiveStepIndex > 0; }

	// --- Acties vanuit de UI ---

	/** Opent een hoofdstuk bij de eerste onvoltooide stap. Al open? Dan blijft de scène staan. */
	void StartChapter(const FString& ChapterId);
	void NextStep();
	void PreviousStep();

	/** Back-up van de scène, daarna de exacte beginstaat van de actieve stap. */
	void ResetActiveStep();

	/** Docentmodus: voert de stap automatisch uit. */
	void SolveActiveStep();

	/** Sluit de leerlijn af en zet de editor terug naar de volledige indeling. */
	void LeaveCurriculum();

	/** Docentmodus: leest JSON en Python opnieuw in na aanpassingen. */
	void ReloadCurriculum();

	bool IsTeacherMode() const { return bTeacherMode; }

	// --- Brug met Python ---

	/** Aangeroepen door unrealemy.runner na elke controle. */
	UFUNCTION(BlueprintCallable, Category = "Unrealemy")
	void ReportCheckResult(const FString& StepId, bool bPassed, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Unrealemy")
	FString GetActiveStepId() const;

	/** Wordt uitgezonden bij elke wijziging die de UI moet laten zien. */
	FOnCurriculumChanged OnChanged;

private:
	bool Tick(float DeltaTime);
	void ReadSettings();
	void LoadCurriculumFiles();
	void LoadProgress();
	void SaveProgress() const;
	static FString GetProgressPath();
	void EnterStep(int32 StepIndex, bool bResetScene);
	void ApplyUiProfile(bool bEnforceOnly) const;
	const FCurriculumUiProfile* GetActiveUiProfile() const;
	bool RunPython(const FString& Code);

	FCurriculumData Data;

	/** Ooit voltooide stappen; bewaard per Windows-gebruiker. */
	TSet<FString> Completed;

	/** Stappen voltooid sinds de laatste reset van de scène. */
	TSet<FString> SessionPassed;

	FString ActiveChapterId;
	int32 ActiveStepIndex = INDEX_NONE;
	bool bActivePassed = false;
	FString LastMessage;
	double AutoAdvanceAt = 0.0;
	bool bBusy = false;

	// Instellingen uit [Unrealemy] in DefaultEditor.ini
	bool bTeacherMode = false;
	bool bAllowSkip = false;
	bool bEnforceUiProfiles = true;
	bool bPlaySounds = true;
	float AutoAdvanceDelay = 1.5f;
	float CheckInterval = 0.25f;

	FTSTicker::FDelegateHandle TickHandle;
};

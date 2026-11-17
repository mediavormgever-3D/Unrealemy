#include "CoreMinimal.h"

#include "CurriculumLog.h"
#include "CurriculumStyle.h"
#include "CurriculumTabs.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "UI/SCurriculumHighlightOverlay.h"
#include "UI/SCurriculumHub.h"
#include "UI/SCurriculumStepPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "UnrealemyModule"

DEFINE_LOG_CATEGORY(LogCurriculum);

class FCurriculumCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet() || !FSlateApplication::IsInitialized())
		{
			return;
		}
		bStarted = true;

		FCurriculumStyle::Initialize();

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CurriculumTabs::HubTabId,
				FOnSpawnTab::CreateRaw(this, &FCurriculumCoreModule::SpawnHubTab))
			.SetDisplayName(LOCTEXT("HubTab", "Leerlijnen"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CurriculumTabs::StepTabId,
				FOnSpawnTab::CreateRaw(this, &FCurriculumCoreModule::SpawnStepTab))
			.SetDisplayName(LOCTEXT("StepTab", "Unrealemy"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCurriculumCoreModule::RegisterMenus));

		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		if (MainFrame.IsWindowInitialized())
		{
			OnMainFrameReady(MainFrame.GetParentWindow(), false);
		}
		else
		{
			MainFrameHandle = MainFrame.OnMainFrameCreationFinished().AddRaw(this, &FCurriculumCoreModule::OnMainFrameReady);
		}
	}

	virtual void ShutdownModule() override
	{
		if (!bStarted)
		{
			return;
		}

		if (IMainFrameModule* MainFrame = FModuleManager::GetModulePtr<IMainFrameModule>(TEXT("MainFrame")))
		{
			MainFrame->OnMainFrameCreationFinished().Remove(MainFrameHandle);
		}

		if (const TSharedPtr<SWindow> Window = RootWindow.Pin())
		{
			if (Overlay.IsValid())
			{
				Window->RemoveOverlaySlot(Overlay.ToSharedRef());
			}
		}
		Overlay.Reset();

		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);

		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CurriculumTabs::HubTabId);
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CurriculumTabs::StepTabId);
		}

		FCurriculumStyle::Shutdown();
	}

private:
	void OnMainFrameReady(TSharedPtr<SWindow> InRootWindow, bool bIsRunningStartupDialog)
	{
		if (bIsRunningStartupDialog || !InRootWindow.IsValid())
		{
			return;
		}

		RootWindow = InRootWindow;
		InRootWindow->AddOverlaySlot()
		[
			SAssignNew(Overlay, SCurriculumHighlightOverlay, InRootWindow.ToSharedRef())
		];

		bool bOpenHub = true;
		GConfig->GetBool(TEXT("Unrealemy"), TEXT("bOpenHubOnStartup"), bOpenHub, GEditorIni);
		if (bOpenHub)
		{
			CurriculumTabs::OpenHub();
		}
	}

	TSharedRef<SDockTab> SpawnHubTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SCurriculumHub)
			];
	}

	TSharedRef<SDockTab> SpawnStepTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SCurriculumStepPanel)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		// Vaste knop in de werkbalk: de inhoudsopgave is altijd één klik weg.
		if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User")))
		{
			FToolMenuSection& Section = Toolbar->FindOrAddSection(TEXT("Unrealemy"));
			Section.AddEntry(FToolMenuEntry::InitToolBarButton(
				TEXT("UnrealemyHub"),
				FUIAction(FExecuteAction::CreateStatic(&CurriculumTabs::OpenHub)),
				LOCTEXT("ToolbarLabel", "Leerlijnen"),
				LOCTEXT("ToolbarTip", "Open het overzicht van alle leerlijnen en hoofdstukken"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Help"))));
		}

		if (UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window")))
		{
			FToolMenuSection& Section = WindowMenu->FindOrAddSection(TEXT("Unrealemy"));
			Section.Label = LOCTEXT("MenuSection", "Unrealemy");
			Section.AddMenuEntry(TEXT("UnrealemyHubMenu"),
				LOCTEXT("MenuHub", "Leerlijnen"),
				LOCTEXT("MenuHubTip", "Overzicht van alle leerlijnen en hoofdstukken"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&CurriculumTabs::OpenHub)));
			Section.AddMenuEntry(TEXT("UnrealemyStepMenu"),
				LOCTEXT("MenuStep", "Stappenpaneel"),
				LOCTEXT("MenuStepTip", "Het paneel met de huidige stap"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&CurriculumTabs::OpenStepPanel)));
		}
	}

	bool bStarted = false;
	TWeakPtr<SWindow> RootWindow;
	TSharedPtr<SCurriculumHighlightOverlay> Overlay;
	FDelegateHandle MainFrameHandle;
};

IMPLEMENT_MODULE(FCurriculumCoreModule, CurriculumCore)

#undef LOCTEXT_NAMESPACE

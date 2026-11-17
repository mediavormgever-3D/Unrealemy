#include "CurriculumTabs.h"

#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

namespace CurriculumTabs
{
	const FName HubTabId(TEXT("UnrealemyHub"));
	const FName StepTabId(TEXT("UnrealemyStep"));

	static TSharedPtr<FTabManager> GetLevelEditorTabs()
	{
		FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
		return LevelEditor ? LevelEditor->GetLevelEditorTabManager() : nullptr;
	}

	TSharedPtr<SDockTab> FindLiveTab(FName TabId)
	{
		if (const TSharedPtr<FTabManager> LevelEditorTabs = GetLevelEditorTabs())
		{
			if (TSharedPtr<SDockTab> Tab = LevelEditorTabs->FindExistingLiveTab(FTabId(TabId)))
			{
				return Tab;
			}
		}
		return FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(TabId));
	}

	void OpenTab(FName TabId, bool bBringToFront)
	{
		if (const TSharedPtr<SDockTab> Existing = FindLiveTab(TabId))
		{
			if (bBringToFront && !Existing->IsForeground())
			{
				Existing->ActivateInParent(ETabActivationCause::SetDirectly);
			}
			return;
		}

		const TSharedPtr<FTabManager> LevelEditorTabs = GetLevelEditorTabs();
		if (LevelEditorTabs.IsValid() && LevelEditorTabs->HasTabSpawner(TabId))
		{
			LevelEditorTabs->TryInvokeTab(FTabId(TabId));
			return;
		}
		FGlobalTabmanager::Get()->TryInvokeTab(FTabId(TabId));
	}

	void CloseTab(FName TabId)
	{
		if (const TSharedPtr<SDockTab> Tab = FindLiveTab(TabId))
		{
			Tab->RequestCloseTab();
		}
	}

	void OpenHub()
	{
		OpenTab(HubTabId, true);
	}

	void CloseHub()
	{
		CloseTab(HubTabId);
	}

	void OpenStepPanel()
	{
		OpenTab(StepTabId, true);
	}
}

#pragma once

#include "CoreMinimal.h"

class SDockTab;

/** Tabs van de Level Editor en van de plugin zelf tonen, verbergen en vinden. */
namespace CurriculumTabs
{
	extern const FName HubTabId;
	extern const FName StepTabId;

	/** Zoekt eerst in de Level Editor, daarna in de globale tabmanager. */
	TSharedPtr<SDockTab> FindLiveTab(FName TabId);

	void OpenTab(FName TabId, bool bBringToFront);
	void CloseTab(FName TabId);

	void OpenHub();
	void CloseHub();
	void OpenStepPanel();
}

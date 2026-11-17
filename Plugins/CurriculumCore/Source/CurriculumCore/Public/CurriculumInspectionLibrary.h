#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CurriculumInspectionLibrary.generated.h"

class UBlueprint;

/**
 * Controles die Python niet zelf kan doen. Python roept ze aan als
 * unreal.CurriculumInspectionLibrary.<functie_in_snake_case>.
 */
UCLASS()
class CURRICULUMCORE_API UCurriculumInspectionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * True als de exec-uitgang van een event (direct of via een keten van nodes)
	 * uitkomt bij een aanroep van FunctionName. Kijkt naar de grafiek zoals die
	 * in de editor staat, dus ook zonder Compile.
	 *
	 * @param EventName     Interne naam, bv. ReceiveBeginPlay voor Event BeginPlay,
	 *                      of de naam van een Custom Event.
	 * @param FunctionName  Interne naam, bv. PrintString.
	 * @param MaxDepth      Hoeveel nodes de keten maximaal lang mag zijn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Unrealemy|Inspection")
	static bool IsEventLinkedToFunction(UBlueprint* Blueprint, FName EventName, FName FunctionName, int32 MaxDepth = 8);
};

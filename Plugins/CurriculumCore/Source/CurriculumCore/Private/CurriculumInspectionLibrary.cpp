#include "CurriculumInspectionLibrary.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"

namespace
{
	bool ReachesFunction(const UEdGraphNode* Node, FName FunctionName, int32 DepthLeft, TSet<const UEdGraphNode*>& Visited)
	{
		if (!Node || DepthLeft < 0 || Visited.Contains(Node))
		{
			return false;
		}
		Visited.Add(Node);

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				const UEdGraphNode* Next = Linked ? Linked->GetOwningNode() : nullptr;
				if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Next))
				{
					if (Call->FunctionReference.GetMemberName() == FunctionName)
					{
						return true;
					}
				}
				if (ReachesFunction(Next, FunctionName, DepthLeft - 1, Visited))
				{
					return true;
				}
			}
		}
		return false;
	}
}

bool UCurriculumInspectionLibrary::IsEventLinkedToFunction(UBlueprint* Blueprint, FName EventName, FName FunctionName, int32 MaxDepth)
{
	if (!Blueprint)
	{
		return false;
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);

	for (const UEdGraph* Graph : Graphs)
	{
		if (!Graph)
		{
			continue;
		}
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			const UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
			if (!Event)
			{
				continue;
			}
			const FName Name = Event->CustomFunctionName.IsNone()
				? Event->EventReference.GetMemberName()
				: Event->CustomFunctionName;
			if (Name != EventName)
			{
				continue;
			}
			TSet<const UEdGraphNode*> Visited;
			if (ReachesFunction(Event, FunctionName, MaxDepth, Visited))
			{
				return true;
			}
		}
	}
	return false;
}

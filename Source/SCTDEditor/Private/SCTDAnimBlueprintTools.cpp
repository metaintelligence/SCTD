#include "SCTDAnimBlueprintTools.h"

#include "AnimGraphNode_BlendListByEnum.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Root.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "AnimationGraphSchema.h"
#include "BaseMonster.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_VariableGet.h"
#include "MonsterAnimInstance.h"

namespace
{
bool FailWithMessage(const FString& Message, FString& OutMessage)
{
	OutMessage = Message;
	UE_LOG(LogTemp, Warning, TEXT("SCTDAnimBlueprintTools: %s"), *OutMessage);
	return false;
}

UEdGraph* FindMainAnimGraph(UAnimBlueprint* AnimBlueprint)
{
	if (!AnimBlueprint)
	{
		return nullptr;
	}

	for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetSchema() && Graph->GetSchema()->IsA(UAnimationGraphSchema::StaticClass()))
		{
			return Graph;
		}
	}

	return nullptr;
}

UEdGraphPin* FindPosePin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsLocalSpacePosePin(Pin->PinType))
		{
			return Pin;
		}
	}

	return nullptr;
}

TArray<UEdGraphPin*> FindPosePins(UEdGraphNode* Node, EEdGraphPinDirection Direction)
{
	TArray<UEdGraphPin*> Result;
	if (!Node)
	{
		return Result;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsLocalSpacePosePin(Pin->PinType))
		{
			Result.Add(Pin);
		}
	}

	return Result;
}

UEdGraphPin* FindNonPoseInputPinContaining(UEdGraphNode* Node, const FString& NamePart)
{
	if (!Node)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && !UAnimationGraphSchema::IsPosePin(Pin->PinType) && Pin->PinName.ToString().Contains(NamePart))
		{
			return Pin;
		}
	}

	return nullptr;
}

UEdGraphPin* FindFirstOutputPin(UEdGraphNode* Node)
{
	if (!Node)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}

	return nullptr;
}

UAnimGraphNode_SequencePlayer* CreateSequenceNode(UEdGraph* Graph, UAnimationAsset* AnimationAsset, int32 NodePosX, int32 NodePosY, FString& OutMessage)
{
	UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(AnimationAsset);
	if (!Graph || !Sequence)
	{
		OutMessage = TEXT("AnimationAsset is not an AnimSequenceBase.");
		return nullptr;
	}

	FGraphNodeCreator<UAnimGraphNode_SequencePlayer> SequenceNodeCreator(*Graph);
	UAnimGraphNode_SequencePlayer* SequenceNode = SequenceNodeCreator.CreateNode();
	SequenceNode->Node.SetSequence(Sequence);
	SequenceNode->Node.SetLoopAnimation(true);
	SequenceNode->NodePosX = NodePosX;
	SequenceNode->NodePosY = NodePosY;
	SequenceNodeCreator.Finalize();
	return SequenceNode;
}
}

bool USCTDAnimBlueprintTools::BuildSingleSequenceAnimGraph(UAnimBlueprint* AnimBlueprint, UAnimationAsset* AnimationAsset, FString& OutMessage)
{
	if (!AnimBlueprint)
	{
		return FailWithMessage(TEXT("AnimBlueprint is null."), OutMessage);
	}

	if (!AnimationAsset)
	{
		OutMessage = TEXT("AnimationAsset is null.");
		return false;
	}

	UEdGraph* Graph = FindMainAnimGraph(AnimBlueprint);
	if (!Graph)
	{
		OutMessage = TEXT("Animation graph was not found.");
		return false;
	}

	TArray<UAnimGraphNode_Root*> RootNodes;
	Graph->GetNodesOfClass<UAnimGraphNode_Root>(RootNodes);
	UAnimGraphNode_Root* RootNode = RootNodes.Num() > 0 ? RootNodes[0] : nullptr;
	if (!RootNode)
	{
		OutMessage = TEXT("Anim graph root node was not found.");
		return false;
	}

	Graph->Modify();
	AnimBlueprint->Modify();

	TArray<UEdGraphNode*> NodesToRemove;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node != RootNode)
		{
			NodesToRemove.Add(Node);
		}
	}

	for (UEdGraphNode* Node : NodesToRemove)
	{
		Node->Modify();
		Node->DestroyNode();
	}

	UEdGraphPin* RootInputPin = FindPosePin(RootNode, EGPD_Input);
	if (!RootInputPin)
	{
		OutMessage = TEXT("Root pose input pin was not found.");
		return false;
	}

	UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(AnimationAsset);
	if (!Sequence)
	{
		OutMessage = TEXT("AnimationAsset is not an AnimSequenceBase.");
		return false;
	}

	FGraphNodeCreator<UAnimGraphNode_SequencePlayer> SequenceNodeCreator(*Graph);
	UAnimGraphNode_SequencePlayer* SequenceNode = SequenceNodeCreator.CreateNode();
	SequenceNode->Node.SetSequence(Sequence);
	SequenceNode->Node.SetLoopAnimation(true);
	SequenceNode->NodePosX = -260;
	SequenceNode->NodePosY = 0;
	SequenceNodeCreator.Finalize();

	UEdGraphPin* SequenceOutputPin = FindPosePin(SequenceNode, EGPD_Output);
	if (!SequenceOutputPin)
	{
		OutMessage = TEXT("Sequence output pose pin was not found.");
		return false;
	}

	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (!Schema || !Schema->TryCreateConnection(SequenceOutputPin, RootInputPin))
	{
		OutMessage = TEXT("Could not connect sequence output to root pose input.");
		return false;
	}

	TArray<UEdGraphNode*> ConnectedNodes;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node != RootNode)
		{
			UEdGraphPin* OutputPin = FindPosePin(Node, EGPD_Output);
			if (OutputPin && OutputPin->LinkedTo.Contains(RootInputPin))
			{
				ConnectedNodes.Add(Node);
			}
		}
	}

	if (ConnectedNodes.Num() == 0)
	{
		OutMessage = TEXT("Sequence node was not connected to the root pose.");
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);

	OutMessage = FString::Printf(TEXT("Built single sequence anim graph with %s."), *AnimationAsset->GetName());
	return true;
}

bool USCTDAnimBlueprintTools::BuildMonsterStateBlendAnimGraph(UAnimBlueprint* AnimBlueprint, UAnimationAsset* IdleAnimation, UAnimationAsset* WalkingAnimation, UAnimationAsset* AttackAnimation, FString& OutMessage)
{
	if (!AnimBlueprint)
	{
		OutMessage = TEXT("AnimBlueprint is null.");
		return false;
	}

	if (!IdleAnimation || !WalkingAnimation || !AttackAnimation)
	{
		return FailWithMessage(TEXT("One or more animation assets are null."), OutMessage);
	}

	UEdGraph* Graph = FindMainAnimGraph(AnimBlueprint);
	if (!Graph)
	{
		return FailWithMessage(TEXT("Animation graph was not found."), OutMessage);
	}

	TArray<UAnimGraphNode_Root*> RootNodes;
	Graph->GetNodesOfClass<UAnimGraphNode_Root>(RootNodes);
	UAnimGraphNode_Root* RootNode = RootNodes.Num() > 0 ? RootNodes[0] : nullptr;
	if (!RootNode)
	{
		return FailWithMessage(TEXT("Anim graph root node was not found."), OutMessage);
	}

	Graph->Modify();
	AnimBlueprint->Modify();

	TArray<UEdGraphNode*> NodesToRemove;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node != RootNode)
		{
			NodesToRemove.Add(Node);
		}
	}

	for (UEdGraphNode* Node : NodesToRemove)
	{
		Node->Modify();
		Node->DestroyNode();
	}

	UEdGraphPin* RootInputPin = FindPosePin(RootNode, EGPD_Input);
	if (!RootInputPin)
	{
		return FailWithMessage(TEXT("Root pose input pin was not found."), OutMessage);
	}

	FGraphNodeCreator<UAnimGraphNode_BlendListByEnum> BlendNodeCreator(*Graph);
	UAnimGraphNode_BlendListByEnum* BlendNode = BlendNodeCreator.CreateNode();
	BlendNode->NodePosX = -260;
	BlendNode->NodePosY = 0;
	BlendNodeCreator.Finalize();
	BlendNode->ReloadEnum(StaticEnum<EMonsterVisualState>());
	if (FArrayProperty* VisibleEntriesProperty = FindFProperty<FArrayProperty>(BlendNode->GetClass(), TEXT("VisibleEnumEntries")))
	{
		TArray<FName>* VisibleEntries = VisibleEntriesProperty->ContainerPtrToValuePtr<TArray<FName>>(BlendNode);
		if (VisibleEntries)
		{
			VisibleEntries->Reset();
			VisibleEntries->Add(StaticEnum<EMonsterVisualState>()->GetNameByValue(static_cast<int64>(EMonsterVisualState::Moving)));
			VisibleEntries->Add(StaticEnum<EMonsterVisualState>()->GetNameByValue(static_cast<int64>(EMonsterVisualState::Attacking)));
			BlendNode->Node.AddPose();
			BlendNode->Node.AddPose();
		}
	}
	BlendNode->ReconstructNode();

	FGraphNodeCreator<UK2Node_VariableGet> StateGetterCreator(*Graph);
	UK2Node_VariableGet* StateGetterNode = StateGetterCreator.CreateNode();
	StateGetterNode->VariableReference.SetSelfMember(GET_MEMBER_NAME_CHECKED(UMonsterAnimInstance, MonsterVisualState));
	StateGetterNode->NodePosX = -520;
	StateGetterNode->NodePosY = -220;
	StateGetterCreator.Finalize();

	UAnimGraphNode_SequencePlayer* IdleNode = CreateSequenceNode(Graph, IdleAnimation, -560, -20, OutMessage);
	UAnimGraphNode_SequencePlayer* WalkingNode = CreateSequenceNode(Graph, WalkingAnimation, -560, 180, OutMessage);
	UAnimGraphNode_SequencePlayer* AttackNode = CreateSequenceNode(Graph, AttackAnimation, -560, 380, OutMessage);
	if (!IdleNode || !WalkingNode || !AttackNode)
	{
		return false;
	}

	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (!Schema)
	{
		return FailWithMessage(TEXT("Animation graph schema was not found."), OutMessage);
	}

	UEdGraphPin* BlendOutputPin = FindPosePin(BlendNode, EGPD_Output);
	if (!BlendOutputPin || !Schema->TryCreateConnection(BlendOutputPin, RootInputPin))
	{
		return FailWithMessage(TEXT("Could not connect blend output to root pose input."), OutMessage);
	}

	UEdGraphPin* StateGetterOutputPin = FindFirstOutputPin(StateGetterNode);
	UEdGraphPin* ActiveEnumInputPin = FindNonPoseInputPinContaining(BlendNode, TEXT("Active"));
	if (!StateGetterOutputPin || !ActiveEnumInputPin || !Schema->TryCreateConnection(StateGetterOutputPin, ActiveEnumInputPin))
	{
		TArray<FString> BlendPinNames;
		for (UEdGraphPin* Pin : BlendNode->Pins)
		{
			if (Pin)
			{
				BlendPinNames.Add(FString::Printf(TEXT("%s/%s"), *Pin->PinName.ToString(), Pin->Direction == EGPD_Input ? TEXT("In") : TEXT("Out")));
			}
		}
		return FailWithMessage(FString::Printf(TEXT("Could not connect MonsterVisualState to blend active enum input. GetterPin=%s ActivePin=%s BlendPins=%s"),
			StateGetterOutputPin ? *StateGetterOutputPin->PinName.ToString() : TEXT("<null>"),
			ActiveEnumInputPin ? *ActiveEnumInputPin->PinName.ToString() : TEXT("<null>"),
			*FString::Join(BlendPinNames, TEXT(", "))), OutMessage);
	}

	TArray<UEdGraphPin*> BlendPosePins = FindPosePins(BlendNode, EGPD_Input);
	BlendPosePins.Remove(RootInputPin);
	BlendPosePins.Sort([](const UEdGraphPin& Left, const UEdGraphPin& Right)
	{
		return Left.PinName.LexicalLess(Right.PinName);
	});

	if (BlendPosePins.Num() < 3)
	{
		TArray<FString> PinNames;
		for (UEdGraphPin* Pin : BlendNode->Pins)
		{
			if (Pin)
			{
				PinNames.Add(Pin->PinName.ToString());
			}
		}
		return FailWithMessage(FString::Printf(TEXT("Blend node did not expose three pose inputs. Pins: %s"), *FString::Join(PinNames, TEXT(", "))), OutMessage);
	}

	UAnimGraphNode_SequencePlayer* SequenceNodes[] = { IdleNode, WalkingNode, AttackNode };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UEdGraphPin* SequenceOutputPin = FindPosePin(SequenceNodes[Index], EGPD_Output);
		if (!SequenceOutputPin || !Schema->TryCreateConnection(SequenceOutputPin, BlendPosePins[Index]))
		{
			return FailWithMessage(FString::Printf(TEXT("Could not connect sequence pose %d to blend pose input."), Index), OutMessage);
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);

	OutMessage = TEXT("Built MonsterVisualState blend anim graph.");
	return true;
}

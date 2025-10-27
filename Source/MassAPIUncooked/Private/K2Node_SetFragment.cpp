#include "K2Node_SetFragment.h"
#include "MassAPIBPFnLib.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_SetFragment::FragmentInPinName(TEXT("InFragment"));

UK2Node_SetFragment::UK2Node_SetFragment()
{
    FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMassAPIBPFnLib, SetFragment), UMassAPIBPFnLib::StaticClass());
}

FText UK2Node_SetFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("Set Mass Fragment"));
}

void UK2Node_SetFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UScriptStruct* OldFragmentStruct = nullptr;
    // Prioritize getting the type from the data pin itself, as this will be correct even if the pin was split.
    for (const UEdGraphPin* OldPin : OldPins)
    {
        if (OldPin && OldPin->PinName == FragmentInPinName)
        {
            if (OldPin->PinType.PinSubCategoryObject.IsValid())
            {
                OldFragmentStruct = Cast<UScriptStruct>(OldPin->PinType.PinSubCategoryObject.Get());
            }
            break;
        }
    }

    // Fallback to the type selection pin if the data pin didn't have a type
    if (!OldFragmentStruct)
    {
        OldFragmentStruct = GetFragmentStructFromOldPins(OldPins);
    }

    // Apply the found struct to the new FragmentType pin
    if (OldFragmentStruct)
    {
        if (UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName))
        {
            FragmentTypePin->DefaultObject = OldFragmentStruct;
        }
    }
}

void UK2Node_SetFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UK2Node_SetFragment::OnFragmentTypeChanged()
{
    UEdGraphPin* FragmentInPin = FindPin(FragmentInPinName);
    UScriptStruct* SelectedStruct = GetFragmentStruct();

    bool bPinTypeChanged = false;
    if (FragmentInPin && IsValidFragmentStruct(SelectedStruct))
    {
        if (FragmentInPin->PinType.PinSubCategoryObject != SelectedStruct)
        {
            FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            FragmentInPin->PinType.PinSubCategoryObject = SelectedStruct;
            bPinTypeChanged = true;
        }
    }
    else if (FragmentInPin && FragmentInPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
    {
        FragmentInPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        FragmentInPin->PinType.PinSubCategoryObject = nullptr;
        bPinTypeChanged = true;
    }

    if (bPinTypeChanged)
    {
        GetGraph()->NotifyNodeChanged(this);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
    }
}

bool UK2Node_SetFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    return Struct && Struct->IsChildOf(FMassFragment::StaticStruct());
}

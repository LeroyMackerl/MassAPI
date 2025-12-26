/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "Deprecated/K2Node_GetSharedFragment.h"
#include "MassAPIFuncLib.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_GetSharedFragment::FragmentOutPinName(TEXT("OutFragment"));

UK2Node_GetSharedFragment::UK2Node_GetSharedFragment()
{
    FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, GetSharedFragment), UMassAPIFuncLib::StaticClass());
}

FText UK2Node_GetSharedFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("GetSharedFragment(Deprecated)"));
}

FText UK2Node_GetSharedFragment::GetMenuCategory() const
{
    return FText::FromString(TEXT("MassAPI|ZDeprecated"));
}

void UK2Node_GetSharedFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UScriptStruct* OldFragmentStruct = nullptr;
    // Prioritize getting the type from the data pin itself
    for (const UEdGraphPin* OldPin : OldPins)
    {
        if (OldPin && OldPin->PinName == FragmentOutPinName)
        {
            if (OldPin->PinType.PinSubCategoryObject.IsValid())
            {
                OldFragmentStruct = Cast<UScriptStruct>(OldPin->PinType.PinSubCategoryObject.Get());
            }
            break;
        }
    }

    // Fallback to the type selection pin
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

void UK2Node_GetSharedFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UK2Node_GetSharedFragment::OnFragmentTypeChanged()
{
    UEdGraphPin* FragmentOutPin = FindPin(FragmentOutPinName);
    UScriptStruct* SelectedStruct = GetFragmentStruct();

    bool bPinTypeChanged = false;
    if (FragmentOutPin && IsValidFragmentStruct(SelectedStruct))
    {
        if (FragmentOutPin->PinType.PinSubCategoryObject != SelectedStruct)
        {
            FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
            FragmentOutPin->PinType.PinSubCategoryObject = SelectedStruct;
            bPinTypeChanged = true;
        }
    }
    else if (FragmentOutPin && FragmentOutPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
    {
        FragmentOutPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        FragmentOutPin->PinType.PinSubCategoryObject = nullptr;
        bPinTypeChanged = true;
    }

    if (bPinTypeChanged)
    {
        GetGraph()->NotifyNodeChanged(this);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
    }
}

bool UK2Node_GetSharedFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    // Override to check for FMassSharedFragment
    return Struct && Struct->IsChildOf(FMassSharedFragment::StaticStruct());
}

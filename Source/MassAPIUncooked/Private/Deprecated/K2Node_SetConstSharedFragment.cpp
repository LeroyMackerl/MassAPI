/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "Deprecated/K2Node_SetConstSharedFragment.h"
#include "MassAPIFuncLib.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_SetConstSharedFragment::FragmentInPinName(TEXT("InFragment"));

UK2Node_SetConstSharedFragment::UK2Node_SetConstSharedFragment()
{
    FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMassAPIFuncLib, SetConstSharedFragment), UMassAPIFuncLib::StaticClass());
}

FText UK2Node_SetConstSharedFragment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("SetConstSharedFragment(Deprecated)"));
}

FText UK2Node_SetConstSharedFragment::GetMenuCategory() const
{
    return FText::FromString(TEXT("MassAPI|ZDeprecated"));
}

void UK2Node_SetConstSharedFragment::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UScriptStruct* OldFragmentStruct = nullptr;
    // Prioritize getting the type from the data pin itself
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

void UK2Node_SetConstSharedFragment::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UK2Node_SetConstSharedFragment::OnFragmentTypeChanged()
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

bool UK2Node_SetConstSharedFragment::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    // Override to check for FMassConstSharedFragment
    return Struct && Struct->IsChildOf(FMassConstSharedFragment::StaticStruct());
}

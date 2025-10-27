#include "K2Node_SetSharedFragmentInTemplate.h"
#include "MassAPIBPFnLib.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_SetSharedFragmentInTemplate::FragmentInPinName(TEXT("InFragment"));

UK2Node_SetSharedFragmentInTemplate::UK2Node_SetSharedFragmentInTemplate()
{
    FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMassAPIBPFnLib, SetSharedFragmentInTemplate), UMassAPIBPFnLib::StaticClass());
}

FText UK2Node_SetSharedFragmentInTemplate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("Set Shared Fragment in Template"));
}

void UK2Node_SetSharedFragmentInTemplate::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

void UK2Node_SetSharedFragmentInTemplate::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UK2Node_SetSharedFragmentInTemplate::OnFragmentTypeChanged()
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

bool UK2Node_SetSharedFragmentInTemplate::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    // Override to check for FMassSharedFragment
    return Struct && Struct->IsChildOf(FMassSharedFragment::StaticStruct());
}

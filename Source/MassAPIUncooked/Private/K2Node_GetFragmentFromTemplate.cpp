#include "K2Node_GetFragmentFromTemplate.h"
#include "MassAPIBPFnLib.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_GetFragmentFromTemplate::FragmentOutPinName(TEXT("OutFragment"));

UK2Node_GetFragmentFromTemplate::UK2Node_GetFragmentFromTemplate()
{
    FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UMassAPIBPFnLib, GetFragmentFromTemplate), UMassAPIBPFnLib::StaticClass());
}

FText UK2Node_GetFragmentFromTemplate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString(TEXT("Get Fragment from Template"));
}

void UK2Node_GetFragmentFromTemplate::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);

    UScriptStruct* OldFragmentStruct = nullptr;
    // Prioritize getting the type from the data pin itself, as this will be correct even if the pin was split.
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

void UK2Node_GetFragmentFromTemplate::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UK2Node_GetFragmentFromTemplate::OnFragmentTypeChanged()
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

bool UK2Node_GetFragmentFromTemplate::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    return Struct && Struct->IsChildOf(FMassFragment::StaticStruct());
}

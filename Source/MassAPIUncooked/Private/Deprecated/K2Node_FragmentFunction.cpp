/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "Deprecated/K2Node_FragmentFunction.h"
#include "MassEntityTypes.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

const FName UK2Node_FragmentFunction::FragmentTypePinName(TEXT("FragmentType"));

void UK2Node_FragmentFunction::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();
}

void UK2Node_FragmentFunction::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    Super::PinDefaultValueChanged(Pin);
    if (Pin && Pin->PinName == FragmentTypePinName)
    {
        OnFragmentTypeChanged();
    }
}

void UK2Node_FragmentFunction::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);
}

void UK2Node_FragmentFunction::PostReconstructNode()
{
    Super::PostReconstructNode();
    OnFragmentTypeChanged();
}

bool UK2Node_FragmentFunction::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
    if (MyPin && MyPin->PinName == FragmentTypePinName)
    {
        if (const UScriptStruct* PinStruct = Cast<UScriptStruct>(OtherPin->PinType.PinSubCategoryObject.Get()))
        {
            if (!IsValidFragmentStruct(PinStruct))
            {
                OutReason = TEXT("The provided struct must be a valid Mass struct for this node.");
                return true;
            }
        }
    }
    return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_FragmentFunction::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);
    if (Pin && Pin->PinName == FragmentTypePinName)
    {
        if (Pin->LinkedTo.Num() == 0)
        {
            OnFragmentTypeChanged();
        }
    }
}

FText UK2Node_FragmentFunction::GetMenuCategory() const
{
    return FText::FromString(TEXT("MassAPI|Entity"));
}

UScriptStruct* UK2Node_FragmentFunction::GetFragmentStruct() const
{
    UEdGraphPin* FragmentTypePin = FindPin(FragmentTypePinName);
    if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
    {
        return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
    }
    return nullptr;
}

UScriptStruct* UK2Node_FragmentFunction::GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const
{
    const UEdGraphPin* FragmentTypePin = nullptr;
    for (const UEdGraphPin* OldPin : OldPins)
    {
        if (OldPin->PinName == FragmentTypePinName)
        {
            FragmentTypePin = OldPin;
            break;
        }
    }

    if (FragmentTypePin && FragmentTypePin->DefaultObject != nullptr && FragmentTypePin->LinkedTo.Num() == 0)
    {
        return Cast<UScriptStruct>(FragmentTypePin->DefaultObject);
    }
    return nullptr;
}

void UK2Node_FragmentFunction::OnFragmentTypeChanged()
{
    // Base implementation does nothing, intended for override
}

bool UK2Node_FragmentFunction::IsValidFragmentStruct(const UScriptStruct* Struct) const
{
    // Base implementation checks for FMassFragment.
    // Child classes should override this to check for FMassSharedFragment, etc.
    return Struct && Struct->IsChildOf(FMassFragment::StaticStruct());
}

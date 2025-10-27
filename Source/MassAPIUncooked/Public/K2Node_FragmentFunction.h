#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "UObject/ObjectMacros.h" // For FOptionalPinFromProperty
#include "K2Node_FragmentFunction.generated.h"

UCLASS(Abstract)
class MASSAPIUNCOOKED_API UK2Node_FragmentFunction : public UK2Node_CallFunction
{
    GENERATED_BODY()

public:
    // UEdGraphNode interface
    virtual void AllocateDefaultPins() override;
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void PostReconstructNode() override;
    // End of UEdGraphNode interface

    // UK2Node interface
    virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
    virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
    virtual FText GetMenuCategory() const override;
    // End of UK2Node interface

protected:
    /** Retrieves the UScriptStruct from the fragment type pin. */
    UScriptStruct* GetFragmentStruct() const;

    /** Helper to get the struct from old pins during reconstruction */
    UScriptStruct* GetFragmentStructFromOldPins(const TArray<UEdGraphPin*>& OldPins) const;

    /** Called when the FragmentType pin's value changes. */
    virtual void OnFragmentTypeChanged();

    /** * Checks if the selected struct is a valid fragment type.
     * This is intended to be overridden by child classes.
     */
    virtual bool IsValidFragmentStruct(const UScriptStruct* Struct) const;

    /** Name constants for common pins */
    static const FName FragmentTypePinName;
};

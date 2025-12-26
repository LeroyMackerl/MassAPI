// Leroy Works & Ember, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "EdGraphUtilities.h"
#endif

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if WITH_EDITOR

 // * Pin Factory，用于为 MassAPI 蓝图节点创建自定义的 Pin Widget
class FMassAPIPinFactory : public FGraphPanelPinFactory
{
public:

	//================ Factory.Override																				========

	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* InPin) const override;

};
#endif // WITH_EDITOR

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

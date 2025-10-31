// Source/RamdomItemDefense/Private/DamageTextWidget.cpp (»õ ÆÄÀÏ)

#include "DamageTextWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h" // UWidgetAnimation

void UDamageTextWidget::SetDamageText(const FText& InText)
{
	if (DamageText)
	{
		DamageText->SetText(InText);
	}
}
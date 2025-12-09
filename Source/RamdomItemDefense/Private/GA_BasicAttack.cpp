// Source/RamdomItemDefense/Private/GA_BasicAttack.cpp (수정)

#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "RamdomItemDefense.h"
#include "InventoryComponent.h"
#include "GameplayTagContainer.h"
#include "ItemTypes.h"

UGA_BasicAttack::UGA_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	DamageBase = 0.0f;
	DamageCoefficient = 1.0f;

	// 기본 태그 설정
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value"));
	SlowAmountTag = FGameplayTag::RequestGameplayTag(FName("Debuff.Slow.Amount"));
	ArmorShredAmountTag = FGameplayTag::RequestGameplayTag(FName("Debuff.ArmorShred.Amount"));
	DurationTag = FGameplayTag::RequestGameplayTag(FName("Debuff.Duration"));

}

void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 필수 데이터 검증
	if (!DamageEffectClass)
	{
		RID_LOG(FColor::Red, TEXT("GA_BasicAttack: DamageEffectClass is MISSING!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. ASC 및 타겟 액터 가져오기
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetActor || !TargetASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 공격자의 AttributeSet 가져오기
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AttributeSet)
	{
		// 드론 등 다른 Pawn일 경우 OwnerActor를 통해 재시도
		if (ARamdomItemDefenseCharacter* OwnerChar = Cast<ARamdomItemDefenseCharacter>(SourceASC->GetOwnerActor()))
		{
			AttributeSet = OwnerChar->GetAttributeSet();
		}
	}

	if (!AttributeSet)
	{
		RID_LOG(FColor::Red, TEXT("GA_BasicAttack: FAILED to get AttributeSet."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// =========================================================
	// [1] 데미지 계산 및 적용
	// =========================================================
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, false);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (DamageSpecHandle.IsValid())
	{
		if (DamageDataTag.IsValid())
		{
			DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
		}
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	// ---------------------------------------------------------
	// [1] 방어력 감소 (Armor Reduction) - 합산된 스탯 사용
	// ---------------------------------------------------------
	// 스탯은 이미 InventoryComponent가 AttributeSet에 합쳐놓았으므로 그냥 가져오면 됩니다.
	float TotalArmorRed = 0.0f;
	if (AttributeSet)
	{
		TotalArmorRed = AttributeSet->GetArmorReduction();
	}

	// 합산된 수치가 0보다 크면 '확정적으로' '한 번만' 적용
	if (TotalArmorRed > 0.0f)
	{
		// 지속시간은 기획에 따라 고정값(예: 3초) 또는 스탯에서 가져옴
		float DebuffDuration = 3.0f;
		ApplyArmorShredEffect(SourceASC, TargetASC, -TotalArmorRed, DebuffDuration);

		// 로그 확인용
		// UE_LOG(LogTemp, Log, TEXT("Applied Total Armor Shred: %.1f"), TotalArmorRed);
	}

	// ---------------------------------------------------------
	// [2] 스턴 & 슬로우 (Stun & Slow) - 아이템별 개별 확률 적용
	// ---------------------------------------------------------
	// 공격자(나)의 인벤토리 컴포넌트 가져오기
	UInventoryComponent* InventoryComp = nullptr;
	if (AActor* AvatarActor = SourceASC->GetAvatarActor())
	{
		InventoryComp = AvatarActor->FindComponentByClass<UInventoryComponent>();
	}

	if (InventoryComp)
	{
		const TArray<FName>& MyItems = InventoryComp->GetInventoryItems();

		for (const FName& ItemID : MyItems)
		{
			bool bFound = false;
			FItemData ItemData = InventoryComp->GetItemData(ItemID, bFound);

			if (!bFound || ItemData.OnHitEffects.Num() == 0) continue;

			// 이 아이템이 가진 효과들(슬로우, 스턴 등)을 순회
			for (const FItemOnHitEffect& Effect : ItemData.OnHitEffects)
			{
				// [확률 체크] 독립 시행
				if (FMath::FRand() > Effect.Chance) continue; // 꽝!

				// [효과 적용]
				switch (Effect.EffectType)
				{
				case EOnHitEffectType::Slow:
				{
					// 슬로우는 개별 적용 (곱연산 중첩됨)
					ApplySlowEffect(SourceASC, TargetASC, Effect.Magnitude, Effect.Duration);
					break;
				}
				case EOnHitEffectType::Stun:
				{
					// 스턴도 개별 적용
					ApplyStunEffect(SourceASC, TargetASC, Effect.Duration);
					break;
				}
				// 주의: ArmorReduction은 여기서 처리하지 않음 (위에서 합산 처리함)
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_BasicAttack::ApplySlowEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float SlowPercent, float Duration)
{
	if (!SlowEffectClass || !SourceASC || !TargetASC) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(SlowEffectClass, 1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 1. 슬로우 수치 전달
		float MultiplierValue = FMath::Clamp(1.0f - SlowPercent, 0.0f, 1.0f);
		if (SlowAmountTag.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(SlowAmountTag, MultiplierValue);
		}

		// 2. ★★★ [변경] 지속 시간 전달 (SetByCaller) ★★★
		// SetDuration 함수 대신, GE가 Duration Magnitude로 사용할 값을 직접 넣어줍니다.
		if (Duration > 0.0f && DurationTag.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DurationTag, Duration);

			// [로그 확인]
			UE_LOG(LogTemp, Error, TEXT(">> [DEBUG] Sending Duration: %f (Tag: %s)"), Duration, *DurationTag.ToString());
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UGA_BasicAttack::ApplyStunEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Duration)
{
	if (!StunEffectClass || !SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(StunEffectClass, 1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 스턴은 수치(Magnitude)가 필요 없고 지속 시간만 중요합니다.
		if (Duration > 0.0f && DurationTag.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DurationTag, Duration);

			// [로그 확인]
			UE_LOG(LogTemp, Warning, TEXT(">> Sent Duration via Tag: %f"), Duration);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UGA_BasicAttack::ApplyArmorShredEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float ArmorAmount, float Duration)
{
	if (!ArmorReductionEffectClass || !SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(ArmorReductionEffectClass, 1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 3. 수치 전달 (SetByCaller)
		// GE_ArmorShred 설정에서 "Modifier Op: Add"이고 "Coefficient: -1.0"으로 되어 있다면,
		// 여기서 양수(ArmorAmount)를 보내면 됩니다. (예: 10 보내면 -10 적용)
		if (ArmorShredAmountTag.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(ArmorShredAmountTag, ArmorAmount);
		}

		// 4. 지속 시간 설정
		if (Duration > 0.0f)
		{
			SpecHandle.Data.Get()->SetDuration(Duration, false);
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}
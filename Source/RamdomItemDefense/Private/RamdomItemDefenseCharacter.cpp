// Copyright Epic Games, Inc. All Rights Reserved.

#include "RamdomItemDefenseCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "MyAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "AttackComponent.h"
#include "InventoryComponent.h"
#include "GameplayTagsManager.h"
#include "ItemTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "RamdomItemDefense.h"
#include "DamageTextWidget.h" // 생성할 위젯의 헤더 (직접 만든 위젯 클래스 이름으로 변경)
#include "Components/WidgetComponent.h" // 위젯 컴포넌트 사용 시
#include "RID_DamageStatics.h" // 델리게이트 바인딩을 위해 포함
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

ARamdomItemDefenseCharacter::ARamdomItemDefenseCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create Components
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSet"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ARamdomItemDefenseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARamdomItemDefenseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* ARamdomItemDefenseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARamdomItemDefenseCharacter::ApplyDefaultStats()
{
	if (AbilitySystemComponent && DefaultStatsEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultStatsEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void ARamdomItemDefenseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		ApplyDefaultStats();

		for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
		{
			if (AbilityClass)
			{
				// FGameplayAbilitySpec을 생성하여 어빌리티를 부여합니다.
				// (레벨 1, 입력 ID 없음, 소스 오브젝트는 this)
				FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
				AbilitySystemComponent->GiveAbility(Spec);
			}
		}
	}

	if (InventoryComponent)
	{
		InventoryComponent->Initialize(AbilitySystemComponent);
	}
}

void ARamdomItemDefenseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		ApplyDefaultStats();
	}

	if (InventoryComponent)
	{
		InventoryComponent->Initialize(AbilitySystemComponent);
	}
}

void ARamdomItemDefenseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARamdomItemDefenseCharacter, ManualTarget);
}

/** (서버 전용) 스탯 강화 적용 함수 (BaseValue 수정 방식) */
void ARamdomItemDefenseCharacter::ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel)
{
	// 서버에서만 실행하고 AttributeSet이 유효한지 확인
	if (!HasAuthority() || !AttributeSet)
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		if (HasAuthority()) RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Not Server or AttributeSet is NULL"));
		// -----------------------------------------
		return;
	}

	// --- [코드 수정] BaseValue 조정 로직 ---

	// 1. 이번 강화 단계로 인해 추가되어야 할 *증가량(Delta)* 계산
	//    (NewLevel은 1부터 시작)
	float DeltaValue = 0.0f;
	switch (StatType)
	{
		// 예시: 레벨당 증가량 정의 (이 값들을 원하는 대로 조절하세요)
	case EItemStatType::AttackDamage: 			DeltaValue = 10.0f; break; // 레벨당 +10
	case EItemStatType::AttackSpeed: 			DeltaValue = 0.05f; break; // 레벨당 +5%
	case EItemStatType::CritDamage: 			DeltaValue = 0.1f; break;  // 레벨당 +10%
	case EItemStatType::ArmorReduction: 		DeltaValue = 10.0f; break;  // 레벨당 +10
	case EItemStatType::SkillActivationChance: 	DeltaValue = 0.03f; break; // 레벨당 +3%
	default:
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Invalid StatType for BaseValue modification: %s"), *UEnum::GetValueAsString(StatType));
		// -----------------------------------------
		return; // 골드 강화 불가능 스탯이면 종료
	}

	// 2. AttributeSet의 해당 스탯 BaseValue 조정 함수 호출
	switch (StatType)
	{
	case EItemStatType::AttackDamage: 			AttributeSet->AdjustBaseAttackDamage(DeltaValue); break;
	case EItemStatType::AttackSpeed: 			AttributeSet->AdjustBaseAttackSpeed(DeltaValue); break;
	case EItemStatType::CritDamage: 			AttributeSet->AdjustBaseCritDamage(DeltaValue); break;
	case EItemStatType::ArmorReduction: 		AttributeSet->AdjustBaseArmorReduction(DeltaValue); break;
	case EItemStatType::SkillActivationChance: 	AttributeSet->AdjustBaseSkillActivationChance(DeltaValue); break;
		// default는 위에서 처리했으므로 필요 없음
	}

	// 3. 로그 출력 (성공 확인)
	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	FString StatName = UEnum::GetValueAsString(StatType);
	RID_LOG(FColor::Cyan, TEXT("Applied BaseValue Upgrade: %s (Level %d, Delta: %.2f)"), *StatName, NewLevel, DeltaValue);
	// -----------------------------------------

	// GameplayEffect 적용 로직은 모두 제거됨
	// --------------------------------------
}

UAnimMontage* ARamdomItemDefenseCharacter::GetRandomAttackMontage() const
{
	// 1. 배열에 몽타주가 하나라도 있는지 확인합니다.
	if (DefaultAttackMontages.Num() == 0)
	{
		// 몽타주가 없으면 로그를 남기고 null을 반환합니다.
		RID_LOG(FColor::Red, TEXT("Character: DefaultAttackMontages 배열이 비어있습니다!"));
		return nullptr;
	}

	// 2. 0부터 (배열 크기 - 1) 사이의 랜덤 인덱스를 선택합니다.
	// (예: 3개일 경우 0, 1, 2 중 하나 선택)
	const int32 RandomIndex = FMath::RandRange(0, DefaultAttackMontages.Num() - 1);

	// 3. 해당 인덱스의 몽타주를 반환합니다.
	return DefaultAttackMontages[RandomIndex];
}

void ARamdomItemDefenseCharacter::Multicast_PlayAttack_Implementation(UAnimMontage* MontageToPlay, FRotator TargetRotation)
{
	// 1. 모든 클라이언트(나 포함)에서 즉시 회전 적용
	// (Controller의 회전이 아니라 Actor의 회전을 강제로 맞춤)
	SetActorRotation(TargetRotation);

	// 2. 그 후 애니메이션 재생
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
	}
}

void ARamdomItemDefenseCharacter::Multicast_SpawnParticleAtLocation_Implementation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale)
{
	// 서버와 모든 클라이언트(내 화면 포함)에서 실행됨
	if (EmitterTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EmitterTemplate, Location, Rotation, Scale, true);
	}
}

void ARamdomItemDefenseCharacter::Multicast_AddBuffEffect_Implementation(FGameplayTag BuffTag, UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset, FVector Scale)
{
	// 1. 이미 해당 태그로 실행 중인 이펙트가 있다면 중복 실행 방지
	if (ActiveBuffParticles.Contains(BuffTag))
	{
		return;
	}

	if (EmitterTemplate && GetMesh())
	{
		// 2. 이펙트 스폰 및 부착 (bAutoDestroy=false로 설정하여 수동 관리)
		UParticleSystemComponent* NewPSC = UGameplayStatics::SpawnEmitterAttached(
			EmitterTemplate,
			GetMesh(),
			SocketName,
			LocationOffset,
			FRotator::ZeroRotator,
			Scale,
			EAttachLocation::SnapToTarget,
			false // bAutoDestroy: 수동으로 끌 것이므로 false
		);

		if (NewPSC)
		{
			// 3. 맵에 저장하여 추적
			ActiveBuffParticles.Add(BuffTag, NewPSC);
			// RID_LOG(FColor::Green, TEXT("AddBuffEffect: Added FX for Tag [%s]"), *BuffTag.ToString());
		}
	}
}

void ARamdomItemDefenseCharacter::Multicast_RemoveBuffEffect_Implementation(FGameplayTag BuffTag)
{
	// 1. 맵에서 해당 태그의 이펙트 컴포넌트 찾기 (값의 포인터 반환)
	if (TObjectPtr<UParticleSystemComponent>* FoundPSC = ActiveBuffParticles.Find(BuffTag))
	{
		// 2. TObjectPtr을 일반 포인터로 가져옵니다. (*FoundPSC로 역참조)
		UParticleSystemComponent* PSC = *FoundPSC;

		// 3. 유효성 검사: IsValidLowLevel() 대신 IsValid()를 사용하세요. (훨씬 안전함)
		if (IsValid(PSC))
		{
			// 4. 이펙트 비활성화 및 제거
			PSC->Deactivate();
			PSC->DestroyComponent();
		}

		// 5. 맵에서 제거
		ActiveBuffParticles.Remove(BuffTag);

		// 로그 확인 (선택 사항)
		// UE_LOG(LogRamdomItemDefense, Log, TEXT("Buff Effect Removed for Tag: %s"), *BuffTag.ToString());
	}
}

void ARamdomItemDefenseCharacter::Multicast_SpawnParticleAttached_Implementation(UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset, FRotator RotationOffset, FVector Scale)
{
	if (EmitterTemplate && GetMesh())
	{
		// 소켓에 "부착"하여 스폰 (캐릭터 움직임에 따라감)
		UGameplayStatics::SpawnEmitterAttached(
			EmitterTemplate,
			GetMesh(),
			SocketName,
			LocationOffset,
			RotationOffset,
			Scale,
			EAttachLocation::SnapToTarget, // 소켓 위치/회전에 딱 붙임
			true // bAutoDestroy (1회성이므로 자동 제거)
		);
	}
}
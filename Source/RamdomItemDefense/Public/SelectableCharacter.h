#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SelectableCharacter.generated.h"

class USkeletalMeshComponent;
class UAnimationAsset;

UCLASS()
class RAMDOMITEMDEFENSE_API ASelectableCharacter : public AActor
{
	GENERATED_BODY()

public:
	ASelectableCharacter();

protected:
	virtual void BeginPlay() override;

	// 마우스 오버/클릭 델리게이트 바인딩
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;

public:
	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	// --- 설정 변수 ---
	/** 이 액터를 선택했을 때 실제 게임에서 스폰될 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config") // <-- 변경됨
    TSubclassOf<APawn> CharacterClassToSpawn;

    /** 선택 시 재생할 애니메이션 (함성, 인사 등) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config") // <-- 변경됨
    TObjectPtr<UAnimationAsset> SelectAnimation;

    /** 캐릭터 이름 (UI 표시용) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config") // <-- 변경됨
    FText CharacterName;

    /** 캐릭터 설명 (툴팁용) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (MultiLine = true)) // <-- 변경됨
    FText CharacterDescription;

	/** 선택 여부 상태 */
	bool bIsSelected;

	// --- 함수 ---
	/** 선택되었을 때 호출 (애니메이션 재생 등) */
	void PlaySelectionAnimation();

	/** 선택 해제되었을 때 (Idle로 복귀 등) */
	void ResetSelection();
};
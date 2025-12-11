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

	// --- [ Public API ] ---
	/** 선택되었을 때 호출 (애니메이션 등) */
	void PlaySelectionAnimation();

	/** 선택 해제되었을 때 (Idle 복귀) */
	void ResetSelection();

	USceneComponent* GetCameraViewPoint() const { return CameraViewPoint; }

	// --- [ Components ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> CameraViewPoint;

	// --- [ Configuration ] ---
	/** 실제 게임에서 스폰될 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TSubclassOf<APawn> CharacterClassToSpawn;

	/** 선택 시 재생할 애니메이션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UAnimationAsset> SelectAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FText CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (MultiLine = true))
	FText CharacterDescription;

	// --- [ State ] ---
	bool bIsSelected;

protected:
	virtual void BeginPlay() override;

	// --- [ Input Events ] ---
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;
};
// Private/GA_BaseSkill.cpp (수정)
#include "GA_BaseSkill.h"

UGA_BaseSkill::UGA_BaseSkill()
{
	// 모든 스킬은 기본적으로 InstancedPerActor로 설정 (타이머 등 사용 가능)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 기본값 설정 (자식 클래스에서 덮어쓸 수 있음)
	BaseActivationChance = 0.f;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	DamageBase = 0.f;
	DamageCoefficient = 0.f;
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
}
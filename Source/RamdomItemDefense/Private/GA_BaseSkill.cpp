// Private/GA_BaseSkill.cpp (�� ����)
#include "GA_BaseSkill.h"

UGA_BaseSkill::UGA_BaseSkill()
{
	// ��� ��ų�� �⺻������ InstancedPerActor�� ���� (Ÿ�̸� �� ��� ����)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// �⺻�� ���� (�ڽ� Ŭ�������� ��� �� ����)
	BaseActivationChance = 0.f;
}
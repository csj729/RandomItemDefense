// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRamdomItemDefense, Log, All);

// --- [����� ��ũ�� ����] ---
// 0���� �ٲٸ� ��� RID_LOG�� ��Ȱ��ȭ�˴ϴ�.
#define ENABLE_RID_DEBUG 0

#if ENABLE_RID_DEBUG
#include "Engine/Engine.h" // GEngine �ʿ�
/**
 * @brief Ŀ���� ����� �α� ��ũ��
 * @param Color FColor (��: FColor::Red)
 * @param Format FString::Printf ���� (��: TEXT("��: %d"), Value)
 */
#define RID_LOG(Color, Format, ...) \
	{ \
		if (GEngine) \
		{ \
			FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Msg); \
			UE_LOG(LogRamdomItemDefense, Log, TEXT("%s"), *Msg); \
		} \
	}
#else
	// ����� ��Ȱ��ȭ �� ��ũ�θ� �������� ġȯ
#define RID_LOG(Color, Format, ...) (void)0
#endif
// --- [����� ��ũ�� ��] ---
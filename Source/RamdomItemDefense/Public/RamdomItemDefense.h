// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRamdomItemDefense, Log, All);

// --- [디버그 매크로 정의] ---
// 0으로 바꾸면 모든 RID_LOG가 비활성화됩니다.
#define ENABLE_RID_DEBUG 0

#if ENABLE_RID_DEBUG
#include "Engine/Engine.h" // GEngine 필요
/**
 * @brief 커스텀 디버그 로그 매크로
 * @param Color FColor (예: FColor::Red)
 * @param Format FString::Printf 포맷 (예: TEXT("값: %d"), Value)
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
	// 디버그 비활성화 시 매크로를 공백으로 치환
#define RID_LOG(Color, Format, ...) (void)0
#endif
// --- [디버그 매크로 끝] ---
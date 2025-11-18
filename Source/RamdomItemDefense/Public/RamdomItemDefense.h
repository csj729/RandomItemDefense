// Source/RamdomItemDefense/Public/RamdomItemDefense.h (수정)

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRamdomItemDefense, Log, All);

// --- [디버그 매크로 정의] ---

// [ ★★★ 1. 마스터 스위치 ★★★ ]
#define ENABLE_RID_DEBUG 1


// [ ★★★ 2. 범용(General) 로그 매크로 ★★★ ]
#if ENABLE_RID_DEBUG
#include "Engine/Engine.h" // GEngine 필요

// [ ★★★ 수정: Verbosity 파라미터 제거 ★★★ ]
#define RID_LOG(Color, Format, ...) \
	{ \
		if (GEngine) \
		{ \
			FString Msg = FString::Printf(Format, ##__VA_ARGS__); \
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Msg); \
			/* [ ★★★ 수정: Verbosity를 'Log'로 고정 ★★★ ] */ \
			UE_LOG(LogRamdomItemDefense, Log, TEXT("%s"), *Msg); \
		} \
	}
#else
// [ ★★★ 수정: Verbosity 파라미터 제거 ★★★ ]
#define RID_LOG(Color, Format, ...) (void)0
#endif
// --- [디버그 매크로 끝] ---
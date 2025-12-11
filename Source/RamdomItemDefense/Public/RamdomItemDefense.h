// Source/RamdomItemDefense/Public/RamdomItemDefense.h (¼öÁ¤)

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRamdomItemDefense, Log, All);


#define ENABLE_RID_DEBUG 0


#if ENABLE_RID_DEBUG
#include "Engine/Engine.h"

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
#define RID_LOG(Color, Format, ...) (void)0
#endif

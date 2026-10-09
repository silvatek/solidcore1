#pragma once

#include "CoreMinimal.h"

/** Deterministic 2D value / fBm noise for seamless chunk heights (world XY in cm). */
namespace SolidCore1TerrainNoise
{
	FORCEINLINE uint32 HashCoords(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = static_cast<uint32>(X) * 374761393u + static_cast<uint32>(Y) * 668265263u
			+ static_cast<uint32>(Seed) * 2246822519u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return H ^ (H >> 16);
	}

	FORCEINLINE float HashToFloat(int32 X, int32 Y, int32 Seed)
	{
		return static_cast<float>(HashCoords(X, Y, Seed) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
	}

	FORCEINLINE float Smoothstep(float T)
	{
		return T * T * (3.f - 2.f * T);
	}

	FORCEINLINE float ValueNoise2D(float X, float Y, int32 Seed)
	{
		const int32 X0 = FMath::FloorToInt(X);
		const int32 Y0 = FMath::FloorToInt(Y);
		const float Tx = Smoothstep(X - static_cast<float>(X0));
		const float Ty = Smoothstep(Y - static_cast<float>(Y0));

		const float V00 = HashToFloat(X0, Y0, Seed);
		const float V10 = HashToFloat(X0 + 1, Y0, Seed);
		const float V01 = HashToFloat(X0, Y0 + 1, Seed);
		const float V11 = HashToFloat(X0 + 1, Y0 + 1, Seed);

		const float VX0 = FMath::Lerp(V00, V10, Tx);
		const float VX1 = FMath::Lerp(V01, V11, Tx);
		return FMath::Lerp(VX0, VX1, Ty);
	}

	/** Fractal Brownian motion in [0, 1]. */
	FORCEINLINE float Fbm2D(float X, float Y, int32 Seed, int32 Octaves = 5)
	{
		float Sum = 0.f;
		float Amp = 0.5f;
		float Freq = 1.f;
		float Norm = 0.f;

		for (int32 i = 0; i < Octaves; ++i)
		{
			Sum += ValueNoise2D(X * Freq, Y * Freq, Seed + i * 1013) * Amp;
			Norm += Amp;
			Amp *= 0.5f;
			Freq *= 2.f;
		}

		return (Norm > KINDA_SMALL_NUMBER) ? (Sum / Norm) : 0.f;
	}

	/**
	 * World-space height in cm. Frequency is in world units: smaller FrequencyScale => larger hills.
	 * Example: FrequencyScale=0.00015, Amplitude=2500 => rolling hills ~±25m.
	 */
	FORCEINLINE float SampleHeight(float WorldX, float WorldY, int32 Seed, float FrequencyScale, float Amplitude, float BaseHeight)
	{
		const float N = Fbm2D(WorldX * FrequencyScale, WorldY * FrequencyScale, Seed);
		// Keep in [0,1] for gentle rolling hills (no deep negative valleys).
		return BaseHeight + N * Amplitude;
	}

	/**
	 * Grass albedo tone in [0, 1] from world XY (cm). Higher frequency than height so patches
	 * read as surface noise rather than hill-scale color bands.
	 */
	FORCEINLINE float SampleGrassTone(float WorldX, float WorldY, int32 Seed)
	{
		const float Low = Fbm2D(WorldX * 0.0011f, WorldY * 0.0011f, Seed + 9049, 4);
		const float High = Fbm2D(WorldX * 0.0045f, WorldY * 0.0045f, Seed + 17231, 3);
		return FMath::Clamp(Low * 0.55f + High * 0.45f, 0.f, 1.f);
	}
}

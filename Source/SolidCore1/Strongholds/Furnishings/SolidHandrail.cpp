#include "SolidHandrail.h"

namespace SolidStronghold
{
	FRotator FSolidHandrail::Rotation(const FVector& Direction)
	{
		const float Horiz = FVector(Direction.X, Direction.Y, 0.f).Size();
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Direction.Z, Horiz));
		return FRotator(Pitch, Yaw, 0.f);
	}

	void FSolidHandrail::AddBar(const FVector& A, const FVector& B, const bool bHoleEnd)
	{
		const FVector Delta = B - A;
		const float Length = Delta.Size();
		if (Length < 1.f)
		{
			return;
		}
		FPiece Bar;
		Bar.Location = (A + B) * 0.5f;
		Bar.Rotation = Rotation(Delta);
		Bar.SizeCm = FVector(Length, BarCm, BarCm);
		Bar.bHoleEnd = bHoleEnd;
		Stored.Add(Bar);
	}

	void FSolidHandrail::AddPost(const FVector& BaseOnSurface, const float YawDeg, const bool bHoleEnd)
	{
		const float Height = HeightCm - BarCm * 0.5f;
		if (Height < 1.f)
		{
			return;
		}
		FPiece Post;
		Post.bPost = true;
		Post.bHoleEnd = bHoleEnd;
		Post.Location = BaseOnSurface + FVector(0.f, 0.f, Height * 0.5f);
		Post.Rotation = FRotator(0.f, YawDeg, 0.f);
		Post.SizeCm = FVector(BarCm, BarCm, Height);
		Stored.Add(Post);
	}
}

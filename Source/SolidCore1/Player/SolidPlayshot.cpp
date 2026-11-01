#include "SolidPlayshot.h"

#include "Misc/FileHelper.h"

namespace
{
	const ANSICHAR HudKeyword[] = "SolidHUD";

	uint32 PngCrc(const uint8* Data, int32 Length)
	{
		uint32 Crc = 0xFFFFFFFFu;
		for (int32 Index = 0; Index < Length; ++Index)
		{
			Crc ^= Data[Index];
			for (int32 Bit = 0; Bit < 8; ++Bit)
			{
				const uint32 Mask = 0u - (Crc & 1u);
				Crc = (Crc >> 1) ^ (0xEDB88320u & Mask);
			}
		}
		return ~Crc;
	}

	uint32 ReadBE32(const TArray<uint8>& Bytes, int32 At)
	{
		return (uint32(Bytes[At]) << 24)
			| (uint32(Bytes[At + 1]) << 16)
			| (uint32(Bytes[At + 2]) << 8)
			| uint32(Bytes[At + 3]);
	}

	void WriteBE32(TArray<uint8>& Bytes, uint32 Value)
	{
		Bytes.Add(uint8((Value >> 24) & 0xFF));
		Bytes.Add(uint8((Value >> 16) & 0xFF));
		Bytes.Add(uint8((Value >> 8) & 0xFF));
		Bytes.Add(uint8(Value & 0xFF));
	}

	bool PngSignature(const TArray<uint8>& Png)
	{
		static const uint8 Signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
		if (Png.Num() < 8)
		{
			return false;
		}
		for (int32 Index = 0; Index < 8; ++Index)
		{
			if (Png[Index] != Signature[Index])
			{
				return false;
			}
		}
		return true;
	}

	/** Offset of the IEND chunk. INDEX_NONE when the buffer is not a walkable PNG. */
	int32 FindIend(const TArray<uint8>& Png)
	{
		if (!PngSignature(Png))
		{
			return INDEX_NONE;
		}
		int32 At = 8;
		while (At + 12 <= Png.Num())
		{
			const uint32 Length = ReadBE32(Png, At);
			if (Length > uint32(Png.Num()) || At + 12 + int32(Length) > Png.Num())
			{
				return INDEX_NONE;
			}
			const bool bIend = Png[At + 4] == 'I'
				&& Png[At + 5] == 'E'
				&& Png[At + 6] == 'N'
				&& Png[At + 7] == 'D';
			if (bIend)
			{
				return At;
			}
			At += 12 + int32(Length);
		}
		return INDEX_NONE;
	}

	bool ChunkIs(const TArray<uint8>& Png, int32 At, const ANSICHAR* Type)
	{
		return Png[At + 4] == uint8(Type[0])
			&& Png[At + 5] == uint8(Type[1])
			&& Png[At + 6] == uint8(Type[2])
			&& Png[At + 7] == uint8(Type[3]);
	}

	bool NameTaken(const TArray<FString>& ExistingNames, const FString& Candidate)
	{
		for (const FString& Name : ExistingNames)
		{
			if (Name.Equals(Candidate, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}
}

FString SolidPlayshot::FileName(
	const FString& BuildId,
	const FDateTime& Time,
	const TArray<FString>& ExistingNames,
	TArray<FString>& NamesForBuild)
{
	const FString Prefix = BuildId + TEXT("_");
	NamesForBuild.Reset();
	for (const FString& Name : ExistingNames)
	{
		if (Name.StartsWith(Prefix))
		{
			NamesForBuild.Add(Name);
		}
	}

	const FString Stamp = FString::Printf(
		TEXT("%s%04d%02d%02d-%02d%02d%02d"),
		*Prefix,
		Time.GetYear(),
		Time.GetMonth(),
		Time.GetDay(),
		Time.GetHour(),
		Time.GetMinute(),
		Time.GetSecond());

	FString Candidate = Stamp + TEXT(".png");
	if (NameTaken(ExistingNames, Candidate))
	{
		for (int32 Suffix = 2; ; ++Suffix)
		{
			Candidate = FString::Printf(TEXT("%s_%d.png"), *Stamp, Suffix);
			if (!NameTaken(ExistingNames, Candidate))
			{
				break;
			}
		}
	}
	return Candidate;
}

bool SolidPlayshot::StampHud(TArray<uint8>& Png, const FString& Text)
{
	const int32 Iend = FindIend(Png);
	if (Iend == INDEX_NONE)
	{
		return false;
	}

	TArray<uint8> Body;
	for (const ANSICHAR* Cursor = HudKeyword; *Cursor; ++Cursor)
	{
		Body.Add(uint8(*Cursor));
	}
	Body.Add(0);
	Body.Add(0);
	Body.Add(0);
	Body.Add(0);
	Body.Add(0);
	const FTCHARToUTF8 Utf8(*Text);
	for (int32 Index = 0; Index < Utf8.Length(); ++Index)
	{
		Body.Add(uint8(Utf8.Get()[Index]));
	}

	TArray<uint8> Signed;
	Signed.Add(uint8('i'));
	Signed.Add(uint8('T'));
	Signed.Add(uint8('X'));
	Signed.Add(uint8('t'));
	Signed.Append(Body);
	const uint32 Crc = PngCrc(Signed.GetData(), Signed.Num());

	TArray<uint8> Chunk;
	WriteBE32(Chunk, uint32(Body.Num()));
	Chunk.Append(Signed);
	WriteBE32(Chunk, Crc);
	Png.Insert(Chunk.GetData(), Chunk.Num(), Iend);
	return true;
}

FString SolidPlayshot::ReadHud(const TArray<uint8>& Png)
{
	if (!PngSignature(Png))
	{
		return FString();
	}
	int32 At = 8;
	while (At + 12 <= Png.Num())
	{
		const uint32 Length = ReadBE32(Png, At);
		if (Length > uint32(Png.Num()) || At + 12 + int32(Length) > Png.Num())
		{
			return FString();
		}
		if (ChunkIs(Png, At, "iTXt"))
		{
			const uint8* Data = Png.GetData() + At + 8;
			TArray<uint8> Signed;
			Signed.Append(Png.GetData() + At + 4, 4 + int32(Length));
			const uint32 Stored = ReadBE32(Png, At + 8 + int32(Length));
			if (PngCrc(Signed.GetData(), Signed.Num()) == Stored)
			{
				int32 KeyEnd = 0;
				while (KeyEnd < int32(Length) && Data[KeyEnd] != 0)
				{
					++KeyEnd;
				}
				const bool bHud = KeyEnd == UE_ARRAY_COUNT(HudKeyword) - 1
					&& FMemory::Memcmp(Data, HudKeyword, KeyEnd) == 0;
				const int32 Header = KeyEnd + 1 + 2;
				if (bHud && Header + 2 <= int32(Length))
				{
					int32 TextAt = Header;
					int32 Nulls = 0;
					while (TextAt < int32(Length) && Nulls < 2)
					{
						if (Data[TextAt] == 0)
						{
							++Nulls;
						}
						++TextAt;
					}
					if (Nulls == 2 && Data[Header - 2] == 0)
					{
						TArray<uint8> Utf8;
						Utf8.Append(Data + TextAt, int32(Length) - TextAt);
						Utf8.Add(0);
						return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Utf8.GetData())));
					}
				}
			}
		}
		if (ChunkIs(Png, At, "IEND"))
		{
			return FString();
		}
		At += 12 + int32(Length);
	}
	return FString();
}

bool SolidPlayshot::StampHudFile(const FString& Path, const FString& Text)
{
	TArray<uint8> Png;
	if (!FFileHelper::LoadFileToArray(Png, *Path))
	{
		return false;
	}
	if (!StampHud(Png, Text))
	{
		return false;
	}
	return FFileHelper::SaveArrayToFile(Png, *Path);
}

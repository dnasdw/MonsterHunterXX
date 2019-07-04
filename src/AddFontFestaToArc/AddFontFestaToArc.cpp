#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct SArcHeader
{
	u32 Signature;
	u16 Unknown0x4;
	u16 Count;
	u32 Unknown0x8;
} SDW_GNUC_PACKED;

struct SRecord
{
	char FileName[64];
	u32 TypeHash;
	u32 CompressedSize;
	u32 UncompressedSize;
	u32 Offset;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

int UMain(int argc, UChar* argv[])
{
	if (argc != 2)
	{
		return 1;
	}
	FILE* fp = UFopen(argv[1], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uArcSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pArc = new u8[uArcSize];
	fread(pArc, 1, uArcSize, fp);
	fclose(fp);
	SArcHeader* pArcHeader = reinterpret_cast<SArcHeader*>(pArc);
	if (pArcHeader->Signature != SDW_CONVERT_ENDIAN32('ARC\0'))
	{
		delete[] pArc;
		return 1;
	}
	if (pArcHeader->Count != 0x123)
	{
		delete[] pArc;
		return 1;
	}
	SRecord* pRecord = reinterpret_cast<SRecord*>(pArcHeader + 1);
	memmove(pRecord + 12, pRecord + 11, (pArcHeader->Count - 11) * sizeof(SRecord));
	for (n32 i = 0; i < 1; i++)
	{
		memcpy(pRecord + 11 + i, pRecord + 7, sizeof(SRecord));
		pRecord[11 + i].FileName[0x1F] += 4 + i;
	}
	pArcHeader->Count += 1;
	fp = UFopen(argv[1], USTR("wb"), false);
	if (fp == nullptr)
	{
		delete[] pArc;
		return 1;
	}
	fwrite(pArc, 1, uArcSize, fp);
	fclose(fp);
	delete[] pArc;
	return 0;
}

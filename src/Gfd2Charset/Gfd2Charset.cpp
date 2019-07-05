#include <sdw.h>

#include SDW_MSC_PUSH_PACKED
struct SGfdHeader
{
	u32 Signature;
	u32 Unknown0x4;
	u32 Unknown0x8;
	u32 Unknown0xC;
	u32 Unknown0x10;
	u32 Unknown0x14;	// width or height
	u32 TexCount;
	u32 CharCount;
	u32 Unknown0x20;
	u32 EndCount;
	f32 End[4];
} SDW_GNUC_PACKED;

struct SCharInfo
{
	u32 Unicode;
	u32 TexIndex : 8;
	u32 X : 12;
	u32 Y : 12;
	u32 Width : 12;
	u32 Height : 12;
	u32 Unknown0xB : 8;
	u32 Width2 : 12;
	u32 Height2 : 12;
	u32 Unknown0xF : 8;
	n8 Left;
	n8 Top;
	n8 Right;
	n8 Bottom;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

int UMain(int argc, UChar* argv[])
{
	if (argc != 3)
	{
		return 1;
	}
	FILE* fp = UFopen(argv[1], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uGfdSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pGfd = new u8[uGfdSize];
	fread(pGfd, 1, uGfdSize, fp);
	fclose(fp);
	SGfdHeader* pGfdHeader = reinterpret_cast<SGfdHeader*>(pGfd);
	if (pGfdHeader->Signature != SDW_CONVERT_ENDIAN32('GFD\0'))
	{
		delete[] pGfd;
		return 1;
	}
	u32 uPathSizeOffset = sizeof(SGfdHeader) + pGfdHeader->EndCount * 4;
	u32 uPathSize = *reinterpret_cast<u32*>(pGfd + uPathSizeOffset);
	char* pPath = reinterpret_cast<char*>(pGfd + uPathSizeOffset + 4);
	if (uPathSize != strlen(pPath))
	{
		delete[] pGfd;
		return 1;
	}
	UString sName = AToU(pPath);
	UString::size_type uPos = sName.find_last_of(USTR("/\\"));
	if (uPos != UString::npos)
	{
		sName.erase(0, uPos + 1);
	}
	if (!StartWith<UString>(argv[1] + UCslen(argv[1]) - UCslen(USTR(".gfd")) - sName.size(), sName))
	{
		delete[] pGfd;
		return 1;
	}
	SCharInfo* pCharInfo = reinterpret_cast<SCharInfo*>(pPath + uPathSize + 1);
	vector<Char16_t> vCharset;
	for (u32 i = 0; i < pGfdHeader->CharCount; i++)
	{
		u32 uUnicode = pCharInfo[i].Unicode;
		if (uUnicode >= 0x10000)
		{
			delete[] pGfd;
			return 1;
		}
		Char16_t uUnicode16 = uUnicode & 0xFFFF;
		if (uUnicode16 >= 0x20)
		{
			vCharset.push_back(uUnicode16);
		}
	}
	delete[] pGfd;
	fp = UFopen(argv[2], USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite("\xFF\xFE", 2, 1, fp);
	for (n32 i = 0; i < static_cast<n32>(vCharset.size()); i++)
	{
		Char16_t uUnicode = vCharset[i];
		fwrite(&uUnicode, 2, 1, fp);
		if (i % 16 == 15)
		{
			fu16printf(fp, L"\r\n");
		}
	}
	fclose(fp);
	return 0;
}

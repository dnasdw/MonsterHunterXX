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
	SCharInfo()
		: Unicode(0)
		, TexIndex(0)
		, X(0)
		, Y(0)
		, Width(0x15)
		, Height(0x19)
		, Unknown0xB(0)
		, Width2(0x14)
		, Height2(0x25)
		, Unknown0xF(0x14)
		, Left(-1)
		, Top(6)
		, Right(-1)
		, Bottom(-1)
	{
	}
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

int UMain(int argc, UChar* argv[])
{
	if (argc != 3)
	{
		return 1;
	}
	FILE* fp = UFopen(argv[2], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uTxtSize = ftell(fp);
	if (uTxtSize % 2 != 0)
	{
		fclose(fp);
		return 1;
	}
	uTxtSize /= 2;
	fseek(fp, 0, SEEK_SET);
	Char16_t* pTemp = new Char16_t[uTxtSize + 1];
	fread(pTemp, 2, uTxtSize, fp);
	fclose(fp);
	if (pTemp[0] != 0xFEFF)
	{
		delete[] pTemp;
		return 1;
	}
	pTemp[uTxtSize] = 0;
	U16String sTxt = pTemp + 1;
	delete[] pTemp;
	set<Char16_t> sCharset;
	for (u32 i = 0; i < static_cast<u32>(sTxt.size()); i++)
	{
		Char16_t uUnicode = sTxt[i];
		if (uUnicode >= 0x20)
		{
			sCharset.insert(uUnicode);
		}
	}
	fp = UFopen(argv[1], USTR("rb"), false);
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
	map<Char16_t, u32> mCharset;
	for (u32 i = 0; i < pGfdHeader->CharCount; i++)
	{
		u32 uUnicode = pCharInfo[i].Unicode;
		if (uUnicode >= 0x10000)
		{
			delete[] pGfd;
			return 1;
		}
		Char16_t uUnicode16 = uUnicode & 0xFFFF;
		if (uUnicode16 >= 0x20 && (uUnicode16 < 0x4E00 || uUnicode16 > 0x9FA5))
		{
			mCharset.insert(make_pair(uUnicode16, i));
		}
	}
	pGfdHeader->CharCount = static_cast<u32>(sCharset.size());
	vector<SCharInfo> vCharInfo;
	vCharInfo.reserve(pGfdHeader->CharCount);
	pGfdHeader->TexCount = 0;
	u32 uX = 0x0;
	u32 uY = 0x0;
	u32 uHeightMax = 0;
	for (set<Char16_t>::iterator it = sCharset.begin(); it != sCharset.end(); ++it)
	{
		vCharInfo.push_back(SCharInfo());
		SCharInfo& charInfo = vCharInfo.back();
		Char16_t uUnicode = *it;
		map<Char16_t, u32>::iterator itOld = mCharset.find(uUnicode);
		if (itOld != mCharset.end())
		{
			memcpy(&charInfo, pCharInfo + itOld->second, sizeof(SCharInfo));
		}
		else
		{
			charInfo.Unicode = uUnicode;
		}
		if (uX + charInfo.Width + 1 > 0x400)
		{
			uX = 0;
			uY += uHeightMax + 1;
			uHeightMax = 0;
		}
		if (charInfo.Height > uHeightMax)
		{
			uHeightMax = charInfo.Height;
		}
		if (uY + uHeightMax + 1 > 0x400)
		{
			uX = 0;
			uY = 0;
			pGfdHeader->TexCount++;
		}
		charInfo.TexIndex = pGfdHeader->TexCount;
		charInfo.X = uX;
		charInfo.Y = uY;
		uX += charInfo.Width + 1;
	}
	pGfdHeader->TexCount++;
	fp = UFopen(argv[1], USTR("wb"), false);
	if (fp == nullptr)
	{
		delete[] pGfd;
		return 1;
	}
	fwrite(pGfd, 1, uPathSizeOffset + 4 + uPathSize + 1, fp);
	fwrite(&*vCharInfo.begin(), sizeof(SCharInfo), vCharInfo.size(), fp);
	fclose(fp);
	delete[] pGfd;
	return 0;
}

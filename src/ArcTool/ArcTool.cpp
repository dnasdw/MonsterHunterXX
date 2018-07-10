#include <sdw.h>
#include <zlib.h>

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
	u32 Unknown;
	u32 CompressedSize;
	u32 UncompressedSize;
	u32 Offset;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

const char* getExt(const UString& a_sPath, const u8* a_pData, u32 a_uDataSize)
{
	if (a_uDataSize < 4)
	{
		return "bin";
	}
	u32 uExtU32 = *reinterpret_cast<const u32*>(a_pData);
	if (uExtU32 == 2)
	{
		return "2";
	}
	else if (uExtU32 == 8)
	{
		return "8";
	}
	else if (uExtU32 == 0xA)
	{
		return "A";
	}
	else if (uExtU32 == 0xA0)
	{
		return "quest";
	}
	else if (uExtU32 == 0x14120103)
	{
		return "03011214";
	}
	f32 fExtF32 = *reinterpret_cast<const f32*>(a_pData);
	if (fExtF32 == 1.0f)
	{
		return "1.0";
	}
	else if (fExtF32 == 1.04f)
	{
		return "1.04";
	}
	else if (fExtF32 == 1.1f)
	{
		return "1.1";
	}
	else if (fExtF32 == 1.2f)
	{
		return "1.2";
	}
	else if (fExtF32 == 1.21f)
	{
		return "1.21";
	}
	else if (fExtF32 == 1.3f)
	{
		return "1.3";
	}
	else if (fExtF32 == 1.4f)
	{
		return "1.4";
	}
	else if (fExtF32 == 2.0f)
	{
		return "2.0";
	}
	else if (fExtF32 == 2.1f)
	{
		return "2.1";
	}
	else if (fExtF32 == 3.0f)
	{
		return "3.0";
	}
	else if (fExtF32 == 4.0f)
	{
		return "4.0";
	}
	else if (fExtF32 == 5.0f)
	{
		return "5.0";
	}
	else if (fExtF32 == 6.0f)
	{
		return "6.0";
	}
	else if (fExtF32 == 7.0f)
	{
		return "7.0";
	}
	else if (fExtF32 == 7.3f)
	{
		return "7.3";
	}
	else if (fExtF32 == 8.0f)
	{
		return "8.0";
	}
	else if (fExtF32 == 11.6f)
	{
		return "11.6";
	}
	else if (fExtF32 == 21.3f)
	{
		return "21.3";
	}
	else if (fExtF32 == 30.0f)
	{
		return "30.0";
	}
	else if (fExtF32 == 31.0f)
	{
		return "31.0";
	}
	else if (fExtF32 == 34.0f)
	{
		return "34.0";
	}
	else if (fExtF32 == 37.0f)
	{
		return "37.0";
	}
	else if (fExtF32 == 38.0f)
	{
		return "38.0";
	}
	else if (fExtF32 == 62.5f)
	{
		return "62.5";
	}
	else if (fExtF32 == 200.6f)
	{
		return "200.6";
	}
	else if (fExtF32 == 203.0f)
	{
		return "203.0";
	}
	static u8 c_szExt[5] = {};
	memcpy(c_szExt, a_pData, 4);
	for (n32 i = 0; i < 4; i++)
	{
		if (c_szExt[i] == 0xFF)
		{
			c_szExt[i] = 0;
		}
		else if (c_szExt[i] >= 'A' && c_szExt[i] <= 'Z')
		{
			c_szExt[i] = c_szExt[i] - 'A' + 'a';
		}
		else if (c_szExt[i] != 0 && (c_szExt[i] < '0' || (c_szExt[i] > '9' && c_szExt[i] < 'a') || c_szExt[i] > 'z'))
		{
			UPrintf(USTR("%") PRIUS USTR(" unknown file type: %02X %02X %02X %02X\n"), a_sPath.c_str(), a_pData[0], a_pData[1], a_pData[2], a_pData[3]);
			strcpy(reinterpret_cast<char*>(c_szExt), "bin");
			Pause();
		}
	}
	if (c_szExt[0] == 0)
	{
		UPrintf(USTR("%") PRIUS USTR(" unknown file type: %02X %02X %02X %02X\n"), a_sPath.c_str(), a_pData[0], a_pData[1], a_pData[2], a_pData[3]);
		strcpy(reinterpret_cast<char*>(c_szExt), "bin");
		Pause();
	}
	return reinterpret_cast<char*>(c_szExt);
}

int unpackArc(const UChar* a_pFileName, const UChar* a_pDirName, const UChar* a_pConflictDirName)
{
	FILE* fp = UFopen(a_pFileName, USTR("rb"), false);
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
	SRecord* pRecord = reinterpret_cast<SRecord*>(pArcHeader + 1);
	UMkdir(a_pDirName);
	FILE* fpLog = UFopen(USTR("files.txt"), USTR("ab"), false);
	if (fpLog != nullptr)
	{
		fseek(fpLog, 0, SEEK_END);
		if (ftell(fpLog) == 0)
		{
			fwrite("\xFF\xFE", 2, 1, fpLog);
		}
		fu16printf(fpLog, L"%ls\r\n", UToW(a_pFileName).c_str());
	}
	set<UString> sFilePath;
	for (n32 i = 0; i < pArcHeader->Count; i++)
	{
		uLong uUncompressedSize = pRecord[i].UncompressedSize & 0x3FFFFFF;
		u8* pUncompressed = new u8[uUncompressedSize];
		n32 nResult = uncompress(pUncompressed, &uUncompressedSize, pArc + pRecord[i].Offset, pRecord[i].CompressedSize);
		if (nResult != Z_OK)
		{
			delete[] pUncompressed;
			if (fpLog != nullptr)
			{
				fclose(fpLog);
			}
			delete[] pArc;
			return 1;
		}
		if (uUncompressedSize != (pRecord[i].UncompressedSize & 0x3FFFFFF))
		{
			delete[] pUncompressed;
			if (fpLog != nullptr)
			{
				fclose(fpLog);
			}
			delete[] pArc;
			return 1;
		}
		vector<UString> vDirPath = SplitOf(AToU(pRecord[i].FileName), USTR("/\\"));
		UString sDirName = a_pDirName;
		for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
		{
			sDirName += USTR("/") + vDirPath[j];
			UMkdir(sDirName.c_str());
		}
		UString sFileName = sDirName + USTR("/") + vDirPath.back();
		const char* pFileExt = getExt(sFileName, pUncompressed, uUncompressedSize);
		sFileName += USTR(".") + AToU(pFileExt);
		if (fpLog != nullptr)
		{
			fu16printf(fpLog, L"  %ls\r\n", UToW(sFileName).c_str());
		}
		bool bNewFile = true;
		bool bSameFile = true;
		do
		{
			FILE* fpSub = UFopen(sFileName.c_str(), USTR("rb"), false);
			if (fpSub == nullptr)
			{
				bSameFile = false;
				break;
			}
			bNewFile = false;
			fseek(fpSub, 0, SEEK_END);
			u32 uBinSize = ftell(fpSub);
			if (uBinSize != uUncompressedSize)
			{
				fclose(fpSub);
				UPrintf(USTR("%") PRIUS USTR(" size not same\n"), sFileName.c_str());
				bSameFile = false;
				break;
			}
			fseek(fpSub, 0, SEEK_SET);
			u8* pBin = new u8[uBinSize];
			fread(pBin, 1, uBinSize, fpSub);
			fclose(fpSub);
			if (memcmp(pBin, pUncompressed, uBinSize) != 0)
			{
				UPrintf(USTR("%") PRIUS USTR(" data not same\n"), sFileName.c_str());
				bSameFile = false;
			}
			delete[] pBin;
		} while (false);
		if (bNewFile)
		{
			FILE* fpSub = UFopen(sFileName.c_str(), USTR("wb"), false);
			if (fpSub == nullptr)
			{
				delete[] pUncompressed;
				if (fpLog != nullptr)
				{
					fclose(fpLog);
				}
				delete[] pArc;
				return 1;
			}
			fwrite(pUncompressed, 1, uUncompressedSize, fpSub);
			fclose(fpSub);
		}
		else if (!bSameFile)
		{
			UMkdir(a_pConflictDirName);
			UString sConflictDirName = a_pConflictDirName;
			vector<UString> vConflictDirPath = SplitOf<UString>(a_pFileName, USTR("/\\."));
			sConflictDirName += USTR("/") + vConflictDirPath[vConflictDirPath.size() - 2];
			UMkdir(sConflictDirName.c_str());
			if (sFilePath.find(sFileName) != sFilePath.end())
			{
				sConflictDirName += Format(USTR("/%d"), i);
				UMkdir(sConflictDirName.c_str());
			}
			for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
			{
				sConflictDirName += USTR("/") + vDirPath[j];
				UMkdir(sConflictDirName.c_str());
			}
			UString sConflictFileName = sConflictDirName + USTR("/") + vDirPath.back() + USTR(".") + AToU(pFileExt);
			FILE* fpSub = UFopen(sConflictFileName.c_str(), USTR("wb"), false);
			if (fpSub == nullptr)
			{
				delete[] pUncompressed;
				if (fpLog != nullptr)
				{
					fclose(fpLog);
				}
				delete[] pArc;
				return 1;
			}
			fwrite(pUncompressed, 1, uUncompressedSize, fpSub);
			fclose(fpSub);
		}
		delete[] pUncompressed;
		sFilePath.insert(sFileName);
	}
	if (fpLog != nullptr)
	{
		fclose(fpLog);
	}
	delete[] pArc;
	return 0;
}

int packArc(const UChar* a_pFileName, const UChar* a_pDirName, const UChar* a_pConflictDirName)
{
	FILE* fp = UFopen(a_pFileName, USTR("rb"), false);
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
	SRecord* pRecord = reinterpret_cast<SRecord*>(pArcHeader + 1);
	fp = UFopen(a_pFileName, USTR("wb"), false);
	if (fp == nullptr)
	{
		delete[] pArc;
		return 1;
	}
	fwrite(pArc, 1, pRecord[0].Offset, fp);
	set<UString> sFilePath;
	for (n32 i = 0; i < pArcHeader->Count; i++)
	{
		uLong uUncompressedSize = pRecord[i].UncompressedSize & 0x3FFFFFF;
		u8* pUncompressed = new u8[uUncompressedSize];
		n32 nResult = uncompress(pUncompressed, &uUncompressedSize, pArc + pRecord[i].Offset, pRecord[i].CompressedSize);
		if (nResult != Z_OK)
		{
			delete[] pUncompressed;
			fclose(fp);
			delete[] pArc;
			return 1;
		}
		if (uUncompressedSize != (pRecord[i].UncompressedSize & 0x3FFFFFF))
		{
			delete[] pUncompressed;
			fclose(fp);
			delete[] pArc;
			return 1;
		}
		vector<UString> vDirPath = SplitOf(AToU(pRecord[i].FileName), USTR("/\\"));
		UString sDirName = a_pDirName;
		for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
		{
			sDirName += USTR("/") + vDirPath[j];
		}
		UString sFileName = sDirName + USTR("/") + vDirPath.back();
		const char* pFileExt = getExt(sFileName, pUncompressed, uUncompressedSize);
		sFileName += USTR(".") + AToU(pFileExt);
		UString sConflictDirName = a_pConflictDirName;
		vector<UString> vConflictDirPath = SplitOf<UString>(a_pFileName, USTR("/\\."));
		sConflictDirName += USTR("/") + vConflictDirPath[vConflictDirPath.size() - 2];
		if (sFilePath.find(sFileName) != sFilePath.end())
		{
			sConflictDirName += Format(USTR("/%d"), i);
		}
		for (n32 j = 0; j < static_cast<n32>(vDirPath.size()) - 1; j++)
		{
			sConflictDirName += USTR("/") + vDirPath[j];
		}
		UString sConflictFileName = sConflictDirName + USTR("/") + vDirPath.back() + USTR(".") + AToU(pFileExt);
		FILE* fpSub = UFopen(sConflictFileName.c_str(), USTR("rb"), false);
		if (fpSub == nullptr)
		{
			fpSub = UFopen(sFileName.c_str(), USTR("rb"), false);
			if (fpSub == nullptr)
			{
				delete[] pUncompressed;
				fclose(fp);
				delete[] pArc;
				return 1;
			}
		}
		fseek(fpSub, 0, SEEK_END);
		u32 uBinSize = ftell(fpSub);
		fseek(fpSub, 0, SEEK_SET);
		u8* pBin = new u8[uBinSize];
		fread(pBin, 1, uBinSize, fpSub);
		fclose(fpSub);
		bool bSame = uBinSize == uUncompressedSize && memcmp(pBin, pUncompressed, uBinSize) == 0;
		delete[] pUncompressed;
		u32 uOffsetOld = pRecord[i].Offset;
		pRecord[i].Offset = ftell(fp);
		if (bSame)
		{
			fwrite(pArc + uOffsetOld, 1, pRecord[i].CompressedSize, fp);
		}
		else
		{
			uLong uCompressedSize = compressBound(uBinSize);
			u8* pCompressed = new u8[uCompressedSize];
			n32 nResult = compress(pCompressed, &uCompressedSize, pBin, uBinSize);
			if (nResult != Z_OK)
			{
				delete[] pCompressed;
				delete[] pBin;
				fclose(fp);
				delete[] pArc;
				return 1;
			}
			fwrite(pCompressed, 1, uCompressedSize, fp);
			delete[] pCompressed;
			pRecord[i].CompressedSize = uCompressedSize;
			pRecord[i].UncompressedSize &= 0xFC000000;
			pRecord[i].UncompressedSize |= uBinSize;
		}
		delete[] pBin;
		sFilePath.insert(sFileName);
	}
	fseek(fp, 0, SEEK_SET);
	fwrite(pArc, 1, pRecord[0].Offset, fp);
	fclose(fp);
	delete[] pArc;
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	if (argc != 5)
	{
		return 1;
	}
	if (UCslen(argv[1]) == 1)
	{
		switch (*argv[1])
		{
		case USTR('U'):
		case USTR('u'):
			return unpackArc(argv[2], argv[3], argv[4]);
		case USTR('P'):
		case USTR('p'):
			return packArc(argv[2], argv[3], argv[4]);
		default:
			break;
		}
	}
	return 1;
}

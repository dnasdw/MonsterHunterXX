#include <sdw.h>
#include <PVRTextureUtilities.h>

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

struct STexHeader
{
	u32 Signature;
	u8 Unknown0x4[4];
	u32 MipmapLevel : 6;
	u32 Width : 13;
	u32 Height : 13;
	u8 Count;
	u8 Format;
	u8 Unknown0xE[2];
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

enum EConst
{
	kConstGroupsOfBytesWidth = 64,
	kConstGroupsOfBytesHeight = 8,
	kConst16B = 16
};

enum ETextureFormat
{
	kTextureFormat_R8_G8_B8_A8_0x07 = 7,
	kTextureFormat_Bc1_0x13 = 0x13,
	kTextureFormat_Bc2_0x15 = 0x15,
	kTextureFormat_Bc3_0x17 = 0x17,
	kTextureFormat_Bc4_0x19 = 0x19,
	kTextureFormat_Bc1_0x1E = 0x1E,
	kTextureFormat_Bc5_0x1F = 0x1F,
	kTextureFormat_Bc3_0x20 = 0x20,
	kTextureFormat_Bc3_0x2A = 0x2A
};

static const n32 s_nDecodeTransByteNX[32] =
{
	 0,  2, 16, 18,
	 1,  3, 17, 19,
	 4,  6, 20, 22,
	 5,  7, 21, 23,
	 8, 10, 24, 26,
	 9, 11, 25, 27,
	12, 14, 28, 30,
	13, 15, 29, 31
};

n32 getBoundWidth(n32 a_nWidth, n32 a_nFormat)
{
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 4));
	}
	return 0;
}

n32 getBoundHeight(n32 a_nHeight, n32 a_nFormat)
{
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		return static_cast<n32>(Align(a_nHeight, kConstGroupsOfBytesHeight));
	}
	return 0;
}

n32 getBlockHeight(n32 a_nHeight, n32 a_nFormat)
{
	n32 nBlockHeight = 1;
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		nBlockHeight = a_nHeight / kConstGroupsOfBytesHeight;
		break;
	}
	if (nBlockHeight > 16)
	{
		nBlockHeight = 16;
	}
	return nBlockHeight;
}

bool code32BPPTex(u8* a_pTex, bool a_bDecode)
{
	STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(a_pTex);
	if (pTexHeader->Signature != SDW_CONVERT_ENDIAN32('TEX\0'))
	{
		return false;
	}
	if (pTexHeader->Count != 1)
	{
		return false;
	}
	if (pTexHeader->Format != kTextureFormat_R8_G8_B8_A8_0x07)
	{
		return false;
	}
	u32 uDataOffset = *reinterpret_cast<u32*>(a_pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4);
	if (uDataOffset != 0)
	{
		return false;
	}
	n32 nWidth = getBoundWidth(pTexHeader->Width, pTexHeader->Format);
	n32 nHeight = getBoundHeight(pTexHeader->Height, pTexHeader->Format);
	n32 nBlockHeight = getBlockHeight(nHeight, pTexHeader->Format);
	u32 uOffset = sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + pTexHeader->MipmapLevel * 4 + (pTexHeader->Count != 1 ? 4 : 0) + uDataOffset;
	u8* pBuffer = a_pTex + uOffset;
	n32 nBytePerBlock = 4;
	if (a_bDecode)
	{
		u8* pTemp = new u8[nWidth * nHeight * nBytePerBlock];
		for (n32 i = 0; i < nWidth * nHeight * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
		{
			for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
			{
				memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
			}
		}
		{
			n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
			n32 nInnerHeight = nBlockHeight * kConstGroupsOfBytesHeight;
			n32 nOutterWidth = nWidth / nInnerWidth;
			n32 nOutterHeight = nHeight / nInnerHeight;
			for (n32 i = 0; i < nWidth * nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
			{
				u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
				u8* pDest = pBuffer + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
				for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
				{
					for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
					{
						memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
					}
				}
			}
		}
		delete[] pTemp;
	}
	else
	{
		PVRTextureHeaderV3 pvrTextureHeaderV3;
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PVRStandard8PixelType.PixelTypeID;
		pvrTextureHeaderV3.u32Height = nHeight;
		pvrTextureHeaderV3.u32Width = nWidth;
		MetaDataBlock metaDataBlock;
		metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
		metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
		metaDataBlock.u32DataSize = 3;
		metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
		metaDataBlock.Data[0] = ePVRTOrientRight;
		metaDataBlock.Data[1] = ePVRTOrientUp;
		metaDataBlock.Data[2] = ePVRTOrientIn;
		pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pBuffer);
		if (pTexHeader->MipmapLevel != 1)
		{
			pvrtexture::GenerateMIPMaps(*pPVRTexture, pvrtexture::eResizeNearest, pTexHeader->MipmapLevel);
		}
		pvrtexture::ECompressorQuality eCompressorQuality = pvrtexture::ePVRTCBest;
		pvrtexture::uint64 uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		pvrtexture::Transcode(*pPVRTexture, uPixelFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eCompressorQuality);
		for (u32 i = 0; i < pTexHeader->MipmapLevel; i++)
		{
			n32 nMipmapWidthSrc = pPVRTexture->getWidth(i);
			n32 nMipmapHeightSrc = pPVRTexture->getHeight(i);
			n32 nMipmapWidthDest = nWidth >> i;
			n32 nMipmapHeightDest = nHeight >> i;
			nMipmapWidthDest = getBoundWidth(nMipmapWidthDest, pTexHeader->Format);
			nMipmapHeightDest = getBoundHeight(nMipmapHeightDest, pTexHeader->Format);
			nBlockHeight = getBlockHeight(nMipmapHeightDest, pTexHeader->Format);
			u32 uDataOffset = *reinterpret_cast<u32*>(a_pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + i * 4);
			uOffset = sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + pTexHeader->MipmapLevel * 4 + (pTexHeader->Count != 1 ? 4 : 0) + uDataOffset;
			u8* pData = reinterpret_cast<u8*>(pPVRTexture->getDataPtr(i));
			pBuffer = a_pTex + uOffset;
			for (n32 nY = 0; nY < nMipmapHeightSrc; nY++)
			{
				for (n32 nX = 0; nX < nMipmapWidthDest; nX++)
				{
					*(reinterpret_cast<u32*>(pBuffer) + nY * nMipmapWidthDest + nX) = *(reinterpret_cast<u32*>(pData) + nY * nMipmapWidthSrc + nX);
				}
			}
			u8* pTemp = new u8[nMipmapWidthDest * nMipmapHeightDest * nBytePerBlock];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nMipmapWidthDest / nInnerWidth;
				n32 nOutterHeight = nMipmapHeightDest / nInnerHeight;
				for (n32 i = 0; i < nMipmapWidthDest * nMipmapHeightDest * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					u8* pSrc = pBuffer + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, pSrc + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			for (n32 i = 0; i < nMipmapWidthDest * nMipmapHeightDest * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, kConst16B);
				}
			}
			delete[] pTemp;
		}
	}
	return true;
}

void setTexPoint(u8* a_pTex, u32 a_uX, u32 a_uY, u8 a_uLevel)
{
	STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(a_pTex);
	n32 nWidth = getBoundWidth(pTexHeader->Width, pTexHeader->Format);
	n32 nHeight = getBoundHeight(pTexHeader->Height, pTexHeader->Format);
	u32 uHeaderSize = sizeof(STexHeader) + 4 + pTexHeader->MipmapLevel * 4;
	*(reinterpret_cast<u32*>(a_pTex + uHeaderSize) + a_uY * nWidth + a_uX) = a_uLevel << 24 | 0xFFFFFF;
}

u8 getTexPoint(u8* a_pTex, u32 a_uX, u32 a_uY)
{
	STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(a_pTex);
	n32 nWidth = getBoundWidth(pTexHeader->Width, pTexHeader->Format);
	n32 nHeight = getBoundHeight(pTexHeader->Height, pTexHeader->Format);
	u32 uHeaderSize = sizeof(STexHeader) + 4 + pTexHeader->MipmapLevel * 4;
	u8 uLevel = *(reinterpret_cast<u32*>(a_pTex + uHeaderSize) + a_uY * nWidth + a_uX) >> 24 & 0xFF;
	return uLevel;
}

u8 getBmpPoint(u8* a_pBmp, u32 a_uX, u32 a_uY)
{
	BITMAPFILEHEADER* pBitmapFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(a_pBmp);
	BITMAPINFOHEADER* pBitmapInfoHeader = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeader + 1);
	u32 uOffset = pBitmapFileHeader->bfOffBits + ((pBitmapInfoHeader->biHeight - 1 - a_uY) * pBitmapInfoHeader->biWidth + a_uX) * (pBitmapInfoHeader->biBitCount / 8);
	u8 uLevel = a_pBmp[uOffset + 3];
	return uLevel;
}

int UMain(int argc, UChar* argv[])
{
	if (argc != 8)
	{
		return 1;
	}
	u32 uXInCell = SToU32(argv[6]);
	u32 uYInCell = SToU32(argv[7]);
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
	map<Char16_t, SCharInfo> mCharsetOld;
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
			mCharsetOld.insert(make_pair(uUnicode16, pCharInfo[i]));
		}
	}
	delete[] pGfd;
	fp = UFopen(argv[2], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	uGfdSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	pGfd = new u8[uGfdSize];
	fread(pGfd, 1, uGfdSize, fp);
	fclose(fp);
	pGfdHeader = reinterpret_cast<SGfdHeader*>(pGfd);
	u32 uTexCount = pGfdHeader->TexCount;
	delete[] pGfd;
	fp = UFopen(argv[4], USTR("rb"), false);
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
	map<Char16_t, u32> mCharset;
	for (u32 i = 0; i < static_cast<u32>(sTxt.size()); i++)
	{
		Char16_t uUnicode = sTxt[i];
		if (uUnicode >= 0x20)
		{
			mCharset.insert(make_pair(uUnicode, static_cast<u32>(mCharset.size())));
		}
	}
	map<u32, vector<u8>> mTexFileOld;
	map<u32, vector<u8>> mTexFile;
	u32 uHeaderSize = sizeof(STexHeader) + 4 + 11 * 4;
	u32 uFontSize = uHeaderSize;
	for (n32 i = 0; i < 11; i++)
	{
		n32 nWidth = 1024 >> i;
		n32 nHeight = 1024 >> i;
		nWidth = getBoundWidth(nWidth, kTextureFormat_R8_G8_B8_A8_0x07);
		nHeight = getBoundHeight(nHeight, kTextureFormat_R8_G8_B8_A8_0x07);
		uFontSize += nWidth * nHeight * 4;
	}
	fp = UFopen(argv[3], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uFont0Size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pFont = new u8[uFont0Size];
	fread(pFont, 1, uFont0Size, fp);
	fclose(fp);
	if (!code32BPPTex(pFont, true))
	{
		delete[] pFont;
		return 1;
	}
	STexHeader* pFont0Header = reinterpret_cast<STexHeader*>(pFont);
	u32 uHeader0Size = sizeof(STexHeader) + 4 + pFont0Header->MipmapLevel * 4;
	for (n32 i = 0; i < 1; i++)
	{
		mTexFileOld[i].resize(uFontSize);
		memset(&*mTexFileOld[i].begin(), 0, uFontSize);
		memcpy(&*mTexFileOld[i].begin(), pFont, sizeof(STexHeader));
		STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(&*mTexFileOld[i].begin());
		pTexHeader->MipmapLevel = 11;
		pTexHeader->Width = 1024;
		pTexHeader->Height = 1024;
	}
	for (u32 i = 0; i < uTexCount; i++)
	{
		mTexFile[i].resize(uFontSize);
		memset(&*mTexFile[i].begin(), 0, uFontSize);
		memcpy(&*mTexFile[i].begin(), pFont, sizeof(STexHeader));
		STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(&*mTexFile[i].begin());
		pTexHeader->MipmapLevel = 11;
		pTexHeader->Width = 1024;
		pTexHeader->Height = 1024;
		u32 uTotalSize = 0;
		for (u32 j = 0; j < pTexHeader->MipmapLevel; j++)
		{
			*reinterpret_cast<u32*>(&*mTexFile[i].begin() + sizeof(STexHeader) + 4 + j * 4) = uTotalSize;
			n32 nWidth = 1024 >> j;
			n32 nHeight = 1024 >> j;
			nWidth = getBoundWidth(nWidth, kTextureFormat_R8_G8_B8_A8_0x07);
			nHeight = getBoundHeight(nHeight, kTextureFormat_R8_G8_B8_A8_0x07);
			uTotalSize += nWidth * nHeight * 4;
			if (j == 0)
			{
				for (u32 uY = 0; uY < pTexHeader->Height; uY++)
				{
					for (u32 uX = 0; uX < pTexHeader->Width; uX++)
					{
						setTexPoint(&*mTexFile[i].begin(), uX, uY, 0);
					}
				}
			}
		}
		*reinterpret_cast<u32*>(&*mTexFile[i].begin() + sizeof(STexHeader)) = uTotalSize;
	}
	STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(&*mTexFileOld[0].begin());
	for (u32 i = 0; i < pFont0Header->Height; i++)
	{
		memcpy(&*mTexFileOld[0].begin() + uHeaderSize + i * 1024 * 4, pFont + uHeader0Size + i * pFont0Header->Width * 4, pFont0Header->Width * 4);
	}
	delete[] pFont;
	fp = UFopen(argv[5], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uBmpSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pBmp = new u8[uBmpSize];
	fread(pBmp, 1, uBmpSize, fp);
	fclose(fp);
	BITMAPFILEHEADER* pBitmapFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(pBmp);
	BITMAPINFOHEADER* pBitmapInfoHeader = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeader + 1);
	u32 uBlockWidth = pBitmapInfoHeader->biWidth / 16;
	u32 uBlockHeight = pBitmapInfoHeader->biHeight / static_cast<u32>(Align(mCharset.size(), 16) / 16);
	u32 uTexIndex = 0;
	u32 uX = 0x0;
	u32 uY = 0x0;
	u32 uHeightMax = 0;
	for (map<Char16_t, u32>::iterator it = mCharset.begin(); it != mCharset.end(); ++it)
	{
		Char16_t uUnicode = it->first;
		map<Char16_t, SCharInfo>::iterator itOld = mCharsetOld.find(uUnicode);
		bool bCopyFromTexOld = false;
		SCharInfo* pCharInfo = nullptr;
		u32 uCharWidth = 0;
		u32 uCharHeight = 0;
		if (itOld != mCharsetOld.end())
		{
			bCopyFromTexOld = true;
			pCharInfo = &itOld->second;
			uCharWidth = pCharInfo->Width;
			uCharHeight = pCharInfo->Height;
		}
		else
		{
			uCharWidth = 0x4B;
			uCharHeight = 0x49;
		}
		if (uX + uCharWidth + 1 > 0x400)
		{
			uX = 0;
			uY += uHeightMax + 1;
			uHeightMax = 0;
		}
		if (uCharHeight > uHeightMax)
		{
			uHeightMax = uCharHeight;
		}
		if (uY + uHeightMax + 1 > 0x400)
		{
			uX = 0;
			uY = 0;
			uTexIndex++;
		}
		if (bCopyFromTexOld)
		{
			for (u32 i = 0; i < pCharInfo->Height; i++)
			{
				for (u32 j = 0; j < pCharInfo->Width; j++)
				{
					setTexPoint(&*mTexFile[uTexIndex].begin(), uX + j, uY + i, getTexPoint(&*mTexFileOld[pCharInfo->TexIndex].begin(), pCharInfo->X + j, pCharInfo->Y + i));
				}
			}
		}
		else
		{
			u32 uPos = it->second;
			u32 uPosX = uPos % 16;
			u32 uPosY = uPos / 16;
			for (u32 i = 0; i < 73; i++)
			{
				for (u32 j = 0; j < 75; j++)
				{
					setTexPoint(&*mTexFile[uTexIndex].begin(), uX + j, uY + i, getBmpPoint(pBmp, uPosX * uBlockWidth + 2 + uXInCell + j, uPosY * uBlockHeight + 2 + uYInCell + i));
				}
			}
		}
		uX += uCharWidth + 1;
	}
	delete[] pBmp;
	for (u32 i = 0; i < uTexCount; i++)
	{
		UString sFileName = Format(USTR("boss_demo_%02d_HQ.tex"), i);
		fp = UFopen(sFileName.c_str(), USTR("wb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		code32BPPTex(&*mTexFile[i].begin(), false);
		fwrite(&*mTexFile[i].begin(), 1, uFontSize, fp);
		fclose(fp);
	}
	return 0;
}

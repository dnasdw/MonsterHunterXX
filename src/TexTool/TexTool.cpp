#include <sdw.h>
#include <BC.h>
#include <png.h>
#include <PVRTextureUtilities.h>

enum EConst
{
	kConstGroupsOfBytesWidth = 64,
	kConstGroupsOfBytesHeight = 8,
	kConst16B = 16
};

#include SDW_MSC_PUSH_PACKED
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
	case kTextureFormat_Bc1_0x13:
	case kTextureFormat_Bc1_0x1E:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 8 * 4));
	case kTextureFormat_Bc2_0x15:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 16 * 4));
	case kTextureFormat_Bc3_0x17:
	case kTextureFormat_Bc3_0x20:
	case kTextureFormat_Bc3_0x2A:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 16 * 4));
	case kTextureFormat_Bc4_0x19:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 8 * 4));
	case kTextureFormat_Bc5_0x1F:
		return static_cast<n32>(Align(a_nWidth, kConstGroupsOfBytesWidth / 16 * 4));
	}
	return 0;
}

n32 getBoundHeight(n32 a_nHeight, n32 a_nFormat)
{
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		return static_cast<n32>(Align(a_nHeight, kConstGroupsOfBytesHeight));
	case kTextureFormat_Bc1_0x13:
	case kTextureFormat_Bc2_0x15:
	case kTextureFormat_Bc3_0x17:
	case kTextureFormat_Bc4_0x19:
	case kTextureFormat_Bc1_0x1E:
	case kTextureFormat_Bc5_0x1F:
	case kTextureFormat_Bc3_0x20:
	case kTextureFormat_Bc3_0x2A:
		return static_cast<n32>(Align(a_nHeight, kConstGroupsOfBytesHeight * 4));
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
	case kTextureFormat_Bc1_0x13:
	case kTextureFormat_Bc2_0x15:
	case kTextureFormat_Bc3_0x17:
	case kTextureFormat_Bc4_0x19:
	case kTextureFormat_Bc1_0x1E:
	case kTextureFormat_Bc5_0x1F:
	case kTextureFormat_Bc3_0x20:
	case kTextureFormat_Bc3_0x2A:
		nBlockHeight = a_nHeight / 4 / kConstGroupsOfBytesHeight;
		break;
	}
	if (nBlockHeight > 16)
	{
		nBlockHeight = 16;
	}
	return nBlockHeight;
}

int decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nBlockHeight, pvrtexture::CPVRTexture** a_pPVRTexture)
{
	u8* pRGBA = nullptr;
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		{
			n32 nBytePerBlock = 4;
			u8* pTemp = new u8[a_nWidth * a_nHeight * nBytePerBlock];
			for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * nBytePerBlock];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	case kTextureFormat_Bc1_0x13:
	case kTextureFormat_Bc1_0x1E:
		{
			n32 nBlockWidth = 4;
			n32 nBlockHeight = 4;
			n32 nBlockColumn = a_nWidth / nBlockWidth;
			n32 nBlockRow = a_nHeight / nBlockHeight;
			n32 nBytePerBlock = 8;
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nBlockColumn / nInnerWidth;
				n32 nOutterHeight = nBlockRow / nInnerHeight;
				for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			{
				for (n32 i = 0; i < nBlockColumn * nBlockRow; i++)
				{
					const u8* pSrc = pRGBA + i * 8;
					u8* pDest = pTemp + i * 64;
					DirectX::D3DXDecodeBC1(pDest, pSrc);
				}
			}
			{
				n32 nInnerWidth = 4;
				n32 nInnerHeight = 4;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				nBytePerBlock = 4;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	case kTextureFormat_Bc2_0x15:
		{
			n32 nBlockWidth = 4;
			n32 nBlockHeight = 4;
			n32 nBlockColumn = a_nWidth / nBlockWidth;
			n32 nBlockRow = a_nHeight / nBlockHeight;
			n32 nBytePerBlock = 16;
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nBlockColumn / nInnerWidth;
				n32 nOutterHeight = nBlockRow / nInnerHeight;
				for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			{
				for (n32 i = 0; i < nBlockColumn * nBlockRow; i++)
				{
					const u8* pSrc = pRGBA + i * 16;
					u8* pDest = pTemp + i * 64;
					DirectX::D3DXDecodeBC2(pDest, pSrc);
				}
			}
			{
				n32 nInnerWidth = 4;
				n32 nInnerHeight = 4;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				nBytePerBlock = 4;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	case kTextureFormat_Bc3_0x17:
	case kTextureFormat_Bc3_0x20:
	case kTextureFormat_Bc3_0x2A:
		{
			n32 nBlockWidth = 4;
			n32 nBlockHeight = 4;
			n32 nBlockColumn = a_nWidth / nBlockWidth;
			n32 nBlockRow = a_nHeight / nBlockHeight;
			n32 nBytePerBlock = 16;
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nBlockColumn / nInnerWidth;
				n32 nOutterHeight = nBlockRow / nInnerHeight;
				for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			{
				for (n32 i = 0; i < nBlockColumn * nBlockRow; i++)
				{
					const u8* pSrc = pRGBA + i * 16;
					u8* pDest = pTemp + i * 64;
					DirectX::D3DXDecodeBC3(pDest, pSrc);
				}
			}
			{
				n32 nInnerWidth = 4;
				n32 nInnerHeight = 4;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				nBytePerBlock = 4;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	case kTextureFormat_Bc4_0x19:
		{
			n32 nBlockWidth = 4;
			n32 nBlockHeight = 4;
			n32 nBlockColumn = a_nWidth / nBlockWidth;
			n32 nBlockRow = a_nHeight / nBlockHeight;
			n32 nBytePerBlock = 8;
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nBlockColumn / nInnerWidth;
				n32 nOutterHeight = nBlockRow / nInnerHeight;
				for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			{
				for (n32 i = 0; i < nBlockColumn * nBlockRow; i++)
				{
					const u8* pSrc = pRGBA + i * 8;
					u8* pDest = pTemp + i * 16;
					DirectX::D3DXDecodeBC4U(pDest, pSrc);
				}
			}
			{
				n32 nInnerWidth = 4;
				n32 nInnerHeight = 4;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				nBytePerBlock = 1;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	case kTextureFormat_Bc5_0x1F:
		{
			n32 nBlockWidth = 4;
			n32 nBlockHeight = 4;
			n32 nBlockColumn = a_nWidth / nBlockWidth;
			n32 nBlockRow = a_nHeight / nBlockHeight;
			n32 nBytePerBlock = 16;
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight); i++)
			{
				for (n32 j = 0; j < kConstGroupsOfBytesWidth / kConst16B * kConstGroupsOfBytesHeight; j++)
				{
					memcpy(pTemp + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + j * kConst16B, a_pBuffer + i * (kConstGroupsOfBytesWidth * kConstGroupsOfBytesHeight) + s_nDecodeTransByteNX[j] * kConst16B, kConst16B);
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			{
				n32 nInnerWidth = kConstGroupsOfBytesWidth / nBytePerBlock;
				n32 nInnerHeight = a_nBlockHeight * kConstGroupsOfBytesHeight;
				n32 nOutterWidth = nBlockColumn / nInnerWidth;
				n32 nOutterHeight = nBlockRow / nInnerHeight;
				for (n32 i = 0; i < nBlockColumn * nBlockRow * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					for (n32 j = 0; j < nInnerHeight * nOutterHeight; j++)
					{
						for (n32 k = 0; k < nInnerWidth * nOutterWidth; k++)
						{
							memcpy(pDest + (j * nInnerWidth * nOutterWidth + k) * nBytePerBlock, pSrc + (((j / nInnerHeight) * nOutterWidth + (k / nInnerWidth)) * (nInnerWidth * nInnerHeight) + j % nInnerHeight * nInnerWidth + k % nInnerWidth) * nBytePerBlock, nBytePerBlock);
						}
					}
				}
			}
			{
				for (n32 i = 0; i < nBlockColumn * nBlockRow; i++)
				{
					const u8* pSrc = pRGBA + i * 16;
					u8* pDest = pTemp + i * 32;
					DirectX::D3DXDecodeBC5U(pDest, pSrc);
				}
			}
			{
				n32 nInnerWidth = 4;
				n32 nInnerHeight = 4;
				n32 nOutterWidth = a_nWidth / nInnerWidth;
				n32 nOutterHeight = a_nHeight / nInnerHeight;
				nBytePerBlock = 2;
				for (n32 i = 0; i < a_nWidth * a_nHeight * nBytePerBlock / (nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock); i++)
				{
					const u8* pSrc = pTemp + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
					u8* pDest = pRGBA + i * nInnerWidth * nInnerHeight * nOutterWidth * nOutterHeight * nBytePerBlock;
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
		break;
	}
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	switch (a_nFormat)
	{
	case kTextureFormat_R8_G8_B8_A8_0x07:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormat_Bc1_0x13:
	case kTextureFormat_Bc1_0x1E:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormat_Bc2_0x15:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormat_Bc3_0x17:
	case kTextureFormat_Bc3_0x20:
	case kTextureFormat_Bc3_0x2A:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormat_Bc4_0x19:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormat_Bc5_0x1F:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	}
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	*a_pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBA);
	delete[] pRGBA;
	pvrtexture::Transcode(**a_pPVRTexture, pvrtexture::PVRStandard8PixelType, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB);
	return 0;
}

void encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer)
{
	//PVRTextureHeaderV3 pvrTextureHeaderV3;
	//pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PVRStandard8PixelType.PixelTypeID;
	//pvrTextureHeaderV3.u32Height = a_nHeight;
	//pvrTextureHeaderV3.u32Width = a_nWidth;
	//MetaDataBlock metaDataBlock;
	//metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	//metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	//metaDataBlock.u32DataSize = 3;
	//metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	//metaDataBlock.Data[0] = ePVRTOrientRight;
	//metaDataBlock.Data[1] = ePVRTOrientUp;
	//metaDataBlock.Data[2] = ePVRTOrientIn;
	//pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	//pvrtexture::CPVRTexture* pPVRTexture = nullptr;
	//pvrtexture::CPVRTexture* pPVRTextureAlpha = nullptr;
	//if (a_nFormat != kTextureFormatETC1_A4)
	//{
	//	pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, a_pData);
	//}
	//else
	//{
	//	u8* pRGBAData = new u8[a_nWidth * a_nHeight * 4];
	//	memcpy(pRGBAData, a_pData, a_nWidth * a_nHeight * 4);
	//	u8* pAlphaData = new u8[a_nWidth * a_nHeight * 4];
	//	memcpy(pAlphaData, a_pData, a_nWidth * a_nHeight * 4);
	//	for (n32 i = 0; i < a_nWidth * a_nHeight; i++)
	//	{
	//		pRGBAData[i * 4 + 3] = 0xFF;
	//		pAlphaData[i * 4] = 0;
	//		pAlphaData[i * 4 + 1] = 0;
	//		pAlphaData[i * 4 + 2] = 0;
	//	}
	//	pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBAData);
	//	pPVRTextureAlpha = new pvrtexture::CPVRTexture(pvrTextureHeader, pAlphaData);
	//	delete[] pRGBAData;
	//	delete[] pAlphaData;
	//}
	//if (a_nMipmapLevel != 1)
	//{
	//	pvrtexture::GenerateMIPMaps(*pPVRTexture, pvrtexture::eResizeNearest, a_nMipmapLevel);
	//	if (a_nFormat == kTextureFormatETC1_A4)
	//	{
	//		pvrtexture::GenerateMIPMaps(*pPVRTextureAlpha, pvrtexture::eResizeNearest, a_nMipmapLevel);
	//	}
	//}
	//pvrtexture::uint64 uPixelFormat = 0;
	//pvrtexture::ECompressorQuality eCompressorQuality = pvrtexture::ePVRTCBest;
	//switch (a_nFormat)
	//{
	//case kTextureFormat_R8_G8_B8_A8_Unorm:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
	//	break;
	//case kTextureFormatRGB888:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
	//	break;
	//case kTextureFormatRGBA5551:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
	//	break;
	//case kTextureFormatRGB565:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
	//	break;
	//case kTextureFormatRGBA4444:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
	//	break;
	//case kTextureFormatLA88:
	//	uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatHL8:
	//	uPixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatL8:
	//	uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatA8:
	//	uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatLA44:
	//	uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatL4:
	//	uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatA4:
	//	uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
	//	break;
	//case kTextureFormatETC1:
	//	uPixelFormat = ePVRTPF_ETC1;
	//	eCompressorQuality = pvrtexture::eETCSlowPerceptual;
	//	break;
	//case kTextureFormatETC1_A4:
	//	uPixelFormat = ePVRTPF_ETC1;
	//	eCompressorQuality = pvrtexture::eETCSlowPerceptual;
	//	break;
	//}
	//pvrtexture::Transcode(*pPVRTexture, uPixelFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eCompressorQuality);
	//if (a_nFormat == kTextureFormatETC1_A4)
	//{
	//	pvrtexture::Transcode(*pPVRTextureAlpha, pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, pvrtexture::ePVRTCBest);
	//}
	//n32 nTotalSize = 0;
	//n32 nCurrentSize = 0;
	//for (n32 l = 0; l < a_nMipmapLevel; l++)
	//{
	//	nTotalSize += (a_nWidth >> l) * (a_nHeight >> l) * a_nBPP / 8;
	//}
	//*a_pBuffer = new u8[nTotalSize];
	//for (n32 l = 0; l < a_nMipmapLevel; l++)
	//{
	//	n32 nMipmapWidth = a_nWidth >> l;
	//	n32 nMipmapHeight = a_nHeight >> l;
	//	u8* pRGBA = static_cast<u8*>(pPVRTexture->getDataPtr(l));
	//	u8* pAlpha = nullptr;
	//	if (a_nFormat == kTextureFormatETC1_A4)
	//	{
	//		pAlpha = static_cast<u8*>(pPVRTextureAlpha->getDataPtr(l));
	//	}
	//	switch (a_nFormat)
	//	{
	//	case kTextureFormat_R8_G8_B8_A8_Unorm:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 4];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					for (n32 k = 0; k < 4; k++)
	//					{
	//						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k] = pRGBA[(i * nMipmapWidth + j) * 4 + k];
	//					}
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j++)
	//				{
	//					for (n32 k = 0; k < 4; k++)
	//					{
	//						pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k] = pTemp[(i * 64 + j) * 4 + k];
	//					}
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatRGB888:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 3];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					for (n32 k = 0; k < 3; k++)
	//					{
	//						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k] = pRGBA[(i * nMipmapWidth + j) * 3 + k];
	//					}
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j++)
	//				{
	//					for (n32 k = 0; k < 3; k++)
	//					{
	//						pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k] = pTemp[(i * 64 + j) * 3 + k];
	//					}
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatRGBA5551:
	//	case kTextureFormatRGB565:
	//	case kTextureFormatRGBA4444:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					for (n32 k = 0; k < 2; k++)
	//					{
	//						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
	//					}
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j++)
	//				{
	//					for (n32 k = 0; k < 2; k++)
	//					{
	//						pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k] = pTemp[(i * 64 + j) * 2 + k];
	//					}
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatLA88:
	//	case kTextureFormatHL8:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					for (n32 k = 0; k < 2; k++)
	//					{
	//						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
	//					}
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j++)
	//				{
	//					for (n32 k = 0; k < 2; k++)
	//					{
	//						pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k] = pTemp[(i * 64 + j) * 2 + k];
	//					}
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatL8:
	//	case kTextureFormatA8:
	//	case kTextureFormatLA44:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j++)
	//				{
	//					pMipmapBuffer[i * 64 + s_nDecodeTransByte[j]] = pTemp[i * 64 + j];
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatL4:
	//	case kTextureFormatA4:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
	//			{
	//				for (n32 j = 0; j < 64; j += 2)
	//				{
	//					pMipmapBuffer[i * 32 + s_nDecodeTransByte[j] / 2] = ((pTemp[i * 64 + j] / 0x11) & 0x0F) | ((pTemp[i * 64 + j + 1] / 0x11) << 4 & 0xF0);
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatETC1:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
	//			for (n32 i = 0; i < nMipmapHeight; i += 4)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j += 4)
	//				{
	//					memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
	//			{
	//				for (n32 j = 0; j < 8; j++)
	//				{
	//					pMipmapBuffer[i * 8 + 7 - j] = pTemp[i * 8 + j];
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	case kTextureFormatETC1_A4:
	//		{
	//			u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
	//			for (n32 i = 0; i < nMipmapHeight; i += 4)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j += 4)
	//				{
	//					memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
	//				}
	//			}
	//			u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
	//			{
	//				for (n32 j = 0; j < 8; j++)
	//				{
	//					pMipmapBuffer[8 + i * 16 + 7 - j] = pTemp[i * 8 + j];
	//				}
	//			}
	//			delete[] pTemp;
	//			pTemp = new u8[nMipmapWidth * nMipmapHeight];
	//			for (n32 i = 0; i < nMipmapHeight; i++)
	//			{
	//				for (n32 j = 0; j < nMipmapWidth; j++)
	//				{
	//					pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4] = pAlpha[i * nMipmapWidth + j];
	//				}
	//			}
	//			for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 16; i++)
	//			{
	//				for (n32 j = 0; j < 4; j++)
	//				{
	//					pMipmapBuffer[i * 16 + j * 2] = ((pTemp[i * 16 + j] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 4] / 0x11) << 4 & 0xF0);
	//					pMipmapBuffer[i * 16 + j * 2 + 1] = ((pTemp[i * 16 + j + 8] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 12] / 0x11) << 4 & 0xF0);
	//				}
	//			}
	//			delete[] pTemp;
	//		}
	//		break;
	//	}
	//	nCurrentSize += nMipmapWidth * nMipmapHeight * a_nBPP / 8;
	//}
	//delete pPVRTexture;
	//if (a_nFormat == kTextureFormatETC1_A4)
	//{
	//	delete pPVRTextureAlpha;
	//}
}

int decodeTex(const UChar* a_pTexFileName, const UChar* a_pPngFileNamePrefix)
{
	FILE* fp = UFopen(a_pTexFileName, USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uTexSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	u8* pTex = new u8[uTexSize];
	fread(pTex, 1, uTexSize, fp);
	fclose(fp);
	STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(pTex);
	if (pTexHeader->Signature != SDW_CONVERT_ENDIAN32('TEX\0'))
	{
		delete[] pTex;
		return 1;
	}
	if (pTexHeader->Format != kTextureFormat_R8_G8_B8_A8_0x07 && pTexHeader->Format != kTextureFormat_Bc1_0x13 && pTexHeader->Format != kTextureFormat_Bc2_0x15 && pTexHeader->Format != kTextureFormat_Bc3_0x17 && pTexHeader->Format != kTextureFormat_Bc4_0x19 && pTexHeader->Format != kTextureFormat_Bc1_0x1E && pTexHeader->Format != kTextureFormat_Bc5_0x1F && pTexHeader->Format != kTextureFormat_Bc3_0x20 && pTexHeader->Format != kTextureFormat_Bc3_0x2A)
	{
		delete[] pTex;
		return 1;
	}
	if (pTexHeader->Unknown0xE[0] != 1 || pTexHeader->Unknown0xE[1] != 0)
	{
		delete[] pTex;
		return 1;
	}
	u32 uTotalSize = *reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0));
	if (sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + pTexHeader->MipmapLevel * 4 + (pTexHeader->Count != 1 ? 4 : 0) + uTotalSize != uTexSize)
	{
		delete[] pTex;
		return 1;
	}
	u32 uDataOffset = *reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4);
	if (uDataOffset != 0)
	{
		delete[] pTex;
		return 1;
	}
	u32 uDataSize = uTotalSize;
	vector<UString> vPngFileNamePrefix = SplitOf<UString>(a_pPngFileNamePrefix, USTR("/\\"));
	if (pTexHeader->Count != 1)
	{
		uDataSize = *reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + pTexHeader->MipmapLevel * 4);
		if (pTexHeader->Count * uDataSize != uTotalSize)
		{
			delete[] pTex;
			return 1;
		}
		UMkdir(Format(USTR("%") PRIUS USTR(".dir"), a_pPngFileNamePrefix).c_str());
	}
	n32 nWidth = getBoundWidth(pTexHeader->Width, pTexHeader->Format);
	n32 nHeight = getBoundHeight(pTexHeader->Height, pTexHeader->Format);
	n32 nBlockHeight = getBlockHeight(nHeight, pTexHeader->Format);
	for (n32 i = 0; i < pTexHeader->Count; i++)
	{
		u32 uOffset = sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + 4 + pTexHeader->MipmapLevel * 4 + (pTexHeader->Count != 1 ? 4 : 0) + i * uDataSize + uDataOffset;
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		if (decode(pTex + uOffset, nWidth, nHeight, pTexHeader->Format, nBlockHeight, &pPVRTexture) == 0)
		{
			UString sPngFileName;
			if (pTexHeader->Count == 1)
			{
				sPngFileName = Format(USTR("%") PRIUS USTR(".png"), a_pPngFileNamePrefix);
			}
			else
			{
				sPngFileName = Format(USTR("%") PRIUS USTR(".dir/%") PRIUS USTR("%d.png"), a_pPngFileNamePrefix, vPngFileNamePrefix.back().c_str(), i);
			}
			FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("wb"), false);
			if (fpSub == nullptr)
			{
				delete pPVRTexture;
				delete[] pTex;
				return 1;
			}
			png_structp pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
			if (pPng == nullptr)
			{
				fclose(fpSub);
				delete pPVRTexture;
				delete[] pTex;
				return 1;
			}
			png_infop pInfo = png_create_info_struct(pPng);
			if (pInfo == nullptr)
			{
				png_destroy_write_struct(&pPng, nullptr);
				fclose(fpSub);
				delete pPVRTexture;
				delete[] pTex;
				return 1;
			}
			if (setjmp(png_jmpbuf(pPng)) != 0)
			{
				png_destroy_write_struct(&pPng, &pInfo);
				fclose(fpSub);
				delete pPVRTexture;
				delete[] pTex;
				return 1;
			}
			png_init_io(pPng, fpSub);
			png_set_IHDR(pPng, pInfo, pTexHeader->Width, pTexHeader->Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			u8* pData = static_cast<u8*>(pPVRTexture->getDataPtr());
			png_bytepp pRowPointers = new png_bytep[pTexHeader->Height];
			for (u32 j = 0; j < pTexHeader->Height; j++)
			{
				pRowPointers[j] = pData + j * nWidth * 4;
			}
			png_set_rows(pPng, pInfo, pRowPointers);
			png_write_png(pPng, pInfo, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&pPng, &pInfo);
			delete[] pRowPointers;
			fclose(fpSub);
			delete pPVRTexture;
		}
		else
		{
			delete pPVRTexture;
			delete[] pTex;
			return 1;
		}
	}
	delete[] pTex;
	return 0;
}

int encodeTex(const UChar* a_pTexFileName, const UChar* a_pPngFileNamePrefix)
{
	//FILE* fp = UFopen(a_pTexFileName, USTR("rb"), false);
	//if (fp == nullptr)
	//{
	//	return 1;
	//}
	//fseek(fp, 0, SEEK_END);
	//u32 uTexSize = ftell(fp);
	//fseek(fp, 0, SEEK_SET);
	//u8* pTex = new u8[uTexSize];
	//fread(pTex, 1, uTexSize, fp);
	//fclose(fp);
	//STexHeader* pTexHeader = reinterpret_cast<STexHeader*>(pTex);
	//if (pTexHeader->Signature != SDW_CONVERT_ENDIAN32('TEX\0'))
	//{
	//	delete[] pTex;
	//	return 1;
	//}
	//if (pTexHeader->Format != kTextureFormat_R8_G8_B8_A8_Unorm && pTexHeader->Format != kTextureFormatRGB888 && pTexHeader->Format != kTextureFormatRGBA5551 && pTexHeader->Format != kTextureFormatRGB565 && pTexHeader->Format != kTextureFormatRGBA4444 && pTexHeader->Format != kTextureFormatLA88 && pTexHeader->Format != kTextureFormatL8 && pTexHeader->Format != kTextureFormatA8 && pTexHeader->Format != kTextureFormatLA44 && pTexHeader->Format != kTextureFormatL4 && pTexHeader->Format != kTextureFormatA4 && pTexHeader->Format != kTextureFormatETC1 && pTexHeader->Format != kTextureFormatETC1_A4)
	//{
	//	delete[] pTex;
	//	return 1;
	//}
	//if (pTexHeader->Unknown0xE[0] != 1 || pTexHeader->Unknown0xE[1] != 0)
	//{
	//	delete[] pTex;
	//	return 1;
	//}
	//u32 uBackupTexSize = uTexSize;
	//u8* pBackupTex = new u8[uBackupTexSize];
	//memcpy(pBackupTex, pTex, uBackupTexSize);
	//fp = UFopen(a_pTexFileName, USTR("wb"), false);
	//if (fp == nullptr)
	//{
	//	delete[] pBackupTex;
	//	delete[] pTex;
	//	return 1;
	//}
	//fwrite(pTex, 1, sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4, fp);
	//uTexSize = ftell(fp);
	//vector<UString> vPngFileNamePrefix = SplitOf<UString>(a_pPngFileNamePrefix, USTR("/\\"));
	//u32 uWidthOld = pTexHeader->Width;
	//u32 uHeightOld = pTexHeader->Height;
	//for (n32 i = 0; i < pTexHeader->Count; i++)
	//{
	//	UString sPngFileName;
	//	if (pTexHeader->Count == 1)
	//	{
	//		sPngFileName = Format(USTR("%") PRIUS USTR(".png"), a_pPngFileNamePrefix);
	//	}
	//	else
	//	{
	//		sPngFileName = Format(USTR("%") PRIUS USTR(".dir/%") PRIUS USTR("%d.png"), a_pPngFileNamePrefix, vPngFileNamePrefix.back().c_str(), i);
	//	}
	//	FILE* fpSub = UFopen(sPngFileName.c_str(), USTR("rb"), false);
	//	if (fpSub == nullptr)
	//	{
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	//	if (pPng == nullptr)
	//	{
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	png_infop pInfo = png_create_info_struct(pPng);
	//	if (pInfo == nullptr)
	//	{
	//		png_destroy_read_struct(&pPng, nullptr, nullptr);
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	png_infop pEndInfo = png_create_info_struct(pPng);
	//	if (pEndInfo == nullptr)
	//	{
	//		png_destroy_read_struct(&pPng, &pInfo, nullptr);
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	if (setjmp(png_jmpbuf(pPng)) != 0)
	//	{
	//		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	png_init_io(pPng, fpSub);
	//	png_read_info(pPng, pInfo);
	//	u32 uPngWidth = png_get_image_width(pPng, pInfo);
	//	u32 uPngHeight = png_get_image_height(pPng, pInfo);
	//	n32 nBitDepth = png_get_bit_depth(pPng, pInfo);
	//	if (nBitDepth != 8)
	//	{
	//		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	n32 nColorType = png_get_color_type(pPng, pInfo);
	//	if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
	//	{
	//		png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
	//		fclose(fpSub);
	//		fseek(fp, 0, SEEK_SET);
	//		fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//		Chsize(Fileno(fp), uBackupTexSize);
	//		fclose(fp);
	//		delete[] pBackupTex;
	//		delete[] pTex;
	//		return 1;
	//	}
	//	u8* pData = new u8[uPngWidth * uPngHeight * 4];
	//	png_bytepp pRowPointers = new png_bytep[uPngHeight];
	//	for (u32 j = 0; j < uPngHeight; j++)
	//	{
	//		pRowPointers[j] = pData + j * uPngWidth * 4;
	//	}
	//	png_read_image(pPng, pRowPointers);
	//	png_destroy_read_struct(&pPng, &pInfo, &pEndInfo);
	//	delete[] pRowPointers;
	//	fclose(fpSub);
	//	u32 uOffset = sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4 + *reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + i * pTexHeader->MipmapLevel * 4);
	//	pvrtexture::CPVRTexture* pPVRTexture = nullptr;
	//	bool bSame = false;
	//	if (uPngWidth != pTexHeader->Width || uPngHeight != pTexHeader->Height)
	//	{
	//		if (i == 0)
	//		{
	//			pTexHeader->Width = uPngWidth;
	//			pTexHeader->Height = uPngHeight;
	//		}
	//		else
	//		{
	//			delete[] pData;
	//			fseek(fp, 0, SEEK_SET);
	//			fwrite(pBackupTex, 1, uBackupTexSize, fp);
	//			Chsize(Fileno(fp), uBackupTexSize);
	//			fclose(fp);
	//			delete[] pBackupTex;
	//			delete[] pTex;
	//			return 1;
	//		}
	//	}
	//	else if (pTexHeader->Width == uWidthOld && pTexHeader->Height == uHeightOld && decode(pTex + uOffset, pTexHeader->Width, pTexHeader->Height, pTexHeader->Format, &pPVRTexture) == 0 && memcmp(pPVRTexture->getDataPtr(), pData, pTexHeader->Width * pTexHeader->Height * 4) == 0)
	//	{
	//		bSame = true;
	//	}
	//	delete pPVRTexture;
	//	n32 nBPP = 0;
	//	switch (pTexHeader->Format)
	//	{
	//	case kTextureFormat_R8_G8_B8_A8_Unorm:
	//		nBPP = 32;
	//		break;
	//	case kTextureFormatRGB888:
	//		nBPP = 24;
	//		break;
	//	case kTextureFormatRGBA5551:
	//	case kTextureFormatRGB565:
	//	case kTextureFormatRGBA4444:
	//	case kTextureFormatLA88:
	//		nBPP = 16;
	//		break;
	//	case kTextureFormatL8:
	//	case kTextureFormatA8:
	//	case kTextureFormatLA44:
	//	case kTextureFormatETC1_A4:
	//		nBPP = 8;
	//		break;
	//	case kTextureFormatL4:
	//	case kTextureFormatA4:
	//	case kTextureFormatETC1:
	//		nBPP = 4;
	//		break;
	//	}
	//	if (bSame)
	//	{
	//		for (u32 j = 0; j < pTexHeader->MipmapLevel; j++)
	//		{
	//			n32 nMipmapWidth = pTexHeader->Width >> j;
	//			n32 nMipmapHeight = pTexHeader->Height >> j;
	//			uOffset = sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4 + *reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + (i * pTexHeader->MipmapLevel + j) * 4);
	//			*reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + (i * pTexHeader->MipmapLevel + j) * 4) = uTexSize - (sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4);
	//			fwrite(pTex + uOffset, 1, nMipmapWidth * nMipmapHeight * nBPP / 8, fp);
	//			uTexSize = ftell(fp);
	//		}
	//	}
	//	else
	//	{
	//		u8* pBuffer = nullptr;
	//		encode(pData, uPngWidth, uPngHeight, pTexHeader->Format, pTexHeader->MipmapLevel, nBPP, &pBuffer);
	//		n32 nCurrentSize = 0;
	//		for (u32 j = 0; j < pTexHeader->MipmapLevel; j++)
	//		{
	//			n32 nMipmapWidth = pTexHeader->Width >> j;
	//			n32 nMipmapHeight = pTexHeader->Height >> j;
	//			n32 nLevelSize = nMipmapWidth * nMipmapHeight * nBPP / 8;
	//			*reinterpret_cast<u32*>(pTex + sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + (i * pTexHeader->MipmapLevel + j) * 4) = uTexSize - (sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4);
	//			fwrite(pBuffer + nCurrentSize, 1, nLevelSize, fp);
	//			nCurrentSize += nLevelSize;
	//			uTexSize = ftell(fp);
	//		}
	//		delete[] pBuffer;
	//	}
	//	delete[] pData;
	//}
	//fseek(fp, 0, SEEK_SET);
	//fwrite(pTex, 1, sizeof(STexHeader) + (pTexHeader->Count == 6 ? 0x6C : 0) + pTexHeader->Count * pTexHeader->MipmapLevel * 4, fp);
	//fclose(fp);
	//delete[] pBackupTex;
	//delete[] pTex;
	return 0;
}

int UMain(int argc, UChar* argv[])
{
	if (argc != 4)
	{
		return 1;
	}
	if (UCslen(argv[1]) == 1)
	{
		switch (*argv[1])
		{
		case USTR('D'):
		case USTR('d'):
			return decodeTex(argv[2], argv[3]);
		case USTR('E'):
		case USTR('e'):
			return encodeTex(argv[2], argv[3]);
		default:
			break;
		}
	}
	return 1;
}

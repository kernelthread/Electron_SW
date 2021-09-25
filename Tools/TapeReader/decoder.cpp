/*
* BBC format cassette file decoder
*
* Copyright 2021, Dennis May
* First Published 2021
*
* This file is part of Miscellaneous Electron Software.
*
* Miscellaneous Electron Software is free software: you can redistribute it
* and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Miscellaneous Electron Software is distributed in the hope that it will be
* useful, but WITHOUT ANY WARRANTY* without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* Miscellaneous Electron Software.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "decoder.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

uint32_t CDecoder::Crc(const uint8_t* aData, uint32_t aCount, uint32_t aCrc)
{
    const uint32_t poly = 0x11021U;
    int i;

    for (; aCount>0; aCount--)              /* Step through bytes in memory */
    {
        uint32_t x = *aData++;
        aCrc ^= (x << 8);					/* XOR into CRC top byte*/
        for (i=0; i<8; i++)                 /* Prepare to rotate 8 bits */
        {
            if (aCrc & 0x8000)              /* b15 is set... */
                aCrc = (aCrc << 1) ^ poly;	/* rotate and XOR with XMODEM polynomic */
            else                            /* b15 is clear... */
                aCrc <<= 1;                 /* just rotate */
        }                                   /* Loop for 8 bits */
    }                                       /* Loop until num=0 */
    return aCrc;                            /* Return updated CRC */
}

CDecoder::CDecoder()
:	iLdr(0),
	iState(ELeader),
	iBitCount(0),
	iBlockNum(0),
	iIndex(0),
	iIndex2(0),
	iByte(0),
	iShift(0),
	iFileOpen(false)
{
	InitBlockHeader(iFirstBlock);
	InitBlockHeader(iCurrentBlock);
}

CDecoder::~CDecoder()
{
}

void CDecoder::BeginLeaderSearch(bool aFirstBlock)
{
	iLdr = 0;
	iState = ELeader;
	iIndex = 0;
	iIndex2 = 0;
	iByte = 0;
	iShift = 0;
	if (aFirstBlock)
	{
		iBlockNum = 0;
	}
}

void CDecoder::Bit(uint32_t aBit)
{
	uint32_t bit = aBit ? 1 : 0;
	uint32_t err = 0;
	++iBitCount;
	if (iState == ELeader)
	{
		iLdr <<= 1;
		iLdr |= bit;

		// 1 1 0 0 1 0 1 0 1 0 0 1
		if (iLdr == 0xFFFFFFFFFFFFFCA9ULL)
		{
//			printf("Leader detected at %u\n", iBitCount);
			iState = EHeaderName;
			iIndex = 0;
			iIndex2 = 0;
			iByte = 0;
			iShift = 0;
		}
		return;
	}
	if (iShift==0)
	{
		if (bit==0)
		{
			// start bit received
		}
		else
		{
			// extra stop bits/idle time between bytes
			return;
		}
	}
	iByte |= (bit << iShift);
	++iShift;
	if (iShift < 10)
		return;

//	printf("%1x %02x %1x\n", iByte>>9, (iByte>>1)&0xff, iByte&1);
	if ((iByte & 0x201U) != 0x200U)
	{
		// start or stop bit corrupted
		printf("%1x %02x %1x (%u)\n", iByte>>9, (iByte>>1)&0xff, iByte&1, iBitCount);
	}
	iShift = 0;
	iBuffer[iIndex++] = (iByte >> 1) & 0xFFU;
	iByte = 0;

	switch (iState)
	{
	case EHeaderName:
		if (iIndex == MAX_NAME_LENGTH+1 || iBuffer[iIndex-1]==0)
		{
			iState = EHeaderRest;
			iIndex2 = iIndex + HEADER_LENGTH_2;
//			printf("Header name %s\n", iBuffer);
		}
		break;
	case EHeaderRest:
		if (iIndex == iIndex2)
		{
			iState = EData;
			iIndex = 0;
			if (iBlockNum > 0)
			{
				// expecting 2nd or later block of file
				err = InitBlockHeader(iCurrentBlock, iBuffer, &iCurrentBlock);
				if (err == EUnexpectedBlock)
				{
					// name is different
					if (iCurrentBlock.iBlockNum == 0)
					{
						// first block of different file
						// old file has been truncated, so close it and open new file
						Eof();
						iFileOpen = false;
						iFirstBlock = iCurrentBlock;
						File(&iFirstBlock);
						iFileOpen = true;
						iBlockNum = 0;
						err = 0;
					}
				}
				if (iBlockNum != iCurrentBlock.iBlockNum)
				{
					err |= EUnexpectedBlock;
				}
				if (err != 0)
				{
					printf("BlockNum %d err %08x\n", iBlockNum, err);
					BeginLeaderSearch(false);
				}
			}
			else
			{
				// expecting first block of file
				err = InitBlockHeader(iCurrentBlock, iBuffer, 0);
				if (iBlockNum != iCurrentBlock.iBlockNum)
				{
					err |= EUnexpectedBlock;
				}
				if (err == 0)
				{
					iFirstBlock = iCurrentBlock;
					File(&iFirstBlock);
					iFileOpen = true;
				}
				else
				{
					printf("BlockNum %d err %08x\n", iBlockNum, err);
					BeginLeaderSearch(false);
				}
			}
		}
		break;
	case EData:
		if (iIndex == iCurrentBlock.iBlockLen + 2)
		{
			uint32_t crc = (iBuffer[iCurrentBlock.iBlockLen] << 8) | iBuffer[iCurrentBlock.iBlockLen + 1];
			uint32_t crcx = Crc(iBuffer, iCurrentBlock.iBlockLen, 0);
			if (crc != crcx)
			{
				err |= EInvalidDataCrc;
			}
			if (err == 0)
			{
				Block(&iCurrentBlock, iBuffer);
			}
			if (iCurrentBlock.iBlockFlag & BLOCK_FLAG_FINAL)
			{
				Eof();
				iFileOpen = false;
				BeginLeaderSearch(true);
			}
			else
			{
				++iBlockNum;
				BeginLeaderSearch(false);
			}
		}
		break;
	default:
		assert(false);
	}
}

void CDecoder::InitBlockHeader(SBlockHeader& aHdr)
{
	memset(aHdr.iName, 0, sizeof(aHdr.iName));
	aHdr.iLoadAddr = 0;
	aHdr.iExecAddr = 0;
	aHdr.iBlockNum = 0;
	aHdr.iBlockLen = 0;
	aHdr.iNextFile = 0;
	aHdr.iBlockFlag = 0;
}

uint32_t CDecoder::InitBlockHeader(SBlockHeader& aHdr, const uint8_t* aData, const SBlockHeader* aPrevBlock)
{
	uint32_t err = 0;
	uint32_t i;
	SBlockHeader prev;
	if (aPrevBlock)
		prev = *aPrevBlock;
	uint8_t c = 1U;
	const uint8_t* s = aData;
	memset(aHdr.iName, 0, sizeof(aHdr.iName));
	for (i=0; i<=MAX_NAME_LENGTH; ++i)
	{
		c = *s++;
		aHdr.iName[i] = c;
		if (c == 0)
		{
			break;
		}
		if (c<0x20U || c>0x7EU)
		{
			err |= EInvalidName;
		}
	}
	if (i==0 || c!=0)
	{
		err |= EInvalidName;
		aHdr.iName[MAX_NAME_LENGTH] = 0;
	}
	aHdr.iLoadAddr = (s[0]) | (s[1]<<8) | (s[2]<<16) | (s[3]<<24);
	s += 4;
	aHdr.iExecAddr = (s[0]) | (s[1]<<8) | (s[2]<<16) | (s[3]<<24);
	s += 4;
	aHdr.iBlockNum = (s[0]) | (s[1]<<8);
	s += 2;
	aHdr.iBlockLen = (s[0]) | (s[1]<<8);
	s += 2;
	aHdr.iBlockFlag = *s++;
	aHdr.iNextFile = (s[0]) | (s[1]<<8) | (s[2]<<16) | (s[3]<<24);
	s += 4;
	uint32_t crc = (s[1]) | (s[0]<<8);
	uint32_t crcx = Crc(aData, s - aData, 0);
//	printf("LA %08x XA %08x BN %04x BL %04x BF %02x NF %08x CRC %04x CRCX %04x\n",
//		aHdr.iLoadAddr, aHdr.iExecAddr, aHdr.iBlockNum, aHdr.iBlockLen, aHdr.iBlockFlag, aHdr.iNextFile, crc, crcx);
	if (aHdr.iBlockLen > 256)
	{
		err |= EInvalidLength;
	}
	if (aHdr.iBlockFlag & ~(BLOCK_FLAG_LOCKED|BLOCK_FLAG_EMPTY|BLOCK_FLAG_FINAL))
	{
		err |= EInvalidFlag;
	}
	if (aHdr.iBlockFlag & BLOCK_FLAG_EMPTY)
	{
		if (aHdr.iBlockLen > 0)
		{
			err |= EInvalidLength;
		}
	}
	if (!(aHdr.iBlockFlag & BLOCK_FLAG_FINAL))
	{
		if (aHdr.iBlockLen < MAX_BLOCK_LENGTH)
		{
			err |= EInvalidLength;
		}
	}
	if (crc != crcx)
	{
		err |= EInvalidHdrCrc;
	}
	if (aPrevBlock)
	{
		if (strcmp(aHdr.iName, prev.iName) != 0)
		{
			err |= EUnexpectedBlock;
		}
		else if (aHdr.iBlockNum > prev.iBlockNum + 1)
		{
			err |= ESkippedBlock;
		}
		else if (aHdr.iBlockNum > prev.iBlockNum + 1)
		{
			err |= ERepeatBlock;
		}
	}
	else
	{
		if (aHdr.iBlockNum > 0)
		{
			err |= ESkippedBlock;
		}
	}
	return err;
}

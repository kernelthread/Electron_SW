/*
* Header file for BBC format cassette file decoder
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

#include <stdint.h>

#define MAX_NAME_LENGTH		(10)
#define	HEADER_LENGTH_2		(4+4+2+2+1+4+2)		// length of header after name
#define	MAX_BLOCK_LENGTH	(256)

#define	BLOCK_FLAG_LOCKED	(1<<0)
#define	BLOCK_FLAG_EMPTY	(1<<6)
#define	BLOCK_FLAG_FINAL	(1<<7)

struct SBlockHeader
{
	char		iName[MAX_NAME_LENGTH+1];
	uint32_t	iLoadAddr;
	uint32_t	iExecAddr;
	uint16_t	iBlockNum;
	uint16_t	iBlockLen;
	uint32_t	iNextFile;
	uint8_t		iBlockFlag;
};

class CDecoder
{
public:
	CDecoder();
	virtual ~CDecoder();
	void Bit(uint32_t aBit);
	virtual void Block(const SBlockHeader* aHdr, const uint8_t* aData)=0;
	virtual void File(const SBlockHeader* aHdr)=0;
	virtual void Eof()=0;
public:
	enum TError
	{
		EInvalidName = (1U<<0),
		EInvalidLength = (1U<<1),
		EInvalidFlag = (1U<<2),
		EInvalidHdrCrc = (1U<<3),
		EInvalidDataCrc = (1U<<4),
		EUnexpectedBlock = (1U<<5),
		ESkippedBlock = (1U<<6),
		ERepeatBlock = (1U<<7),
	};
private:
	static void InitBlockHeader(SBlockHeader& aHdr);
	static uint32_t InitBlockHeader(SBlockHeader& aHdr, const uint8_t* aData, const SBlockHeader* aPrevBlock);
	static uint32_t Crc(const uint8_t* aData, uint32_t aCount, uint32_t aCrc);
	void BeginLeaderSearch(bool aFirstBlock);
private:
	enum TState
	{
		ELeader = 0,
		EHeaderName = 1,
		EHeaderRest = 2,
		EData = 3,
	};
private:
	uint64_t		iLdr;
	TState			iState;
	uint32_t		iBitCount;
	uint32_t		iBlockNum;
	SBlockHeader	iFirstBlock;
	SBlockHeader	iCurrentBlock;
	uint32_t		iIndex;
	uint32_t		iIndex2;
	uint32_t		iByte;
	uint32_t		iShift;
	bool			iFileOpen;
	uint8_t			iBuffer[MAX_BLOCK_LENGTH+2];
};

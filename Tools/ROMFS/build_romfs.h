/*
* Header file for Acorn MOS ROM builder tool
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

#define	HDR_SYNC			(0x2A)
#define CONTINUATION_HDR	(0x23)
#define END_OF_ROM			(0x2B)

#define MAX_FILES			256

struct SBlockHeader
{
	char		iName[MAX_NAME_LENGTH+1];
	uint32_t	iLoadAddr;
	uint32_t	iExecAddr;
	uint16_t	iBlockNum;
	uint16_t	iBlockLen;
	uint32_t	iNextFile;
	uint8_t		iBlockFlag;
	uint8_t		iNameLen;
};

class CRomFsFile
{
public:
	static CRomFsFile* New(const char* aFileName, uint32_t aBase);
	static CRomFsFile* NewTitle(const char* aTitle, uint32_t aBase);
	virtual ~CRomFsFile();
	inline uint32_t FsLen() const { return iFsLen; }
	inline uint32_t NextBase() const { return iBase+iFsLen; }
	void WriteFsData(FILE* aOut);
	int FindFileName(const char* aName);
	void AddFileName(const char* aName);
private:
	CRomFsFile(uint32_t aBase);
	bool Construct(const char* aFileName);
	bool ConstructTitle(const char* aTitle);
	static uint32_t Crc(const uint8_t* aData, uint32_t aCount, uint32_t aCrc);
private:
	SBlockHeader iHdr;
	uint32_t iBase;
	uint32_t iRawLen;
	uint32_t iFsLen;
	uint32_t iNBlocks;
	uint8_t* iRawData;
	uint8_t* iFsData;
private:
	static uint32_t NumFiles;
	static const char* FileNames[MAX_FILES];
};

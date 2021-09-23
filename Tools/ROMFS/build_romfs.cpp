/*
* Acorn MOS ROM builder tool
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "build_romfs.h"

#ifdef _MSC_VER
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#define PLACE16LE(d,x)	\
	do	{ \
		(d)[0] = (uint8_t)(((x)>>0)&0xFF); \
		(d)[1] = (uint8_t)(((x)>>8)&0xFF); \
		(d) += 2; \
	} while(0)

#define PLACE16BE(d,x)	\
	do	{ \
		(d)[1] = (uint8_t)(((x)>>0)&0xFF); \
		(d)[0] = (uint8_t)(((x)>>8)&0xFF); \
		(d) += 2; \
	} while(0)

#define PLACE32LE(d,x)	\
	do	{ \
		(d)[0] = (uint8_t)(((x)>>0)&0xFF); \
		(d)[1] = (uint8_t)(((x)>>8)&0xFF); \
		(d)[2] = (uint8_t)(((x)>>16)&0xFF); \
		(d)[3] = (uint8_t)(((x)>>24)&0xFF); \
		(d) += 4; \
	} while(0)

uint32_t CRomFsFile::NumFiles = 0;
const char* CRomFsFile::FileNames[MAX_FILES] = {0};

uint32_t CRomFsFile::Crc(const uint8_t* aData, uint32_t aCount, uint32_t aCrc)
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

CRomFsFile* CRomFsFile::New(const char* aFileName, uint32_t aBase)
{
	CRomFsFile* p = new CRomFsFile(aBase);
	if (p)
	{
		bool ok = p->Construct(aFileName);
		if (!ok)
		{
			delete p;
			p = 0;
		}
	}
	return p;
}

CRomFsFile* CRomFsFile::NewTitle(const char* aTitle, uint32_t aBase)
{
	CRomFsFile* p = new CRomFsFile(aBase);
	if (p)
	{
		bool ok = p->ConstructTitle(aTitle);
		if (!ok)
		{
			delete p;
			p = 0;
		}
	}
	return p;
}

CRomFsFile::CRomFsFile(uint32_t aBase)
:	iBase(aBase),
	iRawLen(0),
	iFsLen(0),
	iNBlocks(0),
	iRawData(0),
	iFsData(0)
{
	memset(&iHdr, 0, sizeof(iHdr));
	iHdr.iLoadAddr = 0;
	iHdr.iExecAddr = 0;
}

CRomFsFile::~CRomFsFile()
{
	delete[] iRawData;
	delete[] iFsData;
}

bool CRomFsFile::Construct(const char* aFileName)
{
	printf("Processing file %s\n", aFileName);
	char target_name[11] = {0};
	const char* hfn = aFileName;
	int nl = strlen(aFileName);
	int i;
	int ep = -1;
	for (i=0; i<nl; ++i)
	{
		char c = aFileName[i];
		if (c == 0x20)
			break;
		if (c == '=')
		{
			ep = i;
			break;
		}
	}
	if (ep == 0)
	{
		// target name can't be empty, so assume host name only
		ep = -1;
	}
	if (ep > 0)
	{
		// target name specified
		for (i=0; i<ep; ++i)
		{
			if (i == MAX_NAME_LENGTH)
			{
				fprintf(stderr, "WARNING: `%s` target name too long, truncating to %d characters\n", aFileName, MAX_NAME_LENGTH);
				break;
			}
			char c = aFileName[i];
			if (c<0x20 || c>0x7E)
			{
				fprintf(stderr, "ERROR: Target filename contains invalid character 0x%02x\n", c);
				exit(1);
			}
			iHdr.iName[i] = c;
		}
		nl = i;
		iHdr.iName[i] = 0;
		hfn = aFileName + ep + 1;
		if (*hfn == 0)
		{
			fprintf(stderr, "ERROR: Host filename not specified (%s)\n", aFileName);
			exit(1);
		}
	}
	else
	{
		// only host name specified, derive target name
		bool warn = false;
		int j = nl;
		for (;;)
		{
			char c = aFileName[--j];
			if (c == '/' || c == '\\')
				break;
			if (j == -1)
				break;
		}
		if (j == nl-1)
		{
			fprintf(stderr, "ERROR: Host filename ends in path separator\n");
			exit(1);
		}
		for (i=0; i<nl-1-j; ++i)
		{
			char c = aFileName[i+j+1];
			if (i == MAX_NAME_LENGTH)
			{
				warn = true;
				break;
			}
			if (c>0x20 && c<=0x7E)
				target_name[i] = c;
			else
			{
				target_name[i] = '_';
				warn = true;
			}
		}
		if (warn)
		{
			fprintf(stderr, "WARNING: Target name %s derived from %s\n", target_name, aFileName);
		}
		nl = i;
		memcpy(iHdr.iName, target_name, nl);
	}
	iHdr.iNameLen = (uint8_t)nl;
	FILE* f = fopen(hfn, "rb");
	if (!f)
	{
		fprintf(stderr, "ERROR: Can't open file %s for read\n", hfn);
		return false;
	}
	fseek(f, 0, SEEK_END);
	iRawLen = (uint32_t)ftell(f);
	fseek(f, 0, SEEK_SET);
	printf("Host file %s length 0x%04x\n", hfn, iRawLen);
	iRawData = new uint8_t[iRawLen];
	if (!iRawData)
	{
		fprintf(stderr, "ERROR: Failed to allocate iRawData\n");
		return false;
	}
	if (iRawLen == 0)
	{
		fprintf(stderr, "ERROR: File %s is empty\n", aFileName);
		return false;
	}
	size_t rsz = fread(iRawData, 1, iRawLen, f);
	if (rsz != iRawLen)
	{
		fprintf(stderr, "ERROR: Problem reading file %s (expected %04x got %04x)\n", aFileName, iRawLen, (uint32_t)rsz);
		return false;
	}
	iNBlocks = (iRawLen + MAX_BLOCK_LENGTH - 1) / MAX_BLOCK_LENGTH;

	// First + last block contain header, intermediate blocks have 1 byte continuation header
	uint32_t hdrLen = 1 + HEADER_LENGTH_2 + iHdr.iNameLen + 1;	// 0x2A + name + name terminator + rest of header
	iFsLen = iRawLen + 2 * iNBlocks;	// 2 byte data CRC on each block
	if (iNBlocks == 1)
	{
		// one block header
		iFsLen += hdrLen;
	}
	else
	{
		// one block header on each of first and last blocks, single byte continuation header on each other block
		iFsLen += 2*hdrLen + (iNBlocks-2);
	}
	iFsData = new uint8_t[iFsLen];
	if (!iFsData)
	{
		fprintf(stderr, "ERROR: Failed to allocate iFsData\n");
		return false;
	}
	iHdr.iNextFile = iBase + iFsLen;

	const uint8_t* s = iRawData;
	uint8_t* d = iFsData;
	uint32_t remain = iRawLen;
	uint32_t bn;
	printf("Target name %s\n", iHdr.iName);
	for (bn=0; bn<iNBlocks; ++bn)
	{
		uint32_t bl = (remain >= MAX_BLOCK_LENGTH) ? MAX_BLOCK_LENGTH : remain;
		bool fullHdr = (bn==0 || bn==iNBlocks-1);
		if (fullHdr)
		{
			*d++ = HDR_SYNC;
			const uint8_t* bh = d;
			iHdr.iBlockNum = bn;
			iHdr.iBlockLen = bl;
			iHdr.iBlockFlag = (bn == iNBlocks-1) ? BLOCK_FLAG_FINAL : 0;
			memcpy(d, iHdr.iName, iHdr.iNameLen+1);
			if (bn == 0)
				AddFileName((const char*)d);
			d += iHdr.iNameLen+1;
			PLACE32LE(d, iHdr.iLoadAddr);
			PLACE32LE(d, iHdr.iExecAddr);
			PLACE16LE(d, iHdr.iBlockNum);
			PLACE16LE(d, iHdr.iBlockLen);
			*d++ = (uint8_t)iHdr.iBlockFlag;
			PLACE32LE(d, iHdr.iNextFile);
			uint32_t crc = Crc(bh, hdrLen-3, 0);	// CRC doesn't include initial 0x2A or CRC itself
			PLACE16BE(d, crc);
		}
		else
		{
			*d++ = CONTINUATION_HDR;
		}
		memcpy(d, s, bl);
		d += bl;
		uint32_t crc = Crc(s, bl, 0);
		PLACE16BE(d, crc);
		remain -= bl;
		s += bl;
	}
	return true;
}

bool CRomFsFile::ConstructTitle(const char* aTitle)
{
	printf("Setting ROM title %s\n", aTitle);
	int i;
	int nl = strlen(aTitle);
	if (nl > MAX_NAME_LENGTH)
	{
		fprintf(stderr, "WARNING: ROM title too long, truncating to %d characters\n", MAX_NAME_LENGTH);
		nl = MAX_NAME_LENGTH;
	}
	for (i=0; i<nl; ++i)
	{
		char c = aTitle[i];
		if (c<0x20 || c>0x7E)
		{
			fprintf(stderr, "ERROR: ROM title contains invalid character 0x%02x\n", c);
			exit(1);
		}
	}
	memcpy(iHdr.iName, aTitle, nl);
	iHdr.iNameLen = (uint8_t)nl;
	iRawLen = 0;
	iNBlocks = 0;

	// Just header, no data
	uint32_t hdrLen = 1 + HEADER_LENGTH_2 + iHdr.iNameLen + 1;	// 0x2A + name + name terminator + rest of header
	iFsLen = hdrLen;
	iFsData = new uint8_t[iFsLen];
	if (!iFsData)
	{
		fprintf(stderr, "ERROR: Failed to allocate iFsData\n");
		return false;
	}
	iHdr.iNextFile = iBase + iFsLen;
	uint8_t* d = iFsData;

	*d++ = HDR_SYNC;
	const uint8_t* bh = d;
	iHdr.iBlockFlag = BLOCK_FLAG_FINAL | BLOCK_FLAG_EMPTY;
	memcpy(d, iHdr.iName, iHdr.iNameLen+1);
	AddFileName((const char*)d);
	d += iHdr.iNameLen+1;
	PLACE32LE(d, 0);	// load
	PLACE32LE(d, 0);	// exec
	PLACE16LE(d, 0);	// block number
	PLACE16LE(d, 0);	// block length
	*d++ = (uint8_t)iHdr.iBlockFlag;
	PLACE32LE(d, iHdr.iNextFile);
	uint32_t crc = Crc(bh, hdrLen-3, 0);	// CRC doesn't include initial 0x2A or CRC itself
	PLACE16BE(d, crc);
	return true;
}

void CRomFsFile::WriteFsData(FILE* aOut)
{
	size_t wsz = fwrite(iFsData, 1, iFsLen, aOut);
	if (wsz != iFsLen)
	{
		fprintf(stderr, "ERROR: Problem writing output file\n");
	}
}

int CRomFsFile::FindFileName(const char* aName)
{
	int i;
	for (i=0; i<(int)NumFiles; ++i)
	{
		if (strcasecmp(FileNames[i], aName) == 0)
		{
			return i;
		}
	}
	return -1;
}

void CRomFsFile::AddFileName(const char* aName)
{
	int i = FindFileName(aName);
	if (i >= 0)
	{
		fprintf(stderr, "ERROR: Duplicate target filename (%s)\n", aName);
		exit(1);
	}
	FileNames[NumFiles] = strdup(aName);
	++NumFiles;
}

void write_terminator(FILE* aOut)
{
	uint8_t c = END_OF_ROM;
	size_t wsz = fwrite(&c, 1, 1, aOut);
	if (wsz != 1)
	{
		fprintf(stderr, "ERROR: Problem writing output file\n");
	}
}

void usage(void)
{
	fprintf(stderr, "mkromfs <base_addr> <output filename> <ROM title> [file1 [file2 [file3 ...]]]\n");
	fprintf(stderr, "        where file1 etc. are the files to be included in the ROM\n");
	fprintf(stderr, "        can be specified as target=host_filename or just as host_filename\n");
	fprintf(stderr, "        where target if present is the name appearing in the ROMFS.\n");
	exit(1);
}

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		usage();
	}
	uint32_t base = strtoul(argv[1], 0, 0);
	const char* outfn = argv[2];
	FILE* out = fopen(outfn, "wb");
	if (!out)
	{
		fprintf(stderr, "ERROR: Can't open file %s for output\n", outfn);
		exit(1);
	}
	CRomFsFile* p0 = CRomFsFile::NewTitle(argv[3], base);
	if (!p0)
	{
		fprintf(stderr, "ERROR: Failed to create title entry\n");
		exit(1);
	}
	base = p0->NextBase();
	p0->WriteFsData(out);
	int arg_ix;
	for (arg_ix=4; arg_ix<argc; ++arg_ix)
	{
		const char* infn = argv[arg_ix];
		CRomFsFile* p1 = CRomFsFile::New(infn, base);
		if (!p1)
		{
			fprintf(stderr, "ERROR: Failed to create entry for %s\n", infn);
			exit(1);
		}
		p1->WriteFsData(out);
		base = p1->NextBase();
	}
	write_terminator(out);
	fclose(out);
	return 0;
}

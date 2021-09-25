/*
* WAV file reader
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

#include "wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CWavFile::CWavFile(const char* aFileName)
:	iTotalSize(0),
	iFs(0),
	iNCh(0),
	iBitsPerSample(0),
	iBytesPerSample(0),
	iBytesPerFrame(0),
	iLength(0),
	iIndex(0),
	iFile(0)
{
	iFile = fopen(aFileName, "rb");
	if (!iFile)
	{
		fprintf(stderr, "Can't open file %s for read\n", aFileName);
		exit(1);
	}
	char hdrBuf[20];
	size_t r = fread(hdrBuf, 1, sizeof(hdrBuf), iFile);
	if (r != sizeof(hdrBuf))
	{
read_error:
		fprintf(stderr, "Problem reading file %s (or file too small)\n", aFileName);
		exit(1);
	}
	if (memcmp(hdrBuf, "RIFF", 4) != 0)
	{
not_valid_wav:
		fprintf(stderr, "File %s is not a valid WAV file\n", aFileName);
		exit(1);
	}
	iTotalSize = GetUInt32LE(hdrBuf + 4);
	if (memcmp(hdrBuf+8, "WAVEfmt ", 8) != 0)
		goto not_valid_wav;

	uint32_t fmtLen = GetUInt32LE(hdrBuf + 16);
	iFmtSection = new uint8_t[fmtLen];
	r = fread(iFmtSection, 1, fmtLen, iFile);
	if (r != fmtLen)
		goto read_error;
	if (fmtLen != 16)
	{
fmt_unrec:
		fprintf(stderr, "File %s has unrecognized format\n", aFileName);
		exit(1);
	}
	uint32_t type = GetUInt16LE(iFmtSection);
	if (type != 1)
		goto fmt_unrec;
	iNCh = GetUInt16LE(iFmtSection + 2);
	iFs = GetUInt32LE(iFmtSection + 4);
	iBytesPerSec = GetUInt32LE(iFmtSection + 8);
	iBytesPerFrame = GetUInt16LE(iFmtSection + 12);
	iBitsPerSample = GetUInt16LE(iFmtSection + 14);
	iBytesPerSample = (iBitsPerSample + 7) >> 3;
	iBitsPerSample = iBytesPerSample << 3;
	if (iBytesPerFrame != iBytesPerSample * iNCh)
		goto fmt_unrec;

	char dsHdr[8];
	r = fread(dsHdr, 1, sizeof(dsHdr), iFile);
	if (r != sizeof(dsHdr))
		goto read_error;
	if (memcmp(dsHdr, "data", 4) != 0)
	{
		fprintf(stderr, "WAV file data section not found\n");
		exit(1);
	}
	iDataSize = GetUInt32LE(dsHdr + 4);
	if (iDataSize + fmtLen + 20 != iTotalSize)
	{
		fprintf(stderr, "WAV file total size and data size inconsistent\n");
		exit(1);
	}
	iLength = iDataSize / iBytesPerFrame;
	printf("Finished reading header info for %s:\n", aFileName);
	printf("Total size   = %u\n", iTotalSize);
	printf("Fs           = %u\n", iFs);
	printf("#Channels    = %u\n", iNCh);
	printf("Bits/sample  = %u\n", iBitsPerSample);
	printf("Bytes/sample = %u\n", iBytesPerSample);
	printf("Bytes/frame  = %u\n", iBytesPerFrame);
	printf("Length       = %u\n", iLength);
	printf("Index        = %u\n", iIndex);
	printf("Data size    = %u\n", iDataSize);
	printf("Bytes/second = %u\n", iBytesPerSec);
}

CWavFile::~CWavFile()
{
	delete[] iFmtSection;
	if (iFile)
		fclose(iFile);
}

void CWavFile::ReadSamples(void* aPtr, uint32_t aNFrames)
{
	size_t readSize = iBytesPerFrame * aNFrames;
	size_t r = fread(aPtr, 1, readSize, iFile);
	if (r != readSize)
	{
		fprintf(stderr, "Problem reading WAV file\n");
		exit(1);
	}
	iIndex += aNFrames;
}

int32_t CWavFile::GetSample(const void* aBuf, uint32_t aFrame, uint32_t aCh)
{
	const uint8_t* p = (const uint8_t*)aBuf;
	p += aFrame * iBytesPerFrame;
	p += aCh * iBytesPerSample;
	switch (iBytesPerSample)
	{
	case 1: return GetInt8(p);
	case 2: return GetInt16LE(p);
	case 3: return GetInt24LE(p);
	default: fprintf(stderr, "%u bytes per sample not supported\n", iBytesPerSample); exit(1);
	}
}

/*
* BBC Micro Cassette Tape Reader
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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include "wav.h"
#include "demod.h"
#include "decoder.h"

FILE* create_numbered_file(const char* name)
{
	int i;
	FILE* f;
	char xname[MAX_NAME_LENGTH+5];
	for (i=0; i<1000; ++i)
	{
		sprintf(xname, "%s.%03d", name, i);
		f = fopen(xname, "r");
		if (!f)
			break;
		fclose(f);
	}
	f = fopen(xname, "wb");
	return f;
}

class CDecoderX : public CDecoder
{
public:
	CDecoderX();
	~CDecoderX();

	virtual void Block(const SBlockHeader* aHdr, const uint8_t* aData);
	virtual void File(const SBlockHeader* aHdr);
	virtual void Eof();

private:
	FILE* iFile;
};

CDecoderX::CDecoderX()
:	iFile(0)
{
}

CDecoderX::~CDecoderX()
{
	if (iFile)
		fclose(iFile);
}

void CDecoderX::Block(const SBlockHeader* aHdr, const uint8_t* aData)
{
	printf("%-10s %02x %04x (%02x)\n", aHdr->iName, aHdr->iBlockNum, aHdr->iBlockLen, aHdr->iBlockFlag);
	fwrite(aData, 1, aHdr->iBlockLen, iFile);
}

void CDecoderX::File(const SBlockHeader* aHdr)
{
	printf("File %-10s  LA %08x  XA %08x\n", aHdr->iName, aHdr->iLoadAddr, aHdr->iExecAddr);
	iFile = create_numbered_file(aHdr->iName);
}

void CDecoderX::Eof()
{
	printf("End of file\n");
	fclose(iFile);
	iFile = 0;
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "tape_reader <input file>\n");
        exit(1);
    }
	CWavFile* pWav = new CWavFile(argv[1]);
	CDecoderX* pDecoder = new CDecoderX();
	CDemodulator* pDemod = new CDemodulator((double)pWav->SampleRate());
	uint8_t* frameBuf = new uint8_t[pWav->BytesPerFrame()];
    printf("Reading file...\n");

	while (pWav->Remain())
	{
		pWav->ReadSamples(frameBuf, 1);
		int32_t sample = pWav->GetSample(frameBuf, 0, 0);
		int bit = pDemod->Sample(sample);
		if (bit != NO_BIT)
		{
			pDecoder->Bit((uint32_t)bit);
		}
	}
	delete[] frameBuf;
	delete pDemod;
	delete pDecoder;
	delete pWav;
	return 0;
}

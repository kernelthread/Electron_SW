/*
* BBC BASIC file to text file converter
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

void usage(const char* err_msg = 0, const char* err_msg2 = 0);

const char* TokenTable6502[]=
{
	"AND",			// 80
	"DIV",			// 81
	"EOR",			// 82
	"MOD",			// 83
	"OR",			// 84
	"ERROR",		// 85
	"LINE",			// 86
	"OFF",			// 87
	"STEP",			// 88
	"SPC",			// 89
	"TAB(",			// 8A
	"ELSE",			// 8B
	"THEN",			// 8C
	0,				// 8D LINE NUMBER
	"OPENIN",		// 8E
	"PTR",			// 8F
	"PAGE",			// 90
	"TIME",			// 91
	"LOMEM",		// 92
	"HIMEM",		// 93
	"ABS",			// 94
	"ACS",			// 95
	"ADVAL",		// 96
	"ASC",			// 97
	"ASN",			// 98
	"ATN",			// 99
	"BGET",			// 9A
	"COS",			// 9B
	"COUNT",		// 9C
	"DEG",			// 9D
	"ERL",			// 9E
	"ERR",			// 9F
	"EVAL",			// A0
	"EXP",			// A1
	"EXT",			// A2
	"FALSE",		// A3
	"FN",			// A4
	"GET",			// A5
	"INKEY",		// A6
	"INSTR(",		// A7
	"INT",			// A8
	"LEN",			// A9
	"LN",			// AA
	"LOG",			// AB
	"NOT",			// AC
	"OPENUP",		// AD
	"OPENOUT",		// AE
	"PI",			// AF
	"POINT(",		// B0
	"POS",			// B1
	"RAD",			// B2
	"RND",			// B3
	"SGN",			// B4
	"SIN",			// B5
	"SQR",			// B6
	"TAN",			// B7
	"TO",			// B8
	"TRUE",			// B9
	"USR",			// BA
	"VAL",			// BB
	"VPOS",			// BC
	"CHR$",			// BD
	"GET$",			// BE
	"INKEY$",		// BF
	"LEFT$(",		// C0
	"MID$(",		// C1
	"RIGHT$(",		// C2
	"STR$",			// C3
	"STRING$(",		// C4
	"EOF",			// C5
	"AUTO",			// C6
	"DELETE",		// C7
	"LOAD",			// C8
	"LIST",			// C9
	"NEW",			// CA
	"OLD",			// CB
	"RENUMBER",		// CC
	"SAVE",			// CD
	"EDIT",			// CE
	"PTR",			// CF
	"PAGE",			// D0
	"TIME",			// D1
	"LOMEM",		// D2
	"HIMEM",		// D3
	"SOUND",		// D4
	"BPUT",			// D5
	"CALL",			// D6
	"CHAIN",		// D7
	"CLEAR",		// D8
	"CLOSE",		// D9
	"CLG",			// DA
	"CLS",			// DB
	"DATA",			// DC
	"DEF",			// DD
	"DIM",			// DE
	"DRAW",			// DF
	"END",			// E0
	"ENDPROC",		// E1
	"ENVELOPE",		// E2
	"FOR",			// E3
	"GOSUB",		// E4
	"GOTO",			// E5
	"GCOL",			// E6
	"IF",			// E7
	"INPUT",		// E8
	"LET",			// E9
	"LOCAL",		// EA
	"MODE",			// EB
	"MOVE",			// EC
	"NEXT",			// ED
	"ON",			// EE
	"VDU",			// EF
	"PLOT",			// F0
	"PRINT",		// F1
	"PROC",			// F2
	"READ",			// F3
	"REM",			// F4
	"REPEAT",		// F5
	"REPORT",		// F6
	"RESTORE",		// F7
	"RETURN",		// F8
	"RUN",			// F9
	"STOP",			// FA
	"COLOUR",		// FB
	"TRACE",		// FC
	"UNTIL",		// FD
	"WIDTH",		// FE
	"OSCLI",		// FF
};

const char* TokenTable68k[]=
{
	"AUTO",		    // 80
	"BPUT",			// 81
	"COLOUR",	    // 82
	"CLEAR",		// 83
	"CLOSE",		// 84
	"CLS",			// 85
	"CLG",			// 86
	"CALL",			// 87
	"CHAIN",		// 88
	"DELETE",		// 89
	"DRAW",			// 8A
	"DATA",			// 8B
	"DEF",			// 8C
	"DIM",			// 8D
	"ENVELOPE",		// 8E
	"ENDPROC",		// 8F
	"END",			// 90
	"ELSE",			// 91
	"ERROR",		// 92
	"FOR",			// 93
	"GOTO",			// 94
	"GOSUB",		// 95
	"GCOL",			// 96
	"INPUT",		// 97
	"IF",			// 98
	"LIST",			// 99
	"LOAD",			// 9A
	"LOCAL",		// 9B
	"LET",			// 9C
	"LINE",			// 9D
	"MODE",			// 9E
	"MOVE",			// 9F
	"NEXT",			// A0
	"NEW",			// A1
	"OLD",			// A2
	"ON",			// A3
	"OFF",			// A4
	"OSCLI",		// A5
	"PRINT",		// A6
	"PROC",			// A7
	"PLOT",			// A8
	"REPEAT",		// A9
	"RETURN",		// AA
	"RESTORE",		// AB
	"REPORT",		// AC
	"REM",			// AD
	"READ",			// AE
	"RUN",			// AF
	"RENUMBER",		// B0
	"STEP",			// B1
	"SAVE",			// B2
	"STOP",			// B3
	"SOUND",		// B4
	"SPC",			// B5
	"TRACE",		// B6
	"THEN",			// B7
	"TAB(",			// B8
	"UNTIL",		// B9
	"VDU",			// BA
	"WIDTH",		// BB
	"AND",			// BC
	"OR",			// BD
	"EOR",			// BE
	"DIV",			// BF
	"MOD",			// C0
	"<=",			// C1
	"<>",			// C2
	">=",			// C3
	"PTR",			// C4
	"PAGE",			// C5
	"TOP",			// C6
	"LOMEM",		// C7
	"HIMEM",		// C8
	"TIME",			// C9
	"CHR$",			// CA
	"GET$",			// CB
	"INKEY$",		// CC
	"LEFT$(",		// CD
	"MID$(",		// CE
	"RIGHT$(",		// CF
	"STR$",			// D0
	"STRING$(",		// D1
	"INSTR(",	    // D2
	"VAL",			// D3
	"ASC",			// D4
	"LET",			// D5
	"GET",			// D6
	"INKEY",		// D7
	"ADVAL",		// D8
	"POS",			// D9
	"VPOS",			// DA
	"COUNT",		// DB
	"POINT(",		// DC
	"ERR",			// DD
	"ERL",			// DE
	"OPENIN",		// DF
	"OPENOUT",		// E0
	"OPENUP",		// E1
	"EXT",			// E2
    "BGET#",		// E3
	"EOF",			// E4
	"TRUE",			// E5
	"FALSE",		// E6
	"ABS",			// E7
	"ACS",			// E8
	"ASN",			// E9
	"ATN",			// EA
	"COS",			// EB
	"DEG",			// EC
	"EVAL",			// ED
	"EXP",			// EE
	"FN",			// EF
	"INT",			// F0
	"LN",			// F1
	"LOG",			// F2
	"NOT",			// F3
	"PI",			// F4
	"RAD",			// F5
	"RND",			// F6
	"SGN",			// F7
	"SIN",			// F8
	"SQR",			// F9
	"TAN",			// FA
	"USR",			// FB
	"TO",			// FC
	"",				// FD
	"",				// FE
	"",				// FF
};

struct TOptions
{
	TOptions();
	~TOptions();

	const char* iInputName;
	const char* iOutputName;
	bool iOverwrite;
	bool iDisplayLineNumbers;
	bool i68k;

	uint32_t iInputLength;
	uint8_t* iInputData;
	FILE* iOutputFile;
};

TOptions::TOptions()
{
	iInputName = 0;
	iOutputName = 0;
	iOverwrite = false;
	iDisplayLineNumbers = false;
	i68k = false;

	iInputLength = 0;
	iInputData = 0;
	iInputName = 0;
}

TOptions::~TOptions()
{
	free(iInputData);
	fclose(iOutputFile);
}

void read_input_file(TOptions& opt)
{
	FILE* in = fopen(opt.iInputName, "rb");
	if (!in)
		usage("Can't open input file ", opt.iInputName);
	fseek(in, 0, SEEK_END);
	opt.iInputLength = (uint32_t)ftell(in);
	fseek(in, 0, SEEK_SET);
	opt.iInputData = (uint8_t*)malloc(opt.iInputLength);
	if (!opt.iInputData)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}
	uint32_t ret = (uint32_t)fread(opt.iInputData, 1, opt.iInputLength, in);
	if (ret != opt.iInputLength)
	{
		fprintf(stderr, "Problem reading input file\n");
		exit(1);
	}
	fclose(in);
}

void open_output_file(TOptions& opt)
{
	char* outName = (char*)opt.iOutputName;
	if (!outName)
	{
		int l = strlen(opt.iInputName);
		outName = (char*)malloc(l + 1 + 4);
		if (!outName)
		{
			fprintf(stderr, "Failed to allocate memory\n");
			exit(1);
		}
		memcpy(outName, opt.iInputName, l);
		strcpy(outName+l, ".txt");
	}
	if (!opt.iOverwrite)
	{
		FILE* f = fopen(outName, "rb");
		if (f)
		{
			fclose(f);
			usage("Can't overwrite existing output file ", outName);
		}
	}
	opt.iOutputFile = fopen(outName, "wb");
	if (!opt.iOutputFile)
	{
		fprintf(stderr, "Can't open output file %s\n", outName);
		exit(1);
	}
	if (outName && !opt.iOutputName)
	{
		free(outName);
	}
}

void process_line(TOptions& opt, uint32_t ioff, uint32_t lineNum, uint32_t lineLen)
{
	const uint8_t* s = opt.iInputData;
	if (opt.iDisplayLineNumbers)
		fprintf(opt.iOutputFile, "%5u ", lineNum);
	uint32_t j = opt.i68k ? 4 : 3;
	uint32_t align = opt.i68k ? 2 : 1;
	uint32_t maxToken = opt.i68k ? 0xFC : 0xFE;
	const char** tt = opt.i68k ? TokenTable68k : TokenTable6502;
    uint32_t last_token = 0;
    uint8_t c;
    uint32_t token;
	for (; j < lineLen; ++j, (token>0 && (last_token = token)))
	{
        token = 0;
		c = s[ioff + j];
		if (c>=0x20 && c<=0x7E)
		{
			fputc(c, opt.iOutputFile);
			continue;
		}
		uint32_t jr = (j + align) & ~(align - 1);
		if (c==0x0D && jr==lineLen)
		{
			fprintf(opt.iOutputFile, "\n");
			j = jr;
			break;
		}
        if (c >= 0x80)
            token = c;
        if (opt.i68k)
		{
			if (c == 0xFD)
			{
				// in future will be 2 byte tokens
			}
			else if (c == 0xFE)
			{
				// in future will be predigest token
			}
			else if (c == 0xFF)
			{
				// cached control flow target token
				uint32_t lnm = 0;
				j = (j + 2) & ~1;		// step to next even address
				switch (last_token)
				{
				case 0x91:	// ELSE
				case 0x94:	// GOTO
				case 0x95:	// GOSUB
				case 0xab:	// RESTORE
				case 0xb7:	// THEN
					lnm = (s[ioff+j]<<8) | s[ioff+j+1];
					break;
				default:
					break;
				}
				j += 6 - 1;
				if (lnm)
					fprintf(opt.iOutputFile, "%d", lnm);
				continue;
			}
		}
        else
        {
            if (c == 0x8D)
            {
                // line number token
                uint32_t b1 = s[ioff+j+1];
                uint32_t b2 = s[ioff+j+2];
                uint32_t b3 = s[ioff+j+3];
                j += 3;
                b1 ^= 0x54;
                uint32_t lnm = ((b1 & 0x30) << 2) | ((b1 & 0x0C) << 12);
                lnm |= (b2 & 0x3F);
                lnm |= ((b3 & 0x3F) << 8);
				fprintf(opt.iOutputFile, "%d", lnm);
                continue;
            }
        }
		if (c > maxToken || c == 0x7F || c < 0x20)
		{
			fprintf(opt.iOutputFile, "`%02x`", c);
			continue;
		}
		// 80-FE = tokens
		const char* tknx = tt[c - 0x80];
		if (tknx)
		{
			fputs(tknx, opt.iOutputFile);
		}
		else
		{
			fprintf(opt.iOutputFile, "`%02x`", c);
		}
	}
}

void process(TOptions& opt)
{
	uint32_t ioff = 0;
	const uint8_t* s = opt.iInputData;
	while (ioff < opt.iInputLength)
	{
		if (!opt.i68k && ioff == 0)
		{
			if (s[ioff] != 0x0D)
			{
				fprintf(stderr, "Initial 0D missing\n");
				exit(1);
			}
			++ioff;
			continue;
		}
		uint32_t remain = opt.iInputLength - ioff;
		if (opt.i68k)
		{
			if (remain < 2)
			{
				fprintf(stderr, "Truncated line encountered at offset %04x\n", ioff);
				exit(1);
			}
			uint32_t lineLen = (s[ioff]<<8) | s[ioff+1];
			if (lineLen == 0)
			{
				// end of file
				ioff += 2;
				break;
			}
			if (remain < 6)
			{
				fprintf(stderr, "Truncated line encountered at offset %04x\n", ioff);
				exit(1);
			}
			uint32_t lineNum = (s[ioff+2]<<8) | s[ioff+3];
			process_line(opt, ioff, lineNum, lineLen);
			ioff += lineLen;
		}
		else
		{
			if (remain == 1)
			{
				if (s[ioff] == 0xFF)
				{
					// end of file
					++ioff;
					break;
				}
			}
			if (remain < 4)
			{
				fprintf(stderr, "Truncated line encountered at offset %04x\n", ioff);
				exit(1);
			}
			uint8_t b0 = s[ioff];
			uint8_t b1 = s[ioff+1];
			uint8_t b2 = s[ioff+2];
			if (b0 == 0xFF)
			{
				fprintf(stderr, "Unexpected EOF marker encountered at offset %04x\n", ioff);
				exit(1);
			}
			uint32_t lineNum = (b0<<8) | b1;
			process_line(opt, ioff, lineNum, b2);
			ioff += b2;
		}
	}
}

void usage(const char* err_msg, const char* err_msg2)
{
	if (err_msg)
	{
		fprintf(stderr, "%s%s\n\n", err_msg, err_msg2 ? err_msg2 : "");
	}
	fprintf(stderr, "acorn2txt [options] <input file name>\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -o <filename>    Specify output filename\n");
	fprintf(stderr, "    -y               Allow overwrite of existing output file\n");
	fprintf(stderr, "    -n               Include line numbers\n");
	fprintf(stderr, "    -68k             Input file is 68000 BASIC format\n");
	exit(1);
}

int main(int argc, char** argv)
{
	int i;
	TOptions opt;
	if (argc <= 1)
	{
		usage();
	}
	for (i=1; i<argc; ++i)
	{
		int remain = argc - i - 1;
		const char* arg = argv[i];
		if (strcmp(arg, "-o") == 0)
		{
			if (remain <= 0)
			{
				usage("-o option needs argument");
			}
			opt.iOutputName = argv[++i];
			continue;
		}
		if (strcmp(arg, "-y") == 0)
		{
			opt.iOverwrite = true;
			continue;
		}
		if (strcmp(arg, "-n") == 0)
		{
			opt.iDisplayLineNumbers = true;
			continue;
		}
		if (strcmp(arg, "-68k") == 0)
		{
			opt.i68k = true;
			continue;
		}
		if (*arg == '-')
		{
			usage("Unrecognised option ", arg);
		}
		opt.iInputName = arg;
		if (remain > 0)
		{
			usage("Garbage following input file name");
		}
	}
	if (!opt.iInputName)
		usage("Input filename not specified");
	read_input_file(opt);
	open_output_file(opt);
	process(opt);
	return 0;
}

/*
* Binary FSK demodulator
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

#include <math.h>
#include <string.h>
#include "demod.h"

#define	FREQ0	(16000000.0 / 13312.0)	// tone frequency used for a 0 bit
#define	FREQ1	(2.0 * FREQ0)			// tone frequency used for a 1 bit

#ifndef PI
#define PI		(3.14159265358979323846)
#endif

CDemodulator::CDemodulator(double aFs)
:	iFs(aFs),
	iF0(FREQ0),
	iF1(FREQ1),
	iPhase(-2 * PI),
	iPhaseDelta(0),
	iPrevY(0),
	iNSamples(0),
	iSymL(0),
	iSym0I(0),
	iSym0Q(0),
	iSym1I(0),
	iSym1Q(0),
	iHistory(0)
{
	iPhaseDelta = 2 * PI * iF1 / iFs;
	iSymL = (int)ceil(iFs/iF0);		// number of samples per symbol period
	iSym0I = new double[iSymL];
	iSym0Q = new double[iSymL];
	iSym1I = new double[iSymL];
	iSym1Q = new double[iSymL];
	iHistory = new double[iSymL];

	int i;
	for (i=0; i<iSymL; ++i)
	{
		iHistory[i] = 0;
		iSym0I[i] = cos( (double)i * iPhaseDelta / 2 );
		iSym0Q[i] = sin( (double)i * iPhaseDelta / 2 );
		iSym1I[i] = cos( (double)i * iPhaseDelta );
		iSym1Q[i] = sin( (double)i * iPhaseDelta );
	}
}

CDemodulator::~CDemodulator()
{
	delete[] iHistory;
	delete[] iSym1Q;
	delete[] iSym1I;
	delete[] iSym0Q;
	delete[] iSym0I;
}

int CDemodulator::Sample(int aSample)
{
	int ret = NO_BIT;
	int i;
	memmove(iHistory+1, iHistory, (iSymL-1)*sizeof(double));
	iHistory[0] = (double)aSample;
	++iNSamples;

	double i0 = 0;
	double q0 = 0;
	double i1 = 0;
	double q1 = 0;
	for (i=0; i<iSymL; ++i)
	{
		i0 += iHistory[i] * iSym0I[i];
		q0 += iHistory[i] * iSym0Q[i];
		i1 += iHistory[i] * iSym1I[i];
		q1 += iHistory[i] * iSym1Q[i];
	}
	double y = i1*i1 + q1*q1 - i0*i0 - q0*q0;
	if (iNSamples >= iSymL)
	{
		if (iPrevY>0 && y<0)
		{
			// 1 to 0 transition detected, so synchronize symbol timing
			iPhase = 0;
		}
		iPhase += iPhaseDelta;
		if (iPhase >= 2*PI)
		{
			// half way through symbol period
			// so this is best time to sample
			iPhase -= 4*PI;
			ret = (y >= 0) ? BIT_1 : BIT_0;
		}
	}
	iPrevY = y;
	return ret;
}


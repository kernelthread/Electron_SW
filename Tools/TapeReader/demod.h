/*
* Header file for binary FSK demodulator
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

// return values from Sample() function
#define	NO_BIT		(-1)			// no bit demodulated on this sample
#define	BIT_0		(0)				// a 0 bit has been demodulated on this sample
#define	BIT_1		(1)				// a 1 bit has been demodulated on this sample

class CDemodulator
{
public:
	CDemodulator(double Fs);
	virtual ~CDemodulator();
	int Sample(int aSample);
private:
	double			iFs;			// sample rate
	double			iF0;			// frequency for 0 bit
	double			iF1;			// frequency for 1 bit
	double			iPhase;			// phase relative to symbol clock (4*pi per symbol)
	double			iPhaseDelta;	// phase delta per sample
	double			iPrevY;			// previous bit discriminant
	uint32_t		iNSamples;		// number of samples processed
	int				iSymL;			// length of each symbol in samples (rounded up)
	double*			iSym0I;			// in-phase reference signal for 0 bit
	double*			iSym0Q;			// quadrature reference signal for 0 bit
	double*			iSym1I;			// in-phase reference signal for 1 bit
	double*			iSym1Q;			// quadrature reference signal for 1 bit
	double*			iHistory;		// sample history
};

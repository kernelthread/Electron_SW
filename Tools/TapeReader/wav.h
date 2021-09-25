/*
* Header file for WAV file reader
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

#include <stdio.h>
#include <stdint.h>

inline uint8_t GetUInt8(const void* aPtr)
{
	return *(const uint8_t*)aPtr;
}

inline int8_t GetInt8(const void* aPtr)
{
	return *(const int8_t*)aPtr;
}

inline uint8_t GetUInt8(const void* aPtr, int aOffset)
{
	return *((const uint8_t*)aPtr + aOffset);
}

inline uint16_t GetUInt16LE(const void* aPtr)
{
	return (uint16_t)((GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

inline int16_t GetInt16LE(const void* aPtr)
{
	return (int16_t)((GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

inline uint32_t GetUInt24LE(const void* aPtr)
{
	return (uint32_t)((GetUInt8(aPtr,2)<<16)|(GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

inline uint32_t GetUInt32LE(const void* aPtr)
{
	return (uint32_t)((GetUInt8(aPtr,3)<<24)|(GetUInt8(aPtr,2)<<16)|(GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

inline int32_t GetInt24LE(const void* aPtr)
{
	return (int32_t)((GetUInt8(aPtr,2)<<16)|(GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

inline int32_t GetInt32LE(const void* aPtr)
{
	return (int32_t)((GetUInt8(aPtr,3)<<24)|(GetUInt8(aPtr,2)<<16)|(GetUInt8(aPtr,1)<<8)|GetUInt8(aPtr));
}

class CWavFile
{
public:
	CWavFile(const char* aFileName);
	void ReadSamples(void* aPtr, uint32_t aNG);
	virtual ~CWavFile();
	inline uint32_t SampleRate() const { return iFs; }
	inline uint32_t NumChannels() const { return iNCh; }
	inline uint32_t BitsPerSample() const { return iBitsPerSample; }
	inline uint32_t BytesPerSample() const { return iBytesPerSample; }
	inline uint32_t BytesPerFrame() const { return iBytesPerFrame; }
	inline uint32_t Length() const { return iLength; }
	inline uint32_t Index() const { return iIndex; }
	inline uint32_t Remain() const { return iLength - iIndex; }
public:
	int32_t GetSample(const void* aBuf, uint32_t aFrame, uint32_t aCh);
private:
	uint32_t	iTotalSize;				// total size of file after first 8 bytes
	uint32_t	iFs;					// sample rate/Hz
	uint16_t	iNCh;					// number of channels
	uint16_t	iBitsPerSample;			// bits per sample
	uint16_t	iBytesPerSample;		// bytes per sample
	uint16_t	iBytesPerFrame;			// bytes per sample across all channel
	uint32_t	iLength;				// number of samples for each channel
	uint32_t	iIndex;					// index of next frame to be read
	uint32_t	iDataSize;				// size of data section
	uint32_t	iBytesPerSec;			// bytes of data transferred per second
	FILE*		iFile;
	uint8_t*	iFmtSection;
};

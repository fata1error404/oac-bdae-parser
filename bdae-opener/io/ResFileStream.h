#ifndef _RES_FILE_STREAM_H_
#define _RES_FILE_STREAM_H_

#include "IReadResFile.h"
#include <iostream>

class CResFileStream
{
public:
	bool m_readError;
	IReadResFile *m_pReadFile;

	CResFileStream(IReadResFile *pFile = NULL);

	~CResFileStream();

	int read(unsigned char *data, int len);

	void seek(int offset);

	int size() const;

	int tell() const;

	bool canRead() const;

	bool canWrite() const;

	void setInputFileObject(IReadResFile *pFile);

	inline signed char ReadS8()
	{
		signed char val = 0;
		read((unsigned char *)&val, sizeof(signed char));
		return val;
	}

	inline short ReadS16()
	{
		short val = 0;
		read((unsigned char *)&val, sizeof(short));
		return val;
	}

	inline int ReadS32()
	{
		int val = 0;
		read((unsigned char *)&val, sizeof(int));
		return val;
	}

	inline unsigned char ReadU8()
	{
		unsigned char val = 0;
		read((unsigned char *)&val, sizeof(unsigned char));
		return val;
	}

	inline unsigned short ReadU16()
	{
		unsigned short val = 0;
		read((unsigned char *)&val, sizeof(unsigned short));
		return val;
	}

	inline unsigned int ReadU32()
	{
		unsigned int val = 0;
		read((unsigned char *)&val, sizeof(unsigned int));
		return val;
	}

	inline float ReadF32()
	{
		float val = 0;
		read((unsigned char *)&val, sizeof(float));
		return val;
	}

	inline std::string ReadStr()
	{
		int stringLen = ReadS16();
		char *stringBuf = new char[stringLen + 1];
		read((unsigned char *)stringBuf, stringLen);
		stringBuf[stringLen] = '\0';
		std::string result = std::string(stringBuf);
		delete[] stringBuf;
		stringBuf = NULL;
		return result;
	}

	inline std::wstring ReadWStr()
	{
		int stringLen = ReadS16();
		wchar_t *stringBuf = new wchar_t[stringLen + 1];
		for (int i = 0; i < stringLen; i++)
		{
			int symbol = ReadS16();
			stringBuf[i] = symbol;
		}
		stringBuf[stringLen] = L'\0';
		std::wstring result = std::wstring(stringBuf);
		delete[] stringBuf;
		stringBuf = NULL;
		return result;
	}

	inline void skip(int size)
	{
		if (size <= 0)
			return;

		seek(tell() + size);
	}

	inline void AllocateAndReadStr(char *&buf)
	{
		int strLen = ReadS16();
		buf = new char[strLen + 1];
		read((unsigned char *)buf, strLen);
		buf[strLen] = 0;
	}
};

#endif

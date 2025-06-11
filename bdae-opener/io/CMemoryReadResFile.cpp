#include <string>
#include <string.h>

#include "CMemoryReadResFile.h"

CMemoryReadResFile::CMemoryReadResFile()
	: Buffer(0), Len(0), Pos(0), Filename("")
{
}

CMemoryReadResFile::CMemoryReadResFile(void *memory, long len, const char *fileName, bool d)
	: Buffer(memory), deleteMemWhenDrop(d), Len(len), Pos(0), Filename(fileName)
{
}

CMemoryReadResFile::~CMemoryReadResFile()
{
	if (deleteMemWhenDrop)
		delete[] ((char *)Buffer);
}

IReadResFile *CMemoryReadResFile::clone() const
{
	CMemoryReadResFile *mrf = new CMemoryReadResFile();

	mrf->Buffer = this->Buffer;
	mrf->deleteMemWhenDrop = this->deleteMemWhenDrop;
	mrf->Len = this->Len;
	mrf->Pos = this->Pos;
	mrf->Filename = this->Filename;

	return mrf;
}

//! sets the memory, length and filename
void CMemoryReadResFile::set(void *memory, long len, const char *fileName)
{
	Buffer = memory;
	Len = len;
	Pos = 0;
	Filename = fileName;
}

//! returns how much was read
int CMemoryReadResFile::read(void *buffer, unsigned int sizeToRead)
{
	int amount = static_cast<int>(sizeToRead);
	if (Pos + amount > Len)
		amount -= Pos + amount - Len;

	if (amount <= 0)
		return 0;

	char *p = (char *)Buffer;
	memcpy(buffer, p + Pos, amount);

	Pos += amount;

	return amount;
}

//! changes position in file, returns true if successful (if relativeMovement == true, the pos is changed relative to current pos, otherwise from begin of file)
bool CMemoryReadResFile::seek(long finalPos, bool relativeMovement)
{
	if (relativeMovement)
	{
		if (Pos + finalPos > Len)
			return false;

		Pos += finalPos;
	}
	else
	{
		if (finalPos > Len)
			return false;

		Pos = finalPos;
	}

	return true;
}

IReadResFile *createMemoryReadFile(void *memory, long size, const char *fileName, bool deleteMemoryWhenDropped)
{
	CMemoryReadResFile *file = new CMemoryReadResFile(memory, size, fileName, deleteMemoryWhenDropped);
	return file;
}

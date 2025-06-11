#include <string>
#include "CLimitReadResFile.h"

CLimitReadResFile::CLimitReadResFile(IReadResFile *alreadyOpenedFile, long areaSize, const char *name)
	: LastPos(0), AreaSize(areaSize), AreaStart(0), AreaEnd(0), File(alreadyOpenedFile), Filename(name)
{
	File = alreadyOpenedFile->clone();
	init();
}

CLimitReadResFile::~CLimitReadResFile()
{
}

IReadResFile *CLimitReadResFile::clone() const
{
	IReadResFile *lrf = new CLimitReadResFile(File, AreaSize, Filename.c_str());
	((CLimitReadResFile *)lrf)->AreaEnd = AreaEnd;
	((CLimitReadResFile *)lrf)->AreaStart = AreaStart;
	((CLimitReadResFile *)lrf)->LastPos = LastPos;
	return lrf;
}

//! returns how much was read
int CLimitReadResFile::read(void *buffer, unsigned int sizeToRead)
{
	long pos = File->getPos();

	if (LastPos != pos)
	{
		File->seek(LastPos);
		pos = LastPos;
	}

	if (pos >= AreaEnd)
		return 0;

	if (pos + (long)sizeToRead >= AreaEnd)
		sizeToRead = AreaEnd - pos;

	int result = File->read(buffer, sizeToRead);
	LastPos += result;
	return result;
}

//! changes position in file, returns true if successful (if relativeMovement == true, the pos is changed relative to current pos, otherwise from begin of file)
bool CLimitReadResFile::seek(long finalPos, bool relativeMovement)
{
	const long pos = File->getPos();
	finalPos += pos - LastPos;

	if (relativeMovement)
	{
		if (LastPos + finalPos > AreaEnd)
			finalPos = AreaEnd - pos;

		LastPos = pos + finalPos;
	}
	else
	{
		finalPos += AreaStart;
		if (finalPos > AreaEnd)
			return false;

		LastPos = finalPos;
	}

	return File->seek(finalPos, relativeMovement);
}

//! initializes the constrained read area for the file
void CLimitReadResFile::init()
{
	if (!File)
		return;

	AreaStart = File->getPos();
	AreaEnd = AreaStart + AreaSize;
	File->seek(AreaStart);
	LastPos = AreaStart;
}

IReadResFile *createLimitReadFile(const char *fileName, IReadResFile *alreadyOpenedFile, long areaSize)
{
	return new CLimitReadResFile(alreadyOpenedFile, areaSize, fileName);
}

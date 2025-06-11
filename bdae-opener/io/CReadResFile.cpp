#include <iostream>
#include <cstdio>
#include <string>
#include "CReadResFile.h"

CReadResFile::CReadResFile(const char *fileName)
	: File(0), FileSize(0), Filename(fileName)
{
	openFile();
}

CReadResFile::~CReadResFile()
{
	if (File)
		fclose(File);
}

IReadResFile *CReadResFile::clone() const
{
	CReadResFile *rf = new CReadResFile(this->getFullPath());
	rf->seek(this->getPos());
	return rf;
}

//! returns how much was read
int CReadResFile::read(void *buffer, unsigned int sizeToRead)
{
	if (!isOpen())
		return 0;

	return (int)fread(buffer, 1, sizeToRead, File);
}

//! changes position in file, returns true if successful (if relativeMovement == true, the pos is changed relative to current pos, otherwise from begin of file)
bool CReadResFile::seek(long finalPos, bool relativeMovement)
{
	if (!isOpen())
		return false;

	return fseek(File, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
}

//! opens the file
void CReadResFile::openFile()
{
	if (Filename.size() == 0)
		return;

	File = fopen(Filename.c_str(), "rb");

	if (File)
	{
		fseek(File, 0, SEEK_END);
		FileSize = getPos();
		fseek(File, 0, SEEK_SET);
	}
}

IReadResFile *createReadFile(const char *fileName)
{
	CReadResFile *file = new CReadResFile(fileName);

	if (file->isOpen())
		return file;

	file->drop();
	return 0;
}

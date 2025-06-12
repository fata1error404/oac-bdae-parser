#ifndef __C_LIMIT_READ_RES_FILE_H_INCLUDED__
#define __C_LIMIT_READ_RES_FILE_H_INCLUDED__

#include "IReadResFile.h"

/*
	Class for reading an already opened file (from disk or memory) with constraints, such as a start position or a limited area size.
	(can be useful, for example, for reading uncompressed files in a zip archive)
	_________________________________________________________________________________________________________________________________
*/

class CLimitReadResFile : public IReadResFile
{
public:
	CLimitReadResFile(IReadResFile *alreadyOpenedFile, long areaSize, const char *name);

	virtual ~CLimitReadResFile();

	virtual IReadResFile *clone() const;

	//! returns how much was read
	virtual int read(void *buffer, unsigned int sizeToRead);

	//! changes position in file, returns true if successful
	virtual bool seek(long finalPos, bool relativeMovement = false);

	//! returns size of file
	virtual long getSize() const { return AreaSize; }

	//! returns where in the file we are
	virtual long getPos() const { return LastPos - AreaStart; }

	//! returns name of file
	virtual const char *getFileName() const { return Filename.c_str(); }

	//! returns complete path
	virtual const char *getFullPath() const { return Filename.c_str(); }

private:
	long LastPos;
	long AreaSize;
	long AreaStart;
	long AreaEnd;

	IReadResFile *File;
	std::string Filename;

	//! initializes the constrained read area for the file
	void init();
};

#endif

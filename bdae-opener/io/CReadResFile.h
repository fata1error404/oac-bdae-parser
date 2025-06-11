#ifndef __C_LIMIT_READ_RES_FILE_H_INCLUDED__
#define __C_LIMIT_READ_RES_FILE_H_INCLUDED__

#include <string>
#include "IReadResFile.h"

// Class for reading a real file from disk.
// _______________________________________

class CReadResFile : public IReadResFile
{
public:
	CReadResFile(const char *fileName);

	virtual ~CReadResFile();

	virtual IReadResFile *clone() const;

	//! returns how much was read
	virtual int read(void *buffer, unsigned int sizeToRead);

	//! changes position in file, returns true if successful
	virtual bool seek(long finalPos, bool relativeMovement = false);

	//! returns size of file
	virtual long getSize() const { return FileSize; }

	//! returns if file is open
	virtual bool isOpen() const { return File != 0; }

	//! returns where in the file we are
	virtual long getPos() const { return ftell(File); }

	//! returns name of file
	virtual const char *getFileName() const { return Filename.c_str(); }

	//! returns complete path
	virtual const char *getFullPath() const { return Filename.c_str(); }

private:
	FILE *File;

	long FileSize;
	std::string Filename;

	//! opens the file
	void openFile();
};

#endif

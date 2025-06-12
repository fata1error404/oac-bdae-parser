#ifndef __C_MEMORY_READ_FILE_H_INCLUDED__
#define __C_MEMORY_READ_FILE_H_INCLUDED__

#include "IReadResFile.h"

// Class for reading from memory.
// ______________________________

class CMemoryReadResFile : public IReadResFile
{
public:
	CMemoryReadResFile();

	CMemoryReadResFile(void *memory, long len, const char *fileName, bool deleteMemoryWhenDropped);

	virtual ~CMemoryReadResFile();

	IReadResFile *clone() const;

	//! sets the memory, length and filename
	void set(void *memory, long len, const char *fileName);

	//! returns if the file is valid
	bool isValid() const { return Buffer && Len >= 0; }

	//! returns how much was read
	virtual int read(void *buffer, unsigned int sizeToRead);

	//! changes position in file, returns true if successful
	virtual bool seek(long finalPos, bool relativeMovement = false);

	//! returns size of file
	virtual long getSize() const { return Len; }

	//! returns where in the file we are
	virtual long getPos() const { return Pos; }

	//! returns name of file
	virtual const char *getFileName() const { return Filename.c_str(); }

	//! returns complete path
	virtual const char *getFullPath() const { return Filename.c_str(); }

	//! gets the buffer containing file data
	virtual void *getBuffer(long *size)
	{
		if (size)
			*size = Pos;

		return Buffer;
	}

private:
	void *Buffer;
	bool deleteMemWhenDrop;
	long Len;
	long Pos;
	std::string Filename;
};

IReadResFile *createMemoryReadFile(void *memory, long size, const char *fileName, bool deleteMemoryWhenDropped);

#endif

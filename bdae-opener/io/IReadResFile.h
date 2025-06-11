#ifndef __I_READ_RES_FILE_H_INCLUDED__
#define __I_READ_RES_FILE_H_INCLUDED__

#include "ResReferenceCounted.h"

// Class that declares interface for reading resource files.
// _________________________________________________________

class IReadResFile : public IResReferenceCounted
{
public:
	IReadResFile() {}

	virtual ~IReadResFile() {}

	virtual IReadResFile *clone() const = 0;

	//! Reads a specified amount of bytes from the file.
	/** \param buffer Pointer to buffer where read bytes are written to.
		\param sizeToRead Amount of bytes to read from the file.
		\return The number of bytes actually read. */
	virtual int read(void *buffer, unsigned int sizeToRead) = 0;

	//! Changes the position within the file.
	/** \param finalPos Destination position in the file.
		\param relativeMovement If set to true, the position in the file is
		changed relative to the current position. Otherwise, the position is changed
		from the beginning of the file.
		\return True if successful, false otherwise. */
	virtual bool seek(long finalPos, bool relativeMovement = false) = 0;

	//! Gets the size of the file.
	/** \return Size of the file in bytes. */
	virtual long getSize() const = 0;

	//! Gets the current position in the file.
	/** \return Current position in the file in bytes. */
	virtual long getPos() const = 0;

	//! Gets the name of the file.
	/** \return File name as a zero terminated character string. */
	virtual const char *getFileName() const = 0;

	//! Gets the buffer containing file data.
	/** \param size Pointer to variable where the size of the buffer will be stored.
		\return (is it void?) */
	virtual void *getBuffer(long *size) { return 0; }
};

//! Internal function, please do not use.
IReadResFile *createReadFile(const char *fileName);

#endif

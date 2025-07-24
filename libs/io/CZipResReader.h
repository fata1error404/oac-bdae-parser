#ifndef __C_ZIP_RES_READER_H_INCLUDED__
#define __C_ZIP_RES_READER_H_INCLUDED__

#include <string>
#include <vector>
#include <pthread.h>
#include "IReadResFile.h"

#define ZIP_RES_INFO_IN_DATA_DESCRITOR 0x0008
// #pragma pack(push, 1)
#define PACK_STRUCT

struct SZIPResFileDataDescriptor
{
	int CRC32;
	int CompressedSize;
	int UncompressedSize;
} PACK_STRUCT;

struct SZIPResFileHeader
{
	int Sig;
	short VersionToExtract;
	short GeneralBitFlag;
	short CompressionMethod;
	short LastModFileTime;
	short LastModFileDate;
	SZIPResFileDataDescriptor DataDescriptor;
	short FilenameLength;
	short ExtraFieldLength;
} PACK_STRUCT;

// #pragma pack(pop)

struct SZipResFileEntry
{
	int fileDataPosition;
	std::string zipFileName;
	std::string simpleFileName;
	std::string path;
	SZIPResFileHeader header;

	bool operator<(const SZipResFileEntry &other) const
	{
		for (unsigned int i = 0; i < simpleFileName.length() && i < other.simpleFileName.length(); ++i)
		{
			int diff = simpleFileName[i] - other.simpleFileName[i];
			if (diff)
				return diff < 0;
		}
		return simpleFileName.length() < other.simpleFileName.length();
	}

	bool operator==(const SZipResFileEntry &other) const
	{
		return simpleFileName.compare(other.simpleFileName) == 0;
	}
};

/*!
	Zip file Reader class.
	Doesn't decompress data, only reads the file and is able to
	open uncompressed entries.
	___________________________________________________________
*/

class CZipResReader : public IResReferenceCounted
{
public:
	CZipResReader(const char *filename, bool ignoreCase, bool ignorePaths);

	CZipResReader(IReadResFile *file, bool ignoreCase, bool ignorePaths);

	virtual ~CZipResReader();

	//! Opens a file by file name.
	virtual IReadResFile *openFile(const char *filename);

	//! Opens a file by index.
	IReadResFile *openFile(int index);

	//! Returns count of files in archive.
	int getFileCount() { return FileList.size(); }

	//! Returns data of file.
	const SZipResFileEntry *getFileInfo(int index) const { return &FileList[index]; }

	//! Returns fileindex.
	int findFile(const char *filename);

	//! Returns the name of the zip file archive.
	const char *getZipFileName() const { return File ? File->getFileName() : NULL; }

	static bool isValid(IReadResFile *file);

	static bool isValid(const char *filename);

private:
	IReadResFile *File;

	//! Scans for a local file header, returns false if there is no more.
	bool scanLocalHeader();

protected:
	bool IgnoreCase;
	bool IgnorePaths;
	std::vector<SZipResFileEntry> FileList;
	pthread_mutex_t mutex;

	//! Splits filename from zip file into useful filenames and paths.
	void extractFilename(SZipResFileEntry *entry);

	//! Deletes the path from a filename.
	void deletePathFromFilename(std::string &filename);
};

template <class T>
inline int my_binary_search(const std::vector<T> &vec, const T &element)
{
	if (vec.empty())
		return -1;

	typename std::vector<T>::const_iterator ite = std::lower_bound(vec.begin(), vec.end(), element);

	if (ite != vec.end() && (*ite == element))
		return std::distance(vec.begin(), ite);
	else
		return -1;
}

#endif

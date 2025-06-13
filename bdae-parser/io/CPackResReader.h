#ifndef __C_PACK_RES_READER_H_INCLUDED__
#define __C_PACK_RES_READER_H_INCLUDED__

#define MAX_FILE_NAME_LEN 1024

#include "IReadResFile.h"
#include "CZipResReader.h"

#pragma pack(push, 1)

/*
		Central directory structure:

		[file header 1]
		.
		.
		.
		[file header n]
		[digital signature]


		central file header signature   4 bytes  (0x02014b50)
		version made by                 2 bytes
		version needed to extract       2 bytes
		general purpose bit flag        2 bytes
		compression method              2 bytes
		last mod file time              2 bytes
		last mod file date              2 bytes
		crc-32                          4 bytes
		compressed size                 4 bytes
		uncompressed size               4 bytes
		file name length                2 bytes
		extra field length              2 bytes
		file comment length             2 bytes
		disk number start               2 bytes
		internal file attributes        2 bytes
		external file attributes        4 bytes
		relative offset of local header 4 bytes

		file name (variable size)
		extra field (variable size)
		file comment (variable size)
*/

struct SZIPCenterDirFileHeader
{
	int Sig;						 //	4 bytes  (0x02014b50)
	short VersionMadeBy;			 //	2 bytes
	short VersionToExtract;			 //	2 bytes
	short GeneralBitFlag;			 //	2 bytes
	short CompressionMethod;		 //	2 bytes
	short LastModFileTime;			 //	2 bytes
	short LastModFileDate;			 //	2 bytes
	int CRC32;						 //	4 bytes
	int CompressedSize;				 //	4 bytes
	int UncompressedSize;			 //	4 bytes
	short FilenameLength;			 //	2 bytes
	short ExtraFieldLength;			 //	2 bytes
	short FileCommentLength;		 //	2 bytes
	short DiskNumberStart;			 // 2 bytes
	short InternalFileAttributes;	 // 2 bytes
	int ExternalFileAttributes;		 // 4 bytes
	int RelativeOffsetOfLocalHeader; //	4 bytes
};

/*
		End of central directory record:

		end of central dir signature													4 bytes  (0x06054b50)
		number of this disk																2 bytes
		number of the disk with the start of the central directory						2 bytes
		total number of entries in the central directory on this disk					2 bytes
		total number of entries in the central directory								2 bytes
		size of the central directory													4 bytes
		offset of start of central directory with respect to the starting disk number	4 bytes
		.ZIP file comment length														2 bytes
		.ZIP file comment																(variable size)
*/

struct SZIPEndCenterDirRecord
{
	int Sig;					 //	 4 bytes  (0x06054b50)
	short ThisDiskNo;			 //	 2 bytes
	short CenterDirStartDiskNo;	 //	 2 bytes
	short EntryTotalNumThisDisk; //	 2 bytes
	short EntryTotalNum;		 //	 2 bytes
	short CenterDirSize;		 //	 4 bytes
	short CenterDirOffset;		 //	 4 bytes
	short sZipFileCommentLength; //	 2 bytes
};

#pragma pack(pop)

struct SPackResFileEntry
{
	const char *fileName;  // Pointer to fileName
	int fileDataPosition;  // position of compressed data in file
	short localheaderSize; // when it is equal to zero, then need read
	short compressionMethod;
	unsigned int filePathHash;
	unsigned int uncompressedSize;
	unsigned int compressedSize;

	SPackResFileEntry() : localheaderSize(0) {}

	bool operator<(const SPackResFileEntry &other) const
	{
		return filePathHash < other.filePathHash;
	}

	bool operator==(const SPackResFileEntry &other) const
	{
		return filePathHash == other.filePathHash;
	}
};

/*!
	Pack file Reader.
	A custom pack reader to reduce memory cost when containing many files in a pack.
	For example, the file unit's memory manage cost is reduced from 100 bytes to only 12 bytes (the size of SPackResFileEntry);
	The file unit is compressed by zip generally.
	___________________________________________________________________________________________________________________________
*/

class CPackResReader : public IResReferenceCounted
{
public:
	CPackResReader(const char *filename, bool ignoreCase, bool ignorePaths);

	CPackResReader(IReadResFile *file, bool ignoreCase, bool ignorePaths);

	virtual ~CPackResReader();

	//! Opens a file by file name.
	virtual IReadResFile *openFile(const char *filename);

	//! Opens a file by index.
	IReadResFile *openFile(int index);

	//! Returns count of files in archive.
	int getFileCount() { return m_fileList.size(); }

	//! Returns data of file.
	const SPackResFileEntry *getFileInfo(int index) const { return &m_fileList[index]; }

	//! Returns fileindex.
	int findFile(const char *filename);

	//! Returns the name of the Pack file archive.
	const char *getPackFileName() const { return m_file ? m_file->getFileName() : NULL; }

	static bool isValid(IReadResFile *file);

	static bool isValid(const char *filename);

	//! Scans for a local header, returns false if there is no more local file header.
	void scanFileHeader();

private:
	IReadResFile *m_file;

protected:
	int m_fileNb;
	char *m_fileNameBuffer; // All file Name
	std::vector<SPackResFileEntry> m_fileList;
	pthread_mutex_t mutex;
};

#endif

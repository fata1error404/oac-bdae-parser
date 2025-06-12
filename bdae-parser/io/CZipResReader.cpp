#include <cstdio>
#include <cstring>
#include <string.h>
#include <algorithm>
#include "CZipResReader.h"
#include "CMemoryReadResFile.h"
#include "zlib/zlib.h"

CZipResReader::CZipResReader(const char *filename, bool ignoreCase, bool ignorePaths)
	: IgnoreCase(ignoreCase), IgnorePaths(ignorePaths)
{
	pthread_mutex_init(&mutex, NULL);

	File = createReadFile(filename);

	if (File)
	{
		// scan local headers
		while (scanLocalHeader())
			;

		// prepare file index for binary search
		if (FileList.size() >= 2)
			std::sort(FileList.begin(), FileList.end());
	}
}

CZipResReader::CZipResReader(IReadResFile *file, bool ignoreCase, bool ignorePaths)
	: File(file), IgnoreCase(ignoreCase), IgnorePaths(ignorePaths)
{
	pthread_mutex_init(&mutex, NULL);

	if (File)
	{
		File->grab();

		// scan local headers
		while (scanLocalHeader())
			;

		// prepare file index for binary search
		if (FileList.size() >= 2)
			std::sort(FileList.begin(), FileList.end());
	}
}

CZipResReader::~CZipResReader()
{
	if (File)
		File->drop();

	pthread_mutex_destroy(&mutex);
}

//! splits filename from zip file into useful filenames and paths
void CZipResReader::extractFilename(SZipResFileEntry *entry)
{
	int lorfn = entry->header.FilenameLength; // length of real file name

	if (!lorfn)
		return;

	if (IgnoreCase)
	{
		for (unsigned int i = 0; i < entry->zipFileName.length(); ++i)
		{
			char x = entry->zipFileName[i];
			entry->zipFileName[i] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
		}
	}
	const char *p = entry->zipFileName.c_str() + lorfn;

	while (*p != '/' && p != entry->zipFileName.c_str())
	{
		--p;
		--lorfn;
	}

	bool thereIsAPath = p != entry->zipFileName.c_str();

	if (thereIsAPath)
	{
		++p;
		++lorfn;
	}

	entry->simpleFileName = p;
	entry->path = "";

	if (thereIsAPath)
	{
		lorfn = (int)(p - entry->zipFileName.c_str());

		entry->path = entry->zipFileName.substr(0, lorfn);
	}

	if (!IgnorePaths)
		entry->simpleFileName = entry->zipFileName;
}

//! scans for a local header, returns false if there is no more local file header
bool CZipResReader::scanLocalHeader()
{
	char tmp[1024];

	SZipResFileEntry entry;
	entry.fileDataPosition = 0;
	memset(&entry.header, 0, sizeof(SZIPResFileHeader));

	File->read(&entry.header, sizeof(SZIPResFileHeader));

#ifdef __BIG_ENDIAN__
	entry.header.Sig = os::byteswap(entry.header.Sig);
	entry.header.VersionToExtract = os::byteswap(entry.header.VersionToExtract);
	entry.header.GeneralBitFlag = os::byteswap(entry.header.GeneralBitFlag);
	entry.header.CompressionMethod = os::byteswap(entry.header.CompressionMethod);
	entry.header.LastModFileTime = os::byteswap(entry.header.LastModFileTime);
	entry.header.LastModFileDate = os::byteswap(entry.header.LastModFileDate);
	entry.header.DataDescriptor.CRC32 = os::byteswap(entry.header.DataDescriptor.CRC32);
	entry.header.DataDescriptor.CompressedSize = os::byteswap(entry.header.DataDescriptor.CompressedSize);
	entry.header.DataDescriptor.UncompressedSize = os::byteswap(entry.header.DataDescriptor.UncompressedSize);
	entry.header.FilenameLength = os::byteswap(entry.header.FilenameLength);
	entry.header.ExtraFieldLength = os::byteswap(entry.header.ExtraFieldLength);
#endif

	if (entry.header.Sig != 0x04034b50 && entry.header.Sig != 0x504d4247)
		return false; // local file headers end here

	// read filename
	entry.zipFileName.reserve(entry.header.FilenameLength + 2);
	File->read(tmp, entry.header.FilenameLength);
	tmp[entry.header.FilenameLength] = 0x0;
	entry.zipFileName = tmp;

	extractFilename(&entry);

	// move forward length of extra field
	if (entry.header.ExtraFieldLength)
		File->seek(entry.header.ExtraFieldLength, true);

	// if bit 3 was set, read DataDescriptor, following after the compressed data
	if (entry.header.GeneralBitFlag & ZIP_RES_INFO_IN_DATA_DESCRITOR)
	{
		// read data descriptor
		File->read(&entry.header.DataDescriptor, sizeof(entry.header.DataDescriptor));
#ifdef __BIG_ENDIAN__
		entry.header.DataDescriptor.CRC32 = os::byteswap(entry.header.DataDescriptor.CRC32);
		entry.header.DataDescriptor.CompressedSize = os::byteswap(entry.header.DataDescriptor.CompressedSize);
		entry.header.DataDescriptor.UncompressedSize = os::byteswap(entry.header.DataDescriptor.UncompressedSize);
#endif
	}

	// store position in file
	entry.fileDataPosition = File->getPos();

	// move forward length of data
	File->seek(entry.header.DataDescriptor.CompressedSize, true);

	FileList.push_back(entry);

	return true;
}

//! opens a file by file name
IReadResFile *CZipResReader::openFile(const char *filename)
{
	int index = findFile(filename);

	if (index != -1)
		return openFile(index);

	return 0;
}

//! opens a file by index
IReadResFile *CZipResReader::openFile(int index)
{
	// 0 - The file is stored (no compression)
	// 1 - The file is Shrunk
	// 2 - The file is Reduced with compression factor 1
	// 3 - The file is Reduced with compression factor 2
	// 4 - The file is Reduced with compression factor 3
	// 5 - The file is Reduced with compression factor 4
	// 6 - The file is Imploded
	// 7 - Reserved for Tokenizing compression algorithm
	// 8 - The file is Deflated
	// 9 - Reserved for enhanced Deflating
	// 10 - PKWARE Date Compression Library Imploding

	switch (FileList[index].header.CompressionMethod)
	{
	case 0: // no compression
	{
		const unsigned int uncompressedSize = FileList[index].header.DataDescriptor.UncompressedSize;
		if (uncompressedSize > 0)
		{
			void *pBuf = new char[uncompressedSize];
			if (!pBuf)
			{
				printf("Not enough memory for read file %s", FileList[index].simpleFileName.c_str());
				return 0;
			}
			pthread_mutex_lock(&mutex);
			File->seek(FileList[index].fileDataPosition);
			File->read(pBuf, uncompressedSize);
			pthread_mutex_unlock(&mutex);
			return createMemoryReadFile(pBuf, uncompressedSize, FileList[index].zipFileName.c_str(), true);
		}
		return 0;
	}
	case 8:
	{
		const unsigned int uncompressedSize = FileList[index].header.DataDescriptor.UncompressedSize;
		const unsigned int compressedSize = FileList[index].header.DataDescriptor.CompressedSize;

		void *pBuf = new char[uncompressedSize];
		if (!pBuf)
		{
			printf("Not enough memory for decompressing %s", FileList[index].simpleFileName.c_str());
			return 0;
		}

		char *pcData = new char[compressedSize];
		if (!pcData)
		{
			printf("Not enough memory for decompressing %s", FileList[index].simpleFileName.c_str());
			return 0;
		}

		pthread_mutex_lock(&mutex);
		File->seek(FileList[index].fileDataPosition);
		File->read(pcData, compressedSize);
		pthread_mutex_unlock(&mutex);

		// setup the inflate stream
		z_stream stream;
		int err;

		stream.next_in = (Bytef *)pcData;
		stream.avail_in = (uInt)compressedSize;
		stream.next_out = (Bytef *)pBuf;
		stream.avail_out = uncompressedSize;
		stream.zalloc = (alloc_func)0;
		stream.zfree = (free_func)0;

		// perform inflation. wbits < 0 indicates no zlib header inside the data
		err = inflateInit2(&stream, -MAX_WBITS);
		if (err == Z_OK)
		{
			err = inflate(&stream, Z_FINISH);
			inflateEnd(&stream);
			if (err == Z_STREAM_END)
				err = Z_OK;
			err = Z_OK;
			inflateEnd(&stream);
		}

		delete[] pcData;

		if (err != Z_OK)
		{
			delete[] (char *)pBuf;
			return 0;
		}
		else
			return createMemoryReadFile(pBuf, uncompressedSize, FileList[index].zipFileName.c_str(), true);
	}
	default:
		return 0;
	};
}

//! deletes the path from a filename
void CZipResReader::deletePathFromFilename(std::string &filename)
{
	// delete path from filename
	const char *p = filename.c_str() + filename.size();

	// search for path separator or beginning
	while (*p != '/' && *p != '\\' && p != filename.c_str())
		--p;

	if (p != filename.c_str())
	{
		++p;
		filename = p;
	}
}

//! returns fileindex
int CZipResReader::findFile(const char *simpleFilename)
{
	SZipResFileEntry entry;
	if (simpleFilename[0] == '.' && simpleFilename[1] == '/')
		entry.simpleFileName = simpleFilename + 2;
	else
		entry.simpleFileName = simpleFilename;

	if (IgnoreCase)
	{
		for (unsigned int i = 0; i < entry.simpleFileName.length(); ++i)
		{
			char x = entry.simpleFileName[i];
			entry.simpleFileName[i] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
		}
	}
	if (IgnorePaths)
		deletePathFromFilename(entry.simpleFileName);

	int res = my_binary_search(FileList, entry);

	return res;
}

bool CZipResReader::isValid(IReadResFile *file)
{
	const char zipTag[] = {'P', 'K', 0x03, 0x04};
	char header[4];
	int pos = file->getPos();
	file->read(header, 4);
	file->seek(pos);
	return std::memcmp(header, zipTag, 4) == 0;
	;
}

bool CZipResReader::isValid(const char *filename)
{
	IReadResFile *readFile = createReadFile(filename);
	bool validation = isValid(readFile);
	readFile->drop();
	return validation;
}

#include <cstdio>
#include <string.h>
#include <algorithm>
#include "CPackResReader.h"
#include "CMemoryReadResFile.h"
#include "zlib/zlib.h"

unsigned int hashStringSimple(const char *ptr)
{
	unsigned int hash = 0;

	for (; *ptr; ++ptr)
		hash = (hash * 13) + (*ptr);

	return hash;
}

CPackResReader::CPackResReader(const char *filename, bool ignoreCase, bool ignorePaths)
	: m_fileNb(0), m_fileNameBuffer(NULL)
{
	pthread_mutex_init(&mutex, NULL);

	m_file = createReadFile(filename);

	if (m_file)
	{
		// scan local headers
		scanFileHeader();

		// prepare file index for binary search
		if (m_fileList.size() >= 2)
			std::sort(m_fileList.begin(), m_fileList.end());
	}
}

CPackResReader::CPackResReader(IReadResFile *file, bool ignoreCase, bool ignorePaths)
	: m_file(file), m_fileNb(0), m_fileNameBuffer(NULL)
{
	pthread_mutex_init(&mutex, NULL);

	if (m_file)
	{
		m_file->grab();

		// scan local headers
		scanFileHeader();

		// prepare file index for binary search
		if (m_fileList.size() >= 2)
			std::sort(m_fileList.begin(), m_fileList.end());
	}
}

CPackResReader::~CPackResReader()
{
	if (m_file)
		m_file->drop();

	if (m_fileNameBuffer)
	{
		delete[] m_fileNameBuffer;
		m_fileNameBuffer = NULL;
	}

	pthread_mutex_destroy(&mutex);
}

//! scans for a local header, returns false if there is no more local file header.
void CPackResReader::scanFileHeader()
{
	int fileSize = m_file->getSize();
	if (static_cast<size_t>(fileSize) < sizeof(SZIPEndCenterDirRecord)) // the pack file's length is too short
		return;
	m_file->seek(fileSize - sizeof(SZIPEndCenterDirRecord));
	SZIPEndCenterDirRecord endCenterDirRecord;
	m_file->read(&endCenterDirRecord, sizeof(SZIPEndCenterDirRecord));

	if (endCenterDirRecord.Sig == 0x06054b50) //"PK 05 06"
	{
		m_fileNb = endCenterDirRecord.EntryTotalNum;
		m_fileList.reserve(m_fileNb);

		m_file->seek(endCenterDirRecord.CenterDirOffset);
		char *buffer = new char[endCenterDirRecord.CenterDirSize];
		m_file->read(buffer, endCenterDirRecord.CenterDirSize);

		SZIPCenterDirFileHeader *pHead = (SZIPCenterDirFileHeader *)buffer;
		char zipFileName[MAX_FILE_NAME_LEN];
		int i = 0;
		int fileNameBuffLength = 0;
		SPackResFileEntry entry;
		SZIPResFileHeader zipFileHeader;
		for (; i < m_fileNb; i++)
		{
			if (pHead->Sig != 0x02014b50) // Error
				break;

			// Calculate Hash
			char *pFileName = (char *)(pHead + 1);
			for (int j = 0; j < pHead->FilenameLength; j++)
			{
				char x = pFileName[j];
				zipFileName[j] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
			}
			zipFileName[pHead->FilenameLength] = 0;

			entry.filePathHash = hashStringSimple(zipFileName);
			entry.fileDataPosition = pHead->RelativeOffsetOfLocalHeader;
			entry.fileName = NULL;
			fileNameBuffLength += (pHead->FilenameLength + 4) & ~3;
			// Read Other Information from Local Header, slow but can accelerate dynamic loading in Action Phase
			{
				m_file->seek(entry.fileDataPosition);
				m_file->read(&zipFileHeader, sizeof(SZIPResFileHeader));

				if (zipFileHeader.Sig != 0x04034b50 && zipFileHeader.Sig != 0x504d4247)
					break; // local file headers end here.

				entry.localheaderSize = sizeof(SZIPResFileHeader) + zipFileHeader.FilenameLength + zipFileHeader.ExtraFieldLength;
				// if bit 3 was set, read DataDescriptor, following after the compressed data
				if (zipFileHeader.GeneralBitFlag & ZIP_RES_INFO_IN_DATA_DESCRITOR)
				{
					m_file->seek(zipFileHeader.FilenameLength + zipFileHeader.ExtraFieldLength, true);
					// read data descriptor
					m_file->read(&zipFileHeader.DataDescriptor, sizeof(zipFileHeader.DataDescriptor));
					entry.localheaderSize += sizeof(zipFileHeader.DataDescriptor);
				}
				entry.compressionMethod = zipFileHeader.CompressionMethod;
				entry.uncompressedSize = zipFileHeader.DataDescriptor.UncompressedSize;
				entry.compressedSize = zipFileHeader.DataDescriptor.CompressedSize;
			}

			m_fileList.push_back(entry);

			pHead = (SZIPCenterDirFileHeader *)(pFileName + pHead->FilenameLength + pHead->ExtraFieldLength + pHead->FileCommentLength);
		}
		if (i == m_fileNb) // if success, return ,else scan for start
		{
			m_fileNameBuffer = new char[fileNameBuffLength];
			memset(m_fileNameBuffer, 0, fileNameBuffLength);
			fileNameBuffLength = 0;
			pHead = (SZIPCenterDirFileHeader *)buffer;
			for (i = 0; i < m_fileNb; i++)
			{
				if (pHead->Sig != 0x02014b50) // Error
					break;
				// Calculate Hash
				char *pFileName = (char *)(pHead + 1);
				for (int j = 0; j < pHead->FilenameLength; j++)
				{
					char x = pFileName[j];
					m_fileNameBuffer[fileNameBuffLength + j] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
				}

				m_fileList[i].fileName = m_fileNameBuffer + fileNameBuffLength;
				fileNameBuffLength += (pHead->FilenameLength + 4) & ~3;
				pHead = (SZIPCenterDirFileHeader *)(pFileName + pHead->FilenameLength + pHead->ExtraFieldLength + pHead->FileCommentLength);
			}

			delete[] buffer;

			return;
		}
		delete[] buffer;
	}

	// for compatible, not found end CenterDir Record, scan from start
	{
		m_file->seek(0);
		m_fileNb = 0;
		char zipFileName[MAX_FILE_NAME_LEN];
		SZIPResFileHeader zipFileHeader;
		int fileNameBuffLength = 0;
		std::vector<std::string> fileNameStringVector;
		while (true)
		{
			SPackResFileEntry entry;
			entry.fileDataPosition = m_file->getPos();

			memset(&zipFileHeader, 0, sizeof(SZIPResFileHeader));
			m_file->read(&zipFileHeader, sizeof(SZIPResFileHeader));

			if (zipFileHeader.Sig != 0x04034b50 && zipFileHeader.Sig != 0x504d4247)
				break; // local file headers end here.

			// read filename
			m_file->read(zipFileName, zipFileHeader.FilenameLength);
			zipFileName[zipFileHeader.FilenameLength] = 0;
			fileNameStringVector.push_back(zipFileName);

			// move forward length of extra field.
			if (zipFileHeader.ExtraFieldLength)
				m_file->seek(zipFileHeader.ExtraFieldLength, true);

			// if bit 3 was set, read DataDescriptor, following after the compressed data
			if (zipFileHeader.GeneralBitFlag & ZIP_RES_INFO_IN_DATA_DESCRITOR)
			{
				// read data descriptor
				m_file->read(&zipFileHeader.DataDescriptor, sizeof(zipFileHeader.DataDescriptor));
			}

			m_file->seek(zipFileHeader.DataDescriptor.CompressedSize, true);

			for (int i = 0; i < zipFileHeader.FilenameLength; i++)
			{
				char x = zipFileName[i];
				zipFileName[i] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
			}
			entry.filePathHash = hashStringSimple(zipFileName);
			entry.fileName = 0;
			fileNameBuffLength += (zipFileHeader.FilenameLength + 4) & ~3;

			m_fileList.push_back(entry);
			m_fileNb++;
		}
		m_fileNameBuffer = new char[fileNameBuffLength];
		memset(m_fileNameBuffer, 0, fileNameBuffLength);
		fileNameBuffLength = 0;
		for (int i = 0; i < m_fileNb; i++)
		{
			memcpy(m_fileNameBuffer + fileNameBuffLength, fileNameStringVector[i].c_str(), fileNameStringVector[i].length());
			m_fileList[i].fileName = m_fileNameBuffer + fileNameBuffLength;
			fileNameBuffLength += (fileNameStringVector[i].length() + 4) & ~3;
		}
	}
	// return true;
}

//! opens a file by file name
IReadResFile *CPackResReader::openFile(const char *filename)
{
	int index = findFile(filename);

	if (index != -1)
		return openFile(index);

	return 0;
}

//! opens a file by index
IReadResFile *CPackResReader::openFile(int index)
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

	SPackResFileEntry &fileEntry = m_fileList[index];

	if (fileEntry.localheaderSize == 0)
	{

		SZIPResFileHeader zipFileHeader;
		unsigned int zipFileHeaderLength;

		pthread_mutex_lock(&mutex); // Must lock for thread safe
		m_file->seek(fileEntry.fileDataPosition);
		m_file->read(&zipFileHeader, sizeof(SZIPResFileHeader));

		if (zipFileHeader.Sig != 0x04034b50 && zipFileHeader.Sig != 0x504d4247)
		{
			pthread_mutex_unlock(&mutex);
			return NULL; // local file headers end here.
		}

		zipFileHeaderLength = sizeof(SZIPResFileHeader) + zipFileHeader.FilenameLength + zipFileHeader.ExtraFieldLength;

		// if bit 3 was set, read DataDescriptor, following after the compressed data
		if (zipFileHeader.GeneralBitFlag & ZIP_RES_INFO_IN_DATA_DESCRITOR)
		{
			m_file->seek(zipFileHeader.FilenameLength + zipFileHeader.ExtraFieldLength, true);
			// read data descriptor
			m_file->read(&zipFileHeader.DataDescriptor, sizeof(zipFileHeader.DataDescriptor));
			zipFileHeaderLength += sizeof(zipFileHeader.DataDescriptor);
		}
		pthread_mutex_unlock(&mutex);
		fileEntry.localheaderSize = zipFileHeaderLength;
		fileEntry.compressionMethod = zipFileHeader.CompressionMethod;
		fileEntry.uncompressedSize = zipFileHeader.DataDescriptor.UncompressedSize;
		fileEntry.compressedSize = zipFileHeader.DataDescriptor.CompressedSize;
	}

	int realDataPos = fileEntry.fileDataPosition + fileEntry.localheaderSize;

	switch (fileEntry.compressionMethod)
	{
	case 0: // no compression
	{
		const unsigned int uncompressedSize = fileEntry.uncompressedSize;
		if (uncompressedSize > 0)
		{
			void *pBuf = new char[uncompressedSize];
			if (!pBuf)
			{
				printf("Not enough memory for read file %s", fileEntry.fileName);
				return 0;
			}
			pthread_mutex_lock(&mutex);
			m_file->seek(realDataPos);
			m_file->read(pBuf, uncompressedSize);
			pthread_mutex_unlock(&mutex);
			return createMemoryReadFile(pBuf, uncompressedSize, fileEntry.fileName, true);
		}
		return 0;
	}
	case 8:
	{
		const unsigned int uncompressedSize = fileEntry.uncompressedSize;
		const unsigned int compressedSize = fileEntry.compressedSize;

		void *pBuf = new char[uncompressedSize];
		if (!pBuf)
		{
			printf("Not enough memory for decompressing %s", fileEntry.fileName);
			return 0;
		}

		char *pcData = new char[compressedSize];
		if (!pcData)
		{
			printf("Not enough memory for decompressing %s", fileEntry.fileName);
			delete[] (char *)pBuf;
			return 0;
		}

		memset(pcData, 0, compressedSize);
		pthread_mutex_lock(&mutex);
		m_file->seek(realDataPos);
		m_file->read(pcData, compressedSize);
		pthread_mutex_unlock(&mutex);

		// Setup the inflate stream.
		z_stream stream;
		int err;

		stream.next_in = (Bytef *)pcData;
		stream.avail_in = (uInt)compressedSize;
		stream.next_out = (Bytef *)pBuf;
		stream.avail_out = uncompressedSize;
		stream.zalloc = (alloc_func)0;
		stream.zfree = (free_func)0;

		// Perform inflation. wbits < 0 indicates no zlib header inside the data.
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
			return createMemoryReadFile(pBuf, uncompressedSize, fileEntry.fileName, true);
	}
	default:
		return 0;
	};
	return 0;
}

//! returns fileindex
int CPackResReader::findFile(const char *simpleFilename)
{
	std::string entryFileName;
	if (simpleFilename[0] == '.' && simpleFilename[1] == '/')
		entryFileName = simpleFilename + 2;
	else
		entryFileName = simpleFilename;

	SPackResFileEntry entry;

	{
		for (unsigned int i = 0; i < entryFileName.length(); ++i)
		{
			char x = entryFileName[i];
			entryFileName[i] = x >= 'A' && x <= 'Z' ? (char)x + 0x20 : (char)x;
		}
	}

	entry.filePathHash = hashStringSimple(entryFileName.c_str());

	int index = -1;

	std::vector<SPackResFileEntry>::const_iterator ite = std::lower_bound(m_fileList.begin(), m_fileList.end(), entry);
	for (; ite != m_fileList.end() && *ite == entry; ++ite)
	{
		const SPackResFileEntry &foundEntry = *ite;

		// Verify the name whether correct, because hash value may be same but string not same
		if (strcasecmp(entryFileName.c_str(), foundEntry.fileName) == 0)
		{
			// move forward length of extra field.
			index = ite - m_fileList.begin();
			break;
		}
	}

	return index;
}

bool CPackResReader::isValid(IReadResFile *file)
{
	unsigned int header = 0;
	int pos = file->getPos();
	file->read(&header, 4);
	file->seek(pos);
	return (header == 0x04034b50 || header == 0x504d4247);
}

bool CPackResReader::isValid(const char *filename)
{
	IReadResFile *readFile = createReadFile(filename);
	bool validation = isValid(readFile);
	readFile->drop();
	return validation;
}

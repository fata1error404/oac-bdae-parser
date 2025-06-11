#include <algorithm>
#include "PackPatchReader.h"

CPackPatchReader::CPackPatchReader(const char *filename, bool ignoreCase, bool ignorePaths)
	: CPackResReader(filename, ignoreCase, ignorePaths)
{
}

CPackPatchReader::CPackPatchReader(IReadResFile *file, bool ignoreCase, bool ignorePaths)
	: CPackResReader(file, ignoreCase, ignorePaths)
{
}

CPackPatchReader::~CPackPatchReader(void)
{
	for (unsigned int i = 0; i < PackPatchFiles.size(); ++i)
		delete PackPatchFiles[i];

	PackPatchFiles.clear();
}

IReadResFile *CPackPatchReader::openFile(const char *filename)
{
	int index = findFile(filename);

	if (index != -1)
	{
		int fileDataPosition = m_fileList[index].fileDataPosition;
		if ((fileDataPosition & 0x80000000) == 0)
			return CPackResReader::openFile(index);
		else
		{
			int zipPatchIndex = (fileDataPosition & 0x7FFF0000) >> 16;
			int zipLocalIndex = fileDataPosition & 0xFFFF;

			return PackPatchFiles[zipPatchIndex]->openFile(zipLocalIndex);
		}
	}

	return NULL;
}

bool CPackPatchReader::addPackPatchFile(const char *filename, bool ignoreCase /* = true*/, bool ignorePaths /*= true*/)
{
	CPackResReader *zr = 0;
	IReadResFile *m_file = createReadFile(filename);
	if (m_file)
	{
		zr = new CPackResReader(m_file, ignoreCase, ignorePaths);
		if (zr)
		{
			PackPatchFiles.push_back(zr);

			// update index in the main Pack file
			int curPatchNo = PackPatchFiles.size() - 1;
			int fileEntryNb = zr->getFileCount();
			bool needSort = false;

			for (int i = 0; i < fileEntryNb; ++i)
			{
				const SPackResFileEntry *fileInfo = zr->getFileInfo(i);
				if (fileInfo == NULL)
					continue;
				int res = this->findFile(fileInfo->fileName);
				if (res < 0)
				{
					needSort = true;
					res = m_fileList.size();
					m_fileList.push_back(*fileInfo);
				}
				this->m_fileList[res].fileDataPosition = 0x80000000 | (curPatchNo << 16) | i;
			}

			if (needSort)
				std::sort(m_fileList.begin(), m_fileList.end());
		}

		m_file->drop();
	}

	return (zr != 0);
}

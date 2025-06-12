#include <algorithm>
#include "ZipPatchReader.h"

CZipPatchReader::CZipPatchReader(const char *filename, bool ignoreCase, bool ignorePaths)
	: CZipResReader(filename, ignoreCase, ignorePaths)
{
}

CZipPatchReader::CZipPatchReader(IReadResFile *file, bool ignoreCase, bool ignorePaths)
	: CZipResReader(file, ignoreCase, ignorePaths)
{
}

CZipPatchReader::~CZipPatchReader(void)
{
	for (unsigned int i = 0; i < ZipPatchFiles.size(); ++i)
		delete ZipPatchFiles[i];

	ZipPatchFiles.clear();
}

/*
	Locates a file in either the main or updated zip archive and opens it.
	If the file exists in the main archive, it's opened normally.
	If it’s in a patch archive, the method decodes its position and opens it from the corresponding patch file.
*/
IReadResFile *CZipPatchReader::openFile(const char *filename)
{
	int index = findFile(filename);

	if (index != -1)
	{
		int fileDataPos = FileList[index].fileDataPosition;
		if ((fileDataPos & 0x80000000) == 0)
			return CZipResReader::openFile(index);
		else
		{
			int zipPatchIndex = (fileDataPos & 0x7FFF0000) >> 16;
			int zipLocalIndex = fileDataPos & 0xFFFF;

			return ZipPatchFiles[zipPatchIndex]->openFile(zipLocalIndex);
		}
	}

	return NULL;
}

/*
	Merges contents of the updated zip archive into the main zip.
	If a file in the patch already exists in the main archive, it updates the reference to the patch version.
	Otherwise, it appends the new entry.
*/
bool CZipPatchReader::addZipPatchFile(const char *filename, bool ignoreCase /* = true*/, bool ignorePaths /*= true*/)
{
	CZipResReader *zr = 0;
	IReadResFile *file = createReadFile(filename);
	if (file)
	{
		zr = new CZipResReader(file, ignoreCase, ignorePaths);
		if (zr)
		{
			ZipPatchFiles.push_back(zr);

			// update index in the main zip
			int curPatchNo = ZipPatchFiles.size() - 1;
			int fileEntryNb = zr->getFileCount();
			bool needSort = false;

			for (int i = 0; i < fileEntryNb; ++i)
			{
				const SZipResFileEntry *fileInfo = zr->getFileInfo(i);
				int res = this->findFile(fileInfo->simpleFileName.c_str());
				if (res < 0)
				{
					needSort = true;
					res = FileList.size();
					FileList.push_back(*fileInfo);
				}
				this->FileList[res].fileDataPosition = 0x80000000 | (curPatchNo << 16) | i;
			}

			if (needSort)
				std::sort(FileList.begin(), FileList.end());
		}

		file->drop();
	}

	return (zr != 0);
}

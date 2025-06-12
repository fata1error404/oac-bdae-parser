#include "CZipResReader.h"

/*
	Extension of the CZipResReader class.
	Aims to support dynamic zip archive updates without the replacing the entire main archive.
	__________________________________________________________________________________________
*/

class CZipPatchReader : public CZipResReader
{
public:
	CZipPatchReader(const char *filename, bool ignoreCase, bool ignorePaths);

	CZipPatchReader(IReadResFile *file, bool ignoreCase, bool ignorePaths);

	virtual ~CZipPatchReader();

	//! locates a file in either the main or updated zip archive and opens it
	virtual IReadResFile *openFile(const char *filename);

	//! merges contents of the updated zip archive into the main zip
	virtual bool addZipPatchFile(const char *filename, bool ignoreCase = true, bool ignorePaths = true);

protected:
	std::vector<CZipResReader *> ZipPatchFiles;
};

#include "CPackResReader.h"

/*
	Extension of the CPackResReader class.
	Aims to support dynamic packed resource file updates without the replacing the entire main file.
	(works the same as implementation for the zip archive)
	________________________________________________________________________________________________
*/

class CPackPatchReader : public CPackResReader
{
public:
	CPackPatchReader(const char *filename, bool ignoreCase, bool ignorePaths);

	CPackPatchReader(IReadResFile *file, bool ignoreCase, bool ignorePaths);

	virtual ~CPackPatchReader();

	virtual IReadResFile *openFile(const char *filename);

	virtual bool addPackPatchFile(const char *filename, bool ignoreCase = true, bool ignorePaths = true);

protected:
	std::vector<CPackResReader *> PackPatchFiles;
};

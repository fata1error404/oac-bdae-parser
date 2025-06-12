#include <iostream>
#include <string>
#include "io/PackPatchReader.h"
#include "resFile.h"

int main()
{
    CPackPatchReader *archiveReader = NULL;

    IReadResFile *archiveFile = createReadFile("city_sky.bdae");

    if (archiveFile)
    {
        archiveReader = new CPackPatchReader(archiveFile, true, false);
        archiveFile->drop();

        IReadResFile *bdaeFile = archiveReader->openFile("little_endian_not_quantized.bdae");

        if (bdaeFile)
        {
            File myFile;
            int result = myFile.Init(bdaeFile);

            std::cout << "\n"
                      << (result != 1 ? "SUCCESS" : "ERROR") << std::endl;
        }

        delete bdaeFile;
    }

    delete archiveReader;

    return 0;
}

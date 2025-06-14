#include <iostream>
#include <iomanip>
#include <string>
#include "io/PackPatchReader.h"
#include "resFile.h"

int main()
{
    CPackPatchReader *archiveReader = NULL;

    IReadResFile *archiveFile = createReadFile("example.bdae"); // city_sky.bdae

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

            if (result != 1)
            {
                FileHeaderData *header = myFile.ptr();
                const char *targetName = "Object01-mesh";

                // std::cout << "\nFILE ORIGIN IN MEMORY: " << header << std::endl;
                // std::cout << "\nOFFSET TABLE\n"
                //           << std::endl;

                for (unsigned int i = 0; i < header->numOffsets; ++i)
                {
                    Access<Access<int> > &offset = header->offsets[i];

                    // std::cout << "[" << i + 1 << "] " << offset.ptr()->ptr() << std::endl; // correct way to access the data
                    // unsigned char *bytes = reinterpret_cast<unsigned char *>(offset.ptr()->ptr());
                    // for (int i = 0; i < 24; ++i)
                    //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";

                    // std::cout << std::dec << "\n\n";

                    // const char *name = reinterpret_cast<const char *>(offset.ptr()->ptr());
                    // std::cout << name << std::endl;

                    // if (name && strcmp(name, targetName) == 0)
                    // {
                    //     // found it—i+1 is your mesh data block
                    //     Access<Access<int> > &dataEntry = header->offsets[i + 1];
                    //     MeshInfo *mi = reinterpret_cast<MeshInfo *>(dataEntry.ptr());
                    //     // …now you can use mi…
                    //     break;
                    // }
                }

                // void *blockPtr = entry.ptr(); // <-- guaranteed to point to the start of that block

                // unsigned char *bytes = reinterpret_cast<unsigned char *>(target);
                // for (int i = 0; i < 24; ++i)
                //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";

                // std::cout << std::dec << "\n";

                // std::cout << "At position " << target - origin << " reading Mesh 1 info: " << sizeof(MeshInfo) << " bytes" << std::endl;
            }
        }

        delete bdaeFile;
    }

    delete archiveReader;

    return 0;
}

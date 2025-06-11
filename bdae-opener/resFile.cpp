#include <iostream>
#include <iomanip>
#include "resFile.h"
#include "io/PackPatchReader.h"

namespace glitch
{
    namespace res
    {
        bool File::ExtractStringTable = true;

        int File::SizeOfHeader = 0;
        int File::ExternalFileOffsetTableSize[2] = {0, 0};
        int File::ExternalFileStringTableSize[2] = {0, 0};
        char *File::ExternalFilePtr[2] = {0, 0};

        //! Reads raw binary data from .bdae file and loads its parsed sections into memory.
        // _________________________________________________________________________________

        int File::Init(IReadResFile *file)
        {
            std::cout << "[Init] Starting File::Init..\n"
                      << std::endl;
            std::cout << "---------------" << std::endl;
            std::cout << "[Init] PART 1. \n       Reading raw binary data from .bdae file and loading its parsed sections into memory." << std::endl;
            std::cout << "---------------\n\n"
                      << std::endl;

            // 1. Read Header data as a structure.
            Size = file->getSize();
            int headerSize = sizeof(struct FileHeaderData);
            struct FileHeaderData *header = new FileHeaderData;

            std::cout << "[Init] Header size (size of struct): " << headerSize << std::endl;
            std::cout << "[Init] File size (length of file): " << Size << std::endl;
            std::cout << "\n[Init] At position " << file->getPos() << ", reading header.." << std::endl;

            int readSize = file->read(header, headerSize);

            std::cout << "___________________" << std::endl;
            std::cout << "\nFileHeaderData\n"
                      << std::endl;
            std::cout << "Signature: " << std::hex << ((char *)&header->signature)[0] << ((char *)&header->signature)[1] << ((char *)&header->signature)[2] << ((char *)&header->signature)[3] << std::dec << std::endl;
            std::cout << "Endian check: " << header->endianCheck << std::endl;
            std::cout << "Version: " << header->version << std::endl;
            std::cout << "Header size: " << header->sizeOfHeader << std::endl;
            std::cout << "File size: " << header->sizeOfFile << std::endl;
            std::cout << "Number of offsets: " << header->numOffsets << std::endl;
            std::cout << "Origin: " << header->origin << std::endl;
            std::cout << "\nSection offsets  " << std::endl;
            std::cout << "Offset Data:   " << header->offsets.m_offset << std::endl;
            std::cout << "String Data:   " << header->stringData.m_offset << std::endl;
            std::cout << "Data:          " << header->data.m_offset << std::endl;
            std::cout << "Related files: " << header->relatedFiles.m_offset << std::endl;
            std::cout << "Removable:     " << header->removable.m_offset << std::endl;
            std::cout << "\nSize of Removable Chunk: " << header->sizeOfRemovableChunk << std::endl;
            std::cout << "Number of Removable Chunks: " << header->nbOfRemovableChunks << std::endl;
            std::cout << "Use separated allocation: " << ((header->useSeparatedAllocationForRemovableBuffers > 0) ? "Yes" : "No") << std::endl;
            std::cout << "Size of Dynamic Chunk: " << header->sizeOfDynamicChunk << std::endl;
            std::cout << "________________________\n"
                      << std::endl;

            // 2. Search for related files.
            unsigned int beginOfRelatedFiles = header->relatedFiles.m_offset - header->origin;

            if (header->origin == 0)
            {
                std::cout << "[Init] At position " << beginOfRelatedFiles << ", checking for related filenames.." << std::endl;

                // read name size of the related file
                int sizeOfName = 0;
                file->seek(beginOfRelatedFiles);
                file->read(&sizeOfName, 4);

                unsigned char *bytes = reinterpret_cast<unsigned char *>(&sizeOfName);
                std::cout << "[Init] Size of related filename: ";
                for (int i = 0; i < 4; ++i)
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";

                std::cout << std::dec;
                std::cout << "(" << sizeOfName << " byte)" << std::endl;

                // validity check: name size should not exceed the limit for filename length
                if (sizeOfName > 256)
                    std::cout << "[Init] Warning: sizeOfName exceeds buffer size!" << std::endl;

                // validity check: name is real (size 1 means none)
                if (sizeOfName > 1)
                {
                    // read name of the related file
                    beginOfRelatedFiles += 4;
                    char relatedFileName[256];
                    file->seek(beginOfRelatedFiles);
                    file->read(relatedFileName, (sizeOfName + 3) & ~3); // align to next 4 bytes, as name size may not be a multiple of 4

                    std::cout << "[Init] Filename: " << relatedFileName << std::endl;

                    // load related bdae file (?)
                    // collada::CResFileManager::getInst()->get(buff, NULL, true);
                }
                else
                    std::cout << "[Init] Invalid name. No related files found."
                              << std::endl;
            }

            // 3. Initialize File struct variables and allocate memory for reading rest of the file.
            int sizeOffsetTable;
            int sizeStringTable;
            int sizeDynamicContent;

            sizeOffsetTable = header->numOffsets * sizeof(unsigned long);
            sizeStringTable = (ExtractStringTable ? header->data.m_offset - header->stringData.m_offset : 0);
            SizeRemovableBuffer = header->sizeOfRemovableChunk;
            sizeDynamicContent = header->sizeOfDynamicChunk;

            SizeUnRemovable = Size - sizeOffsetTable - sizeStringTable - SizeRemovableBuffer - sizeDynamicContent;
            NbRemovableBuffers = header->nbOfRemovableChunks;
            UseSeparatedAllocationForRemovableBuffers = (header->useSeparatedAllocationForRemovableBuffers > 0) ? true : false;

            char *offsetBuffer = new char[sizeOffsetTable];                               // temp buffer for offset table
            char *stringBuffer = (ExtractStringTable ? new char[sizeStringTable] : NULL); // temp buffer for string table
            char *buffer = (char *)malloc(SizeUnRemovable);                               // main buffer

            memcpy(buffer, header, headerSize); // copy header

            // 4. Read offset, string, data and related files sections as a raw binary data.
            file->seek(headerSize);
            std::cout << "\n[Init] At position " << file->getPos() << ", reading offset " << (sizeStringTable ? "and string tables.." : "table..") << std::endl;

            file->read(offsetBuffer, sizeOffsetTable);

            if (sizeStringTable)
                file->read(stringBuffer, sizeStringTable);

            std::cout << "\n[Init] At position " << file->getPos() << ", reading rest of the file (up to the removable section).." << std::endl;
            file->read(&buffer[headerSize], SizeUnRemovable - headerSize); // insert after header

            // 5. Read removable chunks.
            RemovableBuffers = NULL;

            if (SizeRemovableBuffer > 0)
            {
                // read size / offset pairs for each removable chunk
                std::cout << "\n[Init] At position " << file->getPos() << ", reading removable section info.." << std::endl;
                RemovableBuffersInfo = new unsigned int[NbRemovableBuffers * 2];
                file->read(RemovableBuffersInfo, NbRemovableBuffers * 2 * sizeof(unsigned int));

                // read chunks data
                std::cout << "[Init] At position " << file->getPos() << ", reading removable section data.." << std::endl;
                RemovableBuffers = new void *[NbRemovableBuffers];

                if (UseSeparatedAllocationForRemovableBuffers)
                {
                    // separated allocation mode: read each chunk into its own buffer
                    for (int i = 0; i < NbRemovableBuffers; ++i)
                    {
                        unsigned int bufSize = RemovableBuffersInfo[i * 2];
                        RemovableBuffers[i] = new char[bufSize];
                        file->read(RemovableBuffers[i], bufSize);
                    }
                }
                else
                {
                    // single-block mode: read all chunks into one large buffer

                    /*
                        RemovableBuffers[0]             → pointer to the entire data block
                        RemovableBuffers[i] (for i > 0) → pointer into that block at the start of chunk i
                    */
                    unsigned int totalDataSize = SizeRemovableBuffer - (NbRemovableBuffers * 2 * sizeof(unsigned int));
                    RemovableBuffers[0] = new char[totalDataSize];
                    file->read(RemovableBuffers[0], totalDataSize);

                    unsigned int baseOffset = RemovableBuffersInfo[1];

                    // assign pointers
                    for (int i = 1; i < NbRemovableBuffers; ++i)
                    {
                        unsigned int chunkOffset = RemovableBuffersInfo[i * 2 + 1];
                        RemovableBuffers[i] = (char *)RemovableBuffers[0] + (chunkOffset - baseOffset);
                    }
                }
            }

            std::cout << "[Init] Stopped reading " << file->getFileName() << " at position " << file->getPos() << " (end of file)." << std::endl;

            delete header;

            // run the real init (apply offset correction and do string extraction from offset and string tables)
            *this = File(buffer,
                         RemovableBuffersInfo,
                         RemovableBuffers,
                         UseSeparatedAllocationForRemovableBuffers,
                         offsetBuffer,
                         stringBuffer);

            delete[] offsetBuffer;
            OffsetTable = NULL;

            delete[] stringBuffer;
            StringTable = NULL;

            delete[] RemovableBuffersInfo;
            RemovableBuffersInfo = NULL;

            return IsValid != 1;
        }

        //! Ensures that all offset pointers within the loaded binary data correctly point to usable memory, handling both internal and external layouts, string tables, and removable chunks.
        int File::Init()
        {
            // 6. Prepare for file processing: retrieve the Header struct from memory and initialize File struct variables (we replaced the File object with a new one by calling the second Init(), so this is the actual initialization).
            res::FileHeaderData *header = ptr();

            Size = header->sizeOfFile;

            SizeOffsetStringTables = 0;

            if (OffsetTable)
                SizeOffsetStringTables += header->numOffsets * sizeof(unsigned long);
            if (StringTable && ExtractStringTable)
                SizeOffsetStringTables += header->data.m_offset - header->stringData.m_offset;

            SizeDynamic = header->sizeOfDynamicChunk;
            SizeUnRemovable = Size - SizeRemovableBuffer - SizeDynamic;

            /*  save the file origin pointer into a different array index based on whether it's an internal or external .bdae file
                (highest bit of the origin field is used as a flag):

                0 → internal – self-contained, i.e all data (model, textures, animations, etc.) is packed inside one single file
                1 → external – split into multiple related files
            */
            ExternalFilePtr[(header->origin) >> 31] = reinterpret_cast<char *>(header);

            // validity check: file signature
            if (((char *)&header->signature)[0] != 'B' ||
                ((char *)&header->signature)[1] != 'R' ||
                ((char *)&header->signature)[2] != 'E' ||
                ((char *)&header->signature)[3] != 'S')
            {
                std::cout << "[Init] Warning: wrong signature!" << std::endl;
                return -1;
            }

            // validity check: proceed only if the header is valid and the file hasn't already been marked as processed (highest bit of the version field is used as a flag)
            if (header && (header->version & 0x8000) == 0)
            {
                header->version |= 0x8000; // set the high bit of the version by doing a bitwise OR with 0x8000

                // 7a. Using a temporary separate deletable buffer for the offset table. <explain what happens here..>
                if (OffsetTable)
                {
                    SizeOfHeader = header->sizeOfHeader;
                    (&header->offsets)[0] = OffsetTable; // override the address of the offset table in the Header struct to point to the temp buffer for offset table (this allows to perform all pointer fix-ups against our own copy and safely free or reallocate it without touching the original memory block mapped from the .bdae file)

                    // sizes and pointers of the offset / string tables
                    int sizeOffsetTable = header->numOffsets * sizeof(unsigned long);
                    int sizeStringTable = (ExtractStringTable ? header->data.m_offset - header->stringData.m_offset : 0);
                    unsigned int offsetTableEnd = sizeOffsetTable + SizeOfHeader;
                    unsigned int stringTableEnd = ExtractStringTable ? offsetTableEnd + sizeStringTable : offsetTableEnd;
                    ExternalFileOffsetTableSize[(header->origin) >> 31] = offsetTableEnd;
                    ExternalFileStringTableSize[(header->origin) >> 31] = stringTableEnd;
                    char *stringTableStartPtr = (char *)StringTable;

                    // loop through each entry in the offset table
                    for (unsigned int i = 0; i < header->numOffsets; ++i)
                    {
                        res::Access<res::Access<int> > &offset = header->offsets[i];

                        char *origin = reinterpret_cast<char *>(header);                                       // pointer to the start of the current file’s memory block
                        unsigned int originoff = header->origin;                                               // base offset used as a reference to resolve relative offsets in the file
                        unsigned int offptr = reinterpret_cast<unsigned int>(offset.ptr()) - (header->origin); // relative offset from the file’s origin to the target pointer of this entry
                        unsigned int ote = offsetTableEnd;
                        unsigned int ste = stringTableEnd;
                        bool external = false; // flag to indicate if this entry refers to an external memory chunk (which does not mean the entire .bdae file is external)

                        // if this offset lies beyond the bounds of the current file → it refers to an external file/chunk.
                        if (offptr > (unsigned int)Size)
                        {
                            origin = ExternalFilePtr[offptr >> 31];
                            originoff = ((offptr >> 31) << 31);
                            offptr += header->origin;
                            ote = ExternalFileOffsetTableSize[offptr >> 31];
                            ste = ExternalFileStringTableSize[offptr >> 31];
                            external = true;
                        }

                        std::vector<std::string> StringStorage;

                        // if the location before the normal offset table location, don't apply the location correction.
                        if (offptr >= ote)
                        {
                            if (offptr < stringTableEnd && StringTable)
                            {
                                // 1. Get the raw pointer to the string data
                                char *rawPtr = stringTableStartPtr + (offptr - ote);

                                // 2. Read the length prefix just before the string
                                unsigned int size = *reinterpret_cast<unsigned int *>(rawPtr - sizeof(unsigned int));

                                // 3. Copy the string data into std::string (null terminator handled automatically)
                                std::string str(rawPtr, size);

                                // 4. Store the string (copy-based, for old compilers)
                                StringStorage.push_back(str);

                                // 5. Get pointer to stored string
                                const char *cstr = StringStorage.back().c_str();

                                // 6. Wrap in your Access object
                                res::Access<res::Access<int> > newPtr(
                                    const_cast<void *>(static_cast<const void *>(cstr)));
                                offset = newPtr;
                            }
                            else if (offptr > (unsigned int)SizeUnRemovable)
                            {
                                int nb = ((offptr - SizeUnRemovable) - sizeof(unsigned int)) / (sizeof(unsigned int) * 2);
                                if (nb > NbRemovableBuffers)
                                {
                                    int nb1 = 0;
                                    while (nb1 < NbRemovableBuffers - 1)
                                    {
                                        if (offptr > RemovableBuffersInfo[nb1 * 2 + 1] &&
                                            offptr < RemovableBuffersInfo[nb1 * 2 + 3])
                                        {
                                            break;
                                        }
                                        nb1++;
                                    }
                                    //							offset.OffsetToPtr((char*)RemovableBuffers[nb1] - RemovableBuffersInfo[nb1 * 2 + 1]);
                                    offset.OffsetToPtr((char *)(intptr_t(RemovableBuffers[nb1]) - intptr_t(RemovableBuffersInfo[nb1 * 2 + 1])));
                                    // offset.OffsetToPtr((char*)((char*)RemovableBuffers[nb1] - (char*)RemovableBuffersInfo[nb1 * 2 + 1]));
                                    unsigned int offptrptr = reinterpret_cast<unsigned int>(offset.ptr()->ptr()) - (header->origin);
                                    if (offptrptr > (unsigned int)SizeUnRemovable)
                                    {
                                        int nb2 = 0;
                                        while (nb2 < NbRemovableBuffers - 1)
                                        {
                                            if (offptrptr > RemovableBuffersInfo[nb2 * 2 + 1] &&
                                                offptrptr < RemovableBuffersInfo[nb2 * 2 + 3])
                                            {
                                                break;
                                            }
                                            nb2++;
                                        }
                                        offset.ptr()->OffsetToPtr((char *)(intptr_t(RemovableBuffers[nb2]) - intptr_t(RemovableBuffersInfo[nb2 * 2 + 1])));
                                        // offset.ptr()->OffsetToPtr((char*)((char*)RemovableBuffers[nb2] - (char*)RemovableBuffersInfo[nb2 * 2 + 1] ));
                                        continue;
                                    }
                                    //							offset.OffsetToPtr((char*)RemovableBuffersInfo[nb * 2 + 1] - offptr);
                                }
                                else
                                {
                                    //							offset.OffsetToPtr((char*)RemovableBuffersInfo[nb * 2 + 1] - offptr);
                                    continue;
                                }
                            }
                            else
                            {
                                offset.OffsetToPtr(origin - (ste - SizeOfHeader) - originoff);
                            }
                        }
                        else
                        {
                            offset.OffsetToPtr(origin - originoff);
                        }

                        // if (external)
                        // {
                        //     continue;
                        // }
                        // if (i > 0)
                        // {
                        //     // Ignore the offset table's length as it's already been extracted
                        //     // if the location befor the normal offset table location, don't apply the location correction.
                        //     unsigned int offptrptr = reinterpret_cast<unsigned int>(offset.ptr()->ptr()) - (header->origin);
                        //     origin = reinterpret_cast<char *>(p);
                        //     originoff = header->origin;
                        //     ote = offsetTableEnd;
                        //     if (offptrptr > (unsigned int)Size)
                        //     {
                        //         offptrptr += header->origin;
                        //         origin = ExternalFilePtr[offptrptr >> 31];
                        //         originoff = (offptrptr >> 31) << 31;
                        //         ote = ExternalFileOffsetTableSize[offptrptr >> 31];
                        //         ste = ExternalFileStringTableSize[offptrptr >> 31];
                        //     }

                        //     if (offptrptr >= ote)
                        //     {
                        //         if (offptrptr != ote && offptrptr < stringTableEnd)
                        //         {
                        //             // make sure we have a null terminated string
                        //             // unsigned int size = *((unsigned int *)(stringTableStartPtr + (offptrptr - ote)) - 1);
                        //             // core::SScopedProcessArray<char> buffer(size + 1);
                        //             // memcpy(buffer.get(), stringTableStartPtr + (offptrptr - ote), size);
                        //             // buffer[size] = 0;

                        //             // // Get or create the strin in our Shared string
                        //             // // collection and keep a ref locally (for RefCounting)
                        //             // glitch::core::SSharedString sharedString(buffer.get());
                        //             // StringReferenceMap.push_back(sharedString);

                        //             // res::Access<int> newPtr(const_cast<void *>((const void *)sharedString.get()));

                        //             char *rawPtrX = stringTableStartPtr + (offptrptr - ote);

                        //             // read the length prefix
                        //             unsigned int size = *reinterpret_cast<unsigned int *>(rawPtrX - sizeof(unsigned int));

                        //             // copy into std::string (auto null-terminated)
                        //             std::string str(rawPtrX, size);

                        //             // keep it alive
                        //             StringStorage.push_back(str);

                        //             // get stable C-string pointer
                        //             const char *cstr = StringStorage.back().c_str();

                        //             // build new Access pointing at it
                        //             // res::Access<res::Access<int> > newPtr(
                        //             //     const_cast<void *>(static_cast<const void *>(cstr)));

                        //             // *(offset.ptr()) = newPtr;
                        //             res::Access<int> *inner = static_cast<res::Access<int> *>(offset.ptr());
                        //             *inner = res::Access<int>(const_cast<void *>(static_cast<const void *>(cstr)));
                        //         }
                        //         else if (offptrptr > (unsigned int)SizeUnRemovable)
                        //         {
                        //             int nb = 0;

                        //             while (nb < NbRemovableBuffers)
                        //             {
                        //                 if (offptrptr == RemovableBuffersInfo[nb * 2 + 1])
                        //                 {
                        //                     break;
                        //                 }
                        //                 nb++;
                        //             }
                        //             offset.ptr()->OffsetToPtr((char *)RemovableBuffers[nb] - offptrptr + sizeof(int));
                        //         }
                        //         else
                        //         {
                        //             offset.ptr()->OffsetToPtr(origin - (ste - SizeOfHeader) - originoff);
                        //         }
                        //     }
                        //     else
                        //     {
                        //         offset.ptr()->OffsetToPtr(origin - originoff);
                        //     }
                        // }
                    }
                }
                /* 7b. This occurs when a temporary buffer is not used. The offset table is in-place — directly in the file’s main memory buffer — no separate deletable buffer (OffsetTable == NULL), so no need to retrieve or correct anything. Simply convert offsets to pointers. */
                else
                {
                    // if the offset table is in-place, then the string table must be as well
                    if (StringTable != NULL)
                    {
                        std::cerr << "Error: StringTable must be NULL\n";
                        std::abort();
                    }

                    // loop through each entry in the offset table
                    for (unsigned int i = 0; i < header->numOffsets; ++i)
                    {
                        res::Access<res::Access<int> > &offset = header->offsets[i];

                        offset.OffsetToPtr(header); // resolve outer pointer (convert offset[i] from a relative offset into a direct pointer)

                        if (i > 0)
                            offset.ptr()->OffsetToPtr(header); // resolve inner pointer (all entries (except 0, which is the header itself) may point to more data, so convert that too)
                    }
                }
            }

            return 0;
        }
    }
}

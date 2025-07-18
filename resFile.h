#ifndef __RESFILE_H_INCLUDED__
#define __RESFILE_H_INCLUDED__

#include <string.h>
#include "access.h"
#include "libs/io/CPackResReader.h"

struct FileHeaderData
{
    unsigned int signature;
    unsigned short endianCheck;
    unsigned short version;
    unsigned int sizeOfHeader;
    unsigned int sizeOfFile;
    unsigned int numOffsets;
    unsigned int origin;
    Access<Access<Access<int>>> offsets;
    Access<int> stringData;
    Access<char> data;
    Access<char> relatedFiles;
    Access<char> removable;
    unsigned int sizeOfRemovableChunk;
    unsigned int nbOfRemovableChunks;
    unsigned int useSeparatedAllocationForRemovableBuffers;
    unsigned int sizeOfDynamicChunk;
};

/*
    We end up with a fully–populated File object whose entire .bdae payload is in memory (header + offset table + string table + data + removable chunks), with every offset “fix‑up” to real C++ pointers and all embedded strings pulled out into shared‐string instances.
*/

struct File : public Access<FileHeaderData>
{
    int Size;
    int SizeUnRemovable;
    int SizeOffsetStringTables;
    void **RemovableBuffers;
    unsigned long *RemovableBuffersInfo;
    int SizeRemovableBuffer;
    int NbRemovableBuffers;
    bool UseSeparatedAllocationForRemovableBuffers;
    int SizeDynamic;
    std::vector<std::string> StringStorage;

    // bad: these static variables prevent multi-threaded loading..
    static char *ExternalFilePtr[2];
    static int ExternalFileOffsetTableSize[2];
    static int ExternalFileStringTableSize[2];
    static int SizeOfHeader;
    static bool ExtractStringTable;

    bool IsValid;
    void *DataBuffer;
    void *OffsetTable;
    void *StringTable;

    File() : IsValid(false), OffsetTable(NULL) {}

    File(void *ptr, unsigned long *removableBuffersInfo = 0, void **removableBuffers = 0, bool useSeparatedAllocationForRemovableBuffers = false, void *offsetTable = NULL, void *stringTable = NULL)
        : Access<FileHeaderData>(ptr), DataBuffer(ptr), IsValid(false), OffsetTable(offsetTable), StringTable(stringTable), RemovableBuffersInfo(removableBuffersInfo), RemovableBuffers(removableBuffers), UseSeparatedAllocationForRemovableBuffers(useSeparatedAllocationForRemovableBuffers)
    {
        if (ptr)
            IsValid = (Init() == 0);
    }

    int Init(IReadResFile *file);

    int Init();

    int Init(void *ptr)
    {
        *this = File(ptr, NULL);
        return IsValid != 1;
    }

    int getSize() const { return Size; }

    int getMemoryUsage() const
    {
        int size = Size - SizeDynamic - SizeOffsetStringTables;
        if (RemovableBuffers == NULL)
            size -= SizeRemovableBuffer;
        return size;
    }

    bool isValid() const { return IsValid; }

    bool hasDynamicContent() const { return SizeDynamic != 0; }
};

#endif

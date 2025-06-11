#ifndef __RESFILE_H_INCLUDED__
#define __RESFILE_H_INCLUDED__

#include <string.h>
#include "access.h"
#include "io/CPackResReader.h"
//  #include <glitch/core/SSharedString.h>

namespace glitch
{
    namespace res
    {
        //
        enum E_REMOVABLE_BUFFER_TYPE
        {
            ERBT_GEOMETRY = 0x1,
            ERBT_SKINNING = 0x2
        };

        //
        template <typename T>
        struct Data
        {
            T *ptr;

            operator T *() { return ptr; }
            operator const T *() const { return ptr; }

            size_t GetSize() const
            {
                size_t s = *(reinterpret_cast<const size_t *>(ptr) - 1);
                return s;
            }
        };

        //
        struct String
        {
            const char *ptr;

            bool operator==(const String &str2) const { return (strcmp(ptr, str2.ptr) == 0); }

            bool operator!=(const String &str2) const { return (strcmp(ptr, str2.ptr) != 0); }

            bool operator==(const char *str2) const { return (strcmp(ptr, str2) == 0); }

            bool operator!=(const char *str2) const { return (strcmp(ptr, str2) != 0); }

            operator const char *() const { return ptr; }

            size_t GetSize() const { return strlen(ptr); }
        };

        //
        template <typename T>
        struct Buffer
        {
            size_t size;
            T data[1];

            T &operator[](int idx) { return data[idx]; }

            const T &operator[](int idx) const { return data[idx]; }
        };

        // struct RawFileHeaderData
        // {
        //     unsigned int signature;                                 // 4
        //     unsigned short endianCheck;                             // 2
        //     unsigned short version;                                 // 2
        //     unsigned int sizeOfHeader;                              // 4
        //     unsigned int sizeOfFile;                                // 4
        //     unsigned int numOffsets;                                // 4
        //     unsigned int origin;                                    // 4
        //     unsigned long offsets;                                  // 4
        //     unsigned long stringData;                               // 4
        //     unsigned long data;                                     // 4
        //     unsigned long relatedFiles;                             // 4
        //     unsigned long removable;                                // 4
        //     unsigned int sizeOfRemovableChunk;                      // 4
        //     unsigned int nbOfRemovableChunks;                       // 4
        //     unsigned int useSeparatedAllocationForRemovableBuffers; // 4
        //     unsigned int sizeOfDynamicChunk;                        // 4
        // };

        struct FileHeaderData
        {
            unsigned int signature;
            unsigned short endianCheck;
            unsigned short version;
            unsigned int sizeOfHeader;
            unsigned int sizeOfFile;
            unsigned int numOffsets;
            unsigned int origin;
            res::Access<res::Access<res::Access<int> > > offsets;
            res::Access<int> stringData;
            res::Access<char> data;
            res::Access<char> relatedFiles;
            res::Access<char> removable;
            unsigned int sizeOfRemovableChunk;
            unsigned int nbOfRemovableChunks;
            unsigned int useSeparatedAllocationForRemovableBuffers;
            unsigned int sizeOfDynamicChunk;
        };

        //
        struct NodeData
        {
            unsigned int size;
            unsigned int numNodes;

            struct ResNodeDicData
            {
                int ref;
                res::Access<const char> name;
                res::Access<char> ptr;
            } data[1];
        };

        struct CNode : public res::Access<res::NodeData>
        {
            unsigned int GetNumNodes() const;
            res::CNode GetResNode(const char *name);
        };

        /*
            You end up with a fully–populated File object whose entire .bdae payload is in memory (header + offset table + string table + data + removable chunks), with every offset “fix‑up” to real C++ pointers and all embedded strings pulled out into shared‐string instances.
        */

        struct File : public res::Access<res::FileHeaderData>
        {
            int SizeRemovableBuffer;
            int NbRemovableBuffers;
            int SizeUnRemovable;                            // SET (RE-UPDATE)
            int SizeOffsetStringTables;                     // SET
            unsigned int *RemovableBuffersInfo;             // NEW (argument)
            void **RemovableBuffers;                        // NEW (argument)
            bool UseSeparatedAllocationForRemovableBuffers; // NEW (argument)
            int SizeDynamic;                                // SET

            // BAD: these static variables prevent multi-threaded loading..
            static char *ExternalFilePtr[2];
            static int ExternalFileOffsetTableSize[2];
            static int ExternalFileStringTableSize[2];
            static int SizeOfHeader;
            static bool ExtractStringTable;

            bool IsValid;
            void *OffsetTable; // NEW (argument)
            void *StringTable; // NEW (argument)
            int Size;          // SET

            // std::vector<glitch::core::SSharedString> StringReferenceMap;

            File() : IsValid(false),
                     OffsetTable(NULL) {}

            File(void *ptr, unsigned int *removableBuffersInfo = 0, void **removableBuffers = 0, bool useSeparatedAllocationForRemovableBuffers = false, void *offsetTable = NULL, void *stringTable = NULL)
                : res::Access<res::FileHeaderData>(ptr), IsValid(false), OffsetTable(offsetTable), StringTable(stringTable), RemovableBuffersInfo(removableBuffersInfo), RemovableBuffers(removableBuffers), UseSeparatedAllocationForRemovableBuffers(useSeparatedAllocationForRemovableBuffers)
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

            bool isValid() const { return IsValid; }

            int getSize() const { return Size; }

            bool hasDynamicContent() const { return SizeDynamic != 0; }

            int getMemoryUsage() const
            {
                int size = Size - SizeDynamic - SizeOffsetStringTables;
                if (RemovableBuffers == NULL)
                    size -= SizeRemovableBuffer;
                return size;
            }
        };

    }
}

#endif

# Input / Output Library

Developer: __Gameloft__ (module of the Glitch Engine)  
Complete: __No__. This is a shortened version of the library with unnecessary files removed.

__Resource files__ store game assets like textures, sounds, models, and scripts used during gameplay. This library provides an interface for reading these files from various sources (disk, memory, custom Pack, ZIP archives) with efficient memory management and reference counting.

Gameloft has their own __custom packed resource format__ (referred to as *Pack* files), which is a variation of a ZIP archive. It compresses file metadata into a compact structure *SPackResFileEntry* to reduce memory overhead when managing many files.

Library file overview:

- `IReadResFile` – interface declaration for reading resource files.
- `CPackResReader` – implementation of *IReadResFile* for reading data from a Pack file.
- `CZipResReader` – implementation of *IReadResFile* for reading data from a ZIP archive and loading it in the memory for subsequent access.
- `ResReferenceCounted` – utility for tracking how many parts of the program are using a resource (when the count drops to zero, meaning nothing is using it, the resource is safely deleted to free memory).
- `PackPatchReader` – extension of *CPackResReader* to support dynamic updates of Pack files by applying patches without replacing the main file.

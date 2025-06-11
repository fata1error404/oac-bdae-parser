CResFilePtr
CResFileManager::get(const char *filename, collada::CColladaDatabase *maindb, bool load)
{
    CResFilePtr resFile;
    const core::stringc absPath = Device->getFileSystem()->getAbsolutePath(filename);
    {
        // First try with a read lock
        glf::ScopedReadLock lock(ReadWriteLock);
        resFile = get(absPath.c_str());
        if (resFile)
        {
            return resFile;
        }
    }

    io::IReadFilePtr file;
    {
#ifndef GLITCH_IS_SINGLE_THREADED
        glf::ScopedWriteLock lock(ReadWriteLock);
        // Second try with a write lock to gain exclusivity
        // We have to be sure that no other thread added the resFile in between the 2 lock
        // So we do one more search
        resFile = get(absPath.c_str());
        if (resFile)
        {
            return resFile;
        }
#endif
        if (load)
        {
            // Get File Accessor
            file = Device->getFileSystem()->createAndOpenFile(absPath.c_str());

            if (file == NULL)
            {
                glf::Console::Println("- Error - File not found   -");
                glf::Console::Println(absPath.c_str());
                glf::Console::Println("----------------------------");
                return NULL;
            }

            // Allocate the res::File
            resFile = irrnew CResFile(absPath.c_str(), file, false);

            if (resFile)
            {
                // Store the res::File
                FileMap[absPath.c_str()] = resFile;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Post load process
    int resu = 0;
    if (resFile->ResFile.ptr()->origin == 0)
    {
        io::IReadFilePtr readfile = getReadFile(file);
        resu = postLoadProcess(resFile, maindb, readfile);
    }
    if (resu)
    {
        unload(absPath.c_str());
        return 0;
    }

    return resFile;
}
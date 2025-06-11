#ifndef __RESFILE_RESFILESTATS_H__
#define __RESFILE_RESFILESTATS_H__

#include <iostream>
#include <string>
#include <map>
#include <fstream>

std::string executablePath;
std::map<std::string, long long> stats;

//! returns path to the stats file, which is located in the same folder as the executable
std::string getPath()
{
    std::string statsPath = executablePath.substr(0, executablePath.find_last_of("/"));
    statsPath += "/stats.txt";
    return statsPath;
}

//! saves path to the executable file that called this function
void setExecutablePath(const char *argv0) { executablePath = std::string(argv0); }

//! adds a new entry to the stats map global variable: a key-value pair (probably a resource file ID and the number of times the current executable interacted with this resource)
void addStats(const char *id, unsigned int count) { stats[id] += count; }

//! opens stats file for reading and appends its contents to the stats map variable
void readStats()
{
    std::ifstream in(getPath().c_str());

    if (in.is_open())
    {
        long long value = 0;
        std::string id;

        while (!in.eof())
        {
            in >> value;
            in >> id;
            if (!in.eof())
                stats[id] += value;
        }
    }
}

//! opens stats file for writing (creates it if it doesn't exist) and writes the contents of the stats map variable to it
void writeStatsToFile()
{
    std::ofstream out(getPath().c_str());

    for (std::map<std::string, long long>::iterator it = stats.begin(); it != stats.end(); ++it)
        out << it->second << " " << it->first << std::endl;
}

#endif

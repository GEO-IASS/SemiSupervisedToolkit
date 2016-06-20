#include "htkfeatio.h"
#include "File.h"

using namespace std;
using namespace Microsoft::MSR::CNTK;
typedef msra::asr::htkfeatreader::parsedpath ppath;


class SCPFileParser
{
public:
    SCPFileParser(){}
    bool Parse(string fileName)
    {
        File fh(fileName, FileOptions::fileOptionsRead | FileOptions::fileOptionsText);
        wstring wline;
        string line;
        size_t conflicted = 0;
        size_t lineIdx = 0;
        while (!fh.IsEOF())
        {
            lineIdx++;
            fh.GetLine(line);
            wline = msra::strfun::utf16(line);
            if (wline == L"") continue;
            ppath p(wline);
            wstring name = (wstring)p;
            string baseName=msra::strfun::utf8(File::FileNameOf(name));
            size_t dotpos = baseName.find_last_of(".");
            if (dotpos == string::npos)
            {
                continue;
            }
            baseName=baseName.substr(0, dotpos);
            if (m_fileRecord.find(baseName) == m_fileRecord.end())
            {
                m_fileRecord.insert(make_pair(baseName, p));
            }
            else
            {
                fprintf(stderr, "WARNING: file (%ls) is conflicted with others -- it will be ignored.\n", name.c_str());
                conflicted++;
            }
            if (lineIdx % 5000 == 0)
            {
                fprintf(stderr, "Loading %d entry in %s\n", lineIdx, fileName.c_str());
            }
        }
        fprintf(stderr, "Loading %d lines from %s -- %d uuid conflicted , %d loaded\n", lineIdx, fileName.c_str(), conflicted, m_fileRecord.size());
        return true;
    }

    ppath QueryUtterance(string uuid, bool& found)
    {
        if (m_fileRecord.find(uuid) != m_fileRecord.end())
        {
            found = true; 
            return m_fileRecord[uuid];
        }
        else
        {
            found = false;
            return ppath();
        }
    }

private:
    map<string, ppath>  m_fileRecord;

};
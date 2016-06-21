#include "File.h"
#include <vector>
#include <regex>

using namespace std;
using namespace Microsoft::MSR::CNTK;

class ConfSegments;

class ConfSegment
{
private:
    float _st;
    float _dur;
    float _confScore;
    bool  _correct;
public:
    ConfSegment(){}
    bool Parse(string input)
    {
        vector<string> array = msra::strfun::split(input, ";");
        if (array.size() != 5)
        {
            RuntimeError("ERROR: cannot parse this segment: %s", input.c_str());
        }
        _confScore = std::stof(array[1]);
        _st = std::stof(array[2]);
        _dur = std::stof(array[3]);
        _correct = std::stoi(array[4]) == 1 ? true : false;
        return true;
    }

    int GetStartFrame()
    {
        return (int)(_st * 100);
    }
    int GetEndFrame()
    {
        return (int)((_st + _dur) * 100)-1;
    }
    float GetConfScore()
    {
        return _confScore;
    }
    bool isCorrect()
    {
        return _correct;
    }

};
class ConfSegments : public vector<ConfSegment>
{
public:
    ConfSegments(){}
    bool Parse(string record, size_t expectedFiled, size_t nameId, size_t confId, string& name)
    {
        vector<string> array = split(record, "\t");
        if (array.size() != expectedFiled)
        {
            return false;
        }
        string uuid;
        {
            string uttName = array[nameId];
            vector<string> elems = msra::strfun::split(uttName, "\\/");
            uuid = elems[elems.size() - 1];
        }
        name = uuid;
        {
            string confString = array[confId];
            if (confString == "")
            {
                // no output 
                return false;
            }
            vector<string> elems = split(confString, "##");
            for (auto& e : elems)
            {
                ConfSegment seg;
                if (seg.Parse(e))
                    push_back(std::move(seg));
            }
        }

        return true;
    }

    int LastFrameIndex()
    {
        return back().GetEndFrame();
    }
private:
    vector<string> split(const string& input, const string& regex)
    {
        std::regex reg(regex);
        vector<string> res;
        sregex_token_iterator it(input.begin(), input.end(), reg, -1);
        sregex_token_iterator end;
        for (; it != end; it++)
        {
            res.push_back(it->str());
        }
        return res;
    }
};
class TSVFileParser
{
private:
    map<string, ConfSegments> m_confScoresPerUtterances;

public:
    bool Parse(string fileName)
    {
        File fh(fileName, FileOptions::fileOptionsRead | FileOptions::fileOptionsText);
        size_t lineIdx = 0; 
        size_t failed2Parse = 0;
        map<string, size_t>  filedName2Id;
        size_t nameId = 0; 
        size_t confId = 0; 

        while (!fh.IsEOF())
        {
            string line;
            fh.GetLine(line);
            if (lineIdx == 0)
            {
                vector<string> fileds = msra::strfun::split(line, "\t");
                size_t id = 0; 
                for (auto f: fileds)
                {
                    filedName2Id[f] = id++;
                }
                if (filedName2Id.find("fileName") == filedName2Id.end())
                {
                    RuntimeError("ERROR: cannot find field fileName");
                }
                nameId = filedName2Id["fileName"];
                if (filedName2Id.find("word_conf") == filedName2Id.end())
                {
                    RuntimeError("ERROR: cannot find field word_conf");
                }
                confId = filedName2Id["word_conf"];
                lineIdx++;
                continue;
            }
            ConfSegments utt;
            string uttName;
            {
                if (utt.Parse(line, filedName2Id.size() - 1, nameId, confId, uttName))
                {
                    m_confScoresPerUtterances.insert(make_pair(uttName, utt));
                    if (lineIdx % 1000 == 0)
                    {
                        fprintf(stderr, "parsing tsv file %s, %d line parsed, %d failed to parse\r", fileName.c_str(), lineIdx+1, failed2Parse); 
                    }
                }
                else
                {
                    failed2Parse++;
                }
            }
            lineIdx++;
        }
        fprintf(stderr, "\n\n");
        return true;
    }
    size_t NumUtterances() const{
        return m_confScoresPerUtterances.size();
    }
    vector<string> GetUUIDs()
    {
        vector<string> res; 
        for (auto& x : m_confScoresPerUtterances)
        {
            res.push_back(x.first);
        }
        return res;
    }
    const ConfSegments& GetConfRecord(string name)
    {
        assert(m_confScoresPerUtterances.find(name) != m_confScoresPerUtterances.end());
        return m_confScoresPerUtterances[name];
    }

    int LastFrameIndex(string name)
    {
        assert(m_confScoresPerUtterances.find(name) != m_confScoresPerUtterances.end());
        return m_confScoresPerUtterances[name].LastFrameIndex();
    }

};

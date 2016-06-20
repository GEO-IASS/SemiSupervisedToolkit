#include <tclap/CmdLine.h>
#include <memory>
#include <map>
#include "File.h"


using namespace TCLAP;
using namespace std;

class ActionParamers
{
public:
    string rootDir; 
    float  silConf; 
    string featureSCP; 
    string tsvfile; 
    string doneFile;
    string prefixInSCP;
    bool   createDummyChunks;
    size_t chunkSize;


    ActionParamers()
    {
        m_argParsers = make_shared<CmdLine>("tsv2scp copyright@yongqiang wang", ' ', "0.1");
        //////////////////////////////////////////////////////////////////////////
        // name arguments
        //////////////////////////////////////////////////////////////////////////
        m_valueArgugments["r"] = make_shared<ValueArg<string>>("r", "root", "root directory", true, "/output/path", "string", *m_argParsers);
        m_valueArgugments["s"] = make_shared<ValueArg<string>>("s", "silConf", "sil confidence score", true, "1.0", "float", *m_argParsers);
        m_valueArgugments["U"] = make_shared<ValueArg<string>>("U", "done", "done file", false, "", "string", *m_argParsers);
        m_valueArgugments["p"] = make_shared<ValueArg<string>>("p", "prefix", "prefix in scp", true, "", "string", *m_argParsers);
        m_valueArgugments["z"] = make_shared<ValueArg<string>>("z", "chunk-size", "chunk size", false, "1000000", "int", *m_argParsers);
        m_switchArguments["c"] = make_shared<SwitchArg>("c", "", "create dummy conf chunk (for Yan's mixing purpose)",  *m_argParsers, false);
        m_postionalArguments.push_back(make_shared<UnlabeledValueArg<string>>("feature_scp", "feature scp", true, "", "file path"));
        m_postionalArguments.push_back(make_shared<UnlabeledValueArg<string>>("tsv_file", "tsv file", true, "", "file path"));
        for (auto& x : m_postionalArguments)
        {
            m_argParsers->add(*x);
        }
    }
    bool Init(int argc, char** argv)
    {
        try
        {
            PrintArguments(argc, argv);
            m_argParsers->parse(argc, argv);

            featureSCP = m_postionalArguments[0]->getValue(); 
            tsvfile = m_postionalArguments[1]->getValue();
            rootDir = m_valueArgugments["r"]->getValue();
            string silConfStr = m_valueArgugments["s"]->getValue();
            silConf = std::stof(silConfStr);
            doneFile = m_valueArgugments["U"]->getValue();
            createDummyChunks = m_switchArguments["c"]->getValue();
            prefixInSCP = m_valueArgugments["p"]->getValue();
            chunkSize = std::stoi(m_valueArgugments["z"]->getValue());
            return true;

        }
        catch (ArgException& e)
        {
            fprintf(stderr, "ERROR: %s for arg %s\n", e.error().c_str(), e.argId());
            return false;
        }
        catch (exception & e)
        {
            fprintf(stderr, "ERROR: %s\n", e.what());
            return false;
        }
    }
    void PrintArguments(int argc, char** argv)
    {
        for (size_t i = 0; i < argc; i++)
        {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
    }

    ~ActionParamers()
    {
        if (!doneFile.empty())
        {
            FILE* fp = fopenOrDie(doneFile.c_str(), "w");
            fprintf(fp, "done");
            fcloseOrDie(fp);
        }
    }
private: 
    shared_ptr<TCLAP::CmdLine>                          m_argParsers;
    map <string, shared_ptr<ValueArg<string>>>          m_valueArgugments;
    map <string, shared_ptr<SwitchArg>>                 m_switchArguments;
    vector<shared_ptr<UnlabeledValueArg<string>>>       m_postionalArguments;



};
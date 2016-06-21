#include <tclap/CmdLine.h>
#include <memory>
#include <map>
#include "File.h"


using namespace TCLAP;
using namespace std;

class ActionParamers
{
public:
    string tsvfile;
    int    numBins;



    ActionParamers()
    {
        m_argParsers = make_shared<CmdLine>("tsv2scp copyright@yongqiang wang", ' ', "0.1");
        //////////////////////////////////////////////////////////////////////////
        // name arguments
        //////////////////////////////////////////////////////////////////////////
        m_valueArgugments["b"] = make_shared<ValueArg<string>>("b", "bin", "number of bins", true, "21", "int", *m_argParsers);
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


            tsvfile = m_postionalArguments[0]->getValue();
            numBins = std::stoi(m_valueArgugments["b"]->getValue());
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
        
    }
private:
    shared_ptr<TCLAP::CmdLine>                          m_argParsers;
    map <string, shared_ptr<ValueArg<string>>>          m_valueArgugments;
    map <string, shared_ptr<SwitchArg>>                 m_switchArguments;
    vector<shared_ptr<UnlabeledValueArg<string>>>       m_postionalArguments;



};
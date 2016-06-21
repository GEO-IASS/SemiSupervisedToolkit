#include "argument.h"
#include "../tsv2scp/TSVFileParser.h"


int main(int argc, char** argv)
{
    try
    {
        ActionParamers arguments;
        bool succ = arguments.Init(argc, argv);
        if (!succ)
        {
            RuntimeError("ERROR: parsing argument failed\n");
        }


        //////////////////////////////////////////////////////////////////////////
        // 1. parsing tsv file 
        //////////////////////////////////////////////////////////////////////////
        TSVFileParser tsvParser;
        {
            succ = tsvParser.Parse(arguments.tsvfile);
            if (!succ)
                RuntimeError("ERROR: parsing tsv file (%s) failed\n", arguments.tsvfile.c_str());
            else
                fprintf(stderr, "Parsing %d utterances from file (%s)\n", (int)tsvParser.NumUtterances(), arguments.tsvfile.c_str());
        }

        
        //////////////////////////////////////////////////////////////////////////
        // 2. output histogram
        //////////////////////////////////////////////////////////////////////////
        float binWidth = 1.0f / (arguments.numBins - 1);
        size_t totalFrames = 0;
        map<int, float> hisogram = tsvParser.SpeechFrameConfHistogram(arguments.numBins, totalFrames);
        fprintf(stdout, "Establishing histogram from %lu speech frames:\n", totalFrames);
        for (auto x : hisogram)
        {
            int id = x.first;
            float num = x.second;
            fprintf(stdout, "(%.5f, %.5f] -> %f%%\n", binWidth*id, binWidth*(id+1), num*100 );
        }
        
    }
    catch (exception& e)
    {
        fprintf(stderr, "ERROR: exception is caught: %s\n", e.what());
    }


}
#include "htkfeatio.h"
#include "argument.h"
#include "TSVFileParser.h"
#include "SCPFileParser.h"

std::unordered_map<std::wstring, unsigned int> msra::asr::htkfeatreader::parsedpath::archivePathStringMap;
std::vector<std::wstring> msra::asr::htkfeatreader::parsedpath::archivePathStringVector;

class ScoopeGuard
{
public:
    explicit ScoopeGuard(std::function<void()> onExit)
        : _onExitCallback(onExit)
    {
    }
    ~ScoopeGuard()
    {
        _onExitCallback();
    }

private:
    std::function<void()> _onExitCallback;
};


void ProcessOneChunk(const vector<pair<string,ppath>>& utterances, 
                        TSVFileParser& tsvParser, 
                        string confChunkName, 
                        string labelChunkName, 
                        bool createLabelChunk, 
                        float silConf,
                        vector<ppath>& confRecFiles, 
                        vector<ppath>& confLabFiles
                    )
{
    msra::asr::htkfeatreader reader; 
    string kind; 
    size_t dim;
    unsigned int period;
    reader.getinfo(utterances[0].second, kind, dim, period);


    msra::asr::htkfeatwriter writer(msra::strfun::utf16(confChunkName), "USER", 2, period);
    shared_ptr<msra::asr::htkfeatwriter> pConfLabWriter; 
    if (createLabelChunk)
        pConfLabWriter = make_shared<msra::asr::htkfeatwriter>(msra::strfun::utf16(labelChunkName), "USER", 2, period);
    size_t numOutFrames = 0;
    size_t st = 0;
    size_t en = 0; 
    float avgSpeechConf = 0;
    size_t speechFrame = 0;

    for (auto& oneutt : utterances)
    {
        string name = oneutt.first; 
        ppath path = oneutt.second;
        ConfSegments segments = tsvParser.GetConfRecord(name);
        size_t numframes = path.numframes();
        vector<vector<float>> res(numframes);
        for (auto& x : res) x.resize(2);

        int preEnd = -1;
        for (size_t iseg = 0; iseg < segments.size(); iseg++)
        {
            ConfSegment& seg = segments[iseg];
            int stFrame = seg.GetStartFrame();
            int enFrame = seg.GetEndFrame();
            float confScore = seg.GetConfScore();
            bool isCorrect = seg.isCorrect();
            enFrame = enFrame < numframes ? enFrame : (int)numframes - 1;
            for (int f = preEnd + 1; f < stFrame; f++)
            {
                res[f][0] = silConf; 
                res[f][1] = 1.0f; 
            }
            for (int f = stFrame; f <= enFrame; f++)
            {
                avgSpeechConf += confScore;
                speechFrame++;
                res[f][0] = confScore; 
                res[f][1] = isCorrect ? (float)1.0 : (float)0.0;
            }
            preEnd = enFrame;
        }
        for (int f = preEnd + 1; f < numframes; f++)
        {            
            res[f][0] = silConf; res[f][1] = 1.0f;
        }

        for (auto fea : res)
        {
            writer.write(fea);
        }
        numOutFrames += res.size();
        en = st + numframes - 1;

        if (pConfLabWriter)
        {
            vector<float> res = { 1.0, 1.0 };
            for (size_t f = 0; f < numframes; f++)
            {
                pConfLabWriter->write(res);
            }
            ppath outpath;
            outpath.setAsArchivePath(wstring(path), msra::strfun::utf16(labelChunkName), st, en);
            confLabFiles.push_back(outpath);
        }
        ppath outpath; 
        outpath.setAsArchivePath(wstring(path), msra::strfun::utf16(confChunkName), st, en);
        confRecFiles.push_back(outpath);
        
        st += numframes;
    }
    writer.close(numOutFrames);
    if (pConfLabWriter)
        pConfLabWriter->close(numOutFrames);

    fprintf(stderr, "\t\tTotal number of frame=%d; speech frame=%d; Average speech frame conf=%f\n", numOutFrames, speechFrame, avgSpeechConf/speechFrame);
}
void ProcessConfChunks(SCPFileParser& scpParser,
                    TSVFileParser& tsvParser,
                    float silConf,
                    string rootDir,
                    string confChunkDir,
                    string labelChunkDir,
                    string scpFileBase,
                    size_t chunkSize,
                    bool createLabelChunk)
{
    vector<string> allUtterances = tsvParser.GetUUIDs();
    vector<pair<string,ppath>> oneChunk; 
    size_t frameInOneChunk=0;
    vector<ppath>   confRecoFiles;
    vector<ppath>   confLabelFiles;
    vector<ppath>   feaFiles;
    size_t currentChunkId = 0;
    size_t lengthMismatch = 0;
    float elapsed_seconds_so_far = 0;
    auto ProcessOneChunkWrapper = [&oneChunk, &currentChunkId, &tsvParser, &confRecoFiles, &confLabelFiles, &frameInOneChunk, &elapsed_seconds_so_far,
        confChunkDir, labelChunkDir, createLabelChunk, silConf]()
    {
        string confRecoChunkName = msra::strfun::strprintf("%s/ConfChunks.%05d.chunk", confChunkDir.c_str(), currentChunkId);
        string confLabelChunkName = msra::strfun::strprintf("%s/ConfChunks.%05d.chunk", labelChunkDir.c_str(), currentChunkId);
        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        ProcessOneChunk(oneChunk, tsvParser, confRecoChunkName, confLabelChunkName, createLabelChunk, silConf, confRecoFiles, confLabelFiles);
        std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - start;
        elapsed_seconds_so_far += elapsed_seconds.count();
        fprintf(stderr, "Processing %d-th chunk with %d utterances and %d frames (in %.2f seconds, %.2f seconds so far)\n",
            currentChunkId, oneChunk.size(), frameInOneChunk, elapsed_seconds.count(), elapsed_seconds_so_far);
        currentChunkId++;
        oneChunk.clear();
        frameInOneChunk = 0;
    };


    for (size_t i = 0; i < allUtterances.size(); i++)
    {
        string name = allUtterances[i];
        bool found;
        ppath path = scpParser.QueryUtterance(name, found);
        if (found)
        {
            if (path.numframes() < tsvParser.LastFrameIndex(name))
            {
                lengthMismatch++;
                continue;
            }
            frameInOneChunk += path.numframes();
            oneChunk.push_back(make_pair(name, path));
            feaFiles.push_back(path);
            if (frameInOneChunk >= chunkSize)
            {
                ProcessOneChunkWrapper();
            }
        }
    }
    /* process remaining one */
    ProcessOneChunkWrapper();

    fprintf(stderr, "%d files suffer length mismatch problem\n", lengthMismatch);

    string confRecSCP = scpFileBase + ".conf.rec.scp";
    string confLabSCP = scpFileBase + ".conf.lab.scp"; 
    string featSCP = scpFileBase + ".fea.scp";

    FILE* fp1 = fopenOrDie(confRecSCP, "w");
    {
        //ScoopeGuard g1([&]{fcloseOrDie(fp1);  });
        for (auto& x : confRecoFiles)
        {
            fprintf(fp1, "%ls=%ls\n", ((wstring)x).c_str(), x.physicallocation2().c_str());
        }
        fcloseOrDie(fp1);
    }
    if (createLabelChunk)
    {
        FILE* fp2 = fopenOrDie(confLabSCP, "w");
        {
            ScoopeGuard g2([&]{fcloseOrDie(fp2);  });
            for (auto& x : confLabelFiles)
            {
                fprintf(fp2, "%ls=%ls\n", ((wstring)x).c_str(), x.physicallocation2().c_str());
            }
        }
    }
    FILE* fp2 = fopenOrDie(featSCP, "w");
    {
        ScoopeGuard g2([&]{fcloseOrDie(fp2); });
        for (auto& x : feaFiles)
        {
            fprintf(fp2, "%ls=%ls\n", ((wstring)x).c_str(), x.physicallocation2().c_str());
        }
    }

}



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
        // 1. parsing scp and tsv file 
        //////////////////////////////////////////////////////////////////////////
        SCPFileParser scpParser;
        {
            succ = scpParser.Parse(arguments.featureSCP);
        }
        TSVFileParser tsvParser;
        {
            succ=tsvParser.Parse(arguments.tsvfile);
            if (!succ)
                RuntimeError("ERROR: parsing tsv file (%s) failed\n", arguments.tsvfile.c_str());
            else
                fprintf(stderr, "Parsing %d utterances from file (%s)\n", (int)tsvParser.NumUtterances(), arguments.tsvfile.c_str());
        }
        
        //////////////////////////////////////////////////////////////////////////
        // 2. create necessary intermediate dir
        //////////////////////////////////////////////////////////////////////////
        string rootdir = arguments.rootDir + "/" + arguments.prefixInSCP;
        string confChunkDir = rootdir + "/ConfChunk.rec";
        string labelChunkDir = rootdir + "/ConfChunk.lab";
        msra::files::make_intermediate_dirs(msra::strfun::utf16(confChunkDir)+L"/dummy");
        if (arguments.createDummyChunks)
        {
             msra::files::make_intermediate_dirs(msra::strfun::utf16(labelChunkDir)+L"/dummy");
        }
        string scpDir = arguments.rootDir + "/SCPs"; 
        msra::files::make_intermediate_dirs(msra::strfun::utf16(scpDir)+L"/dummy");
        
        ProcessConfChunks(scpParser, tsvParser, arguments.silConf,
                            rootdir, 
                            confChunkDir, 
                            labelChunkDir, 
                            scpDir+"/"+arguments.prefixInSCP, 
                            arguments.chunkSize, 
                            arguments.createDummyChunks
                            );

    }
    catch (exception& e)
    {
        fprintf(stderr, "ERROR: exception is caught: %s\n", e.what());
    }


}
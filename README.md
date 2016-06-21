# Semi-supervised toolkit 

##  1.  tsv2scp  
*description*: Given a Cosmos-generated tsv file (containing conf scores at
    segment levle) and a feature scp,  generarte: 
        1. conf score chunks 
        2. conf score scp
        3. re-organized feature scp

*usage*:  

    tsv2scp -s 1.0 -z %chunkSize% -c -r %rootdir% -p %prefix% %featureSCP% %tsv%
    
It will create ```%rootdir%/%prefix%/ConfChunks.(lab|rec)``` directory to store feature chunks;
```%rootdir%/SCPs/%prefix%.conf.(lab|rec).scp``` and ```%rootdir%/SCPs/%prefix%.fea.scp``` to store feature and conf scps. 


### 2. ConfHisogram 
*description*: Given a Cosmos-generated tsv files, generate speech frame
confidence score histogram. 

*usage* 
    ConfHistogram -b 21 %tsv%

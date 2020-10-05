/*
 * =====================================================================================
 *
 *       Filename:  controlLayer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/25/2017 09:38:26
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Baoxing Song (songbx.me), songbaoxing168@163.com
 *
 * =====================================================================================
 */

/*************************************************************************




 ************************************************************************/

#include "controlLayer.h"

std::string softwareName = "proali";


int getSequences( int argc, char** argv, std::map<std::string, std::string>& parameters ){
    std::stringstream usage;
    usage << "Usage: "<<softwareName<<" gff2seq -i inputGffFile -r inputGenome -o outputCdsSequences " << std::endl<<
          "Options" << std::endl <<
          " -h        produce help message" << std::endl <<
          " -i FILE   reference genome in GFF/GTF format" << std::endl <<
          " -r FILE   genome sequence in fasta format" << std::endl <<
          " -m INT    minimum intron size for a functional ORF (default:5)" << std::endl <<
          " -o FILE   output file of the longest CDS for each gene" << std::endl << std::endl;
    InputParser inputParser (argc, argv);
    if(inputParser.cmdOptionExists("-h") ||inputParser.cmdOptionExists("--help")  ){
        std::cerr << usage.str();
    }else if( inputParser.cmdOptionExists("-i") && inputParser.cmdOptionExists("-r") && inputParser.cmdOptionExists("-o") ){
        std::string inputGffFile = inputParser.getCmdOption("-i");
        std::string genome = inputParser.getCmdOption("-r");
        std::string outputCdsSequences = inputParser.getCmdOption("-o");

        int minIntron;
        if( inputParser.cmdOptionExists("-m") ){
            minIntron = std::stoi( inputParser.getCmdOption("-m") );
        }else{
            minIntron=5;
        }

        if( minIntron < 5 ){
            std::cerr << "the intron size should be 5 at minimum" << std::endl;
            exit(1);
        }
        getSequences( inputGffFile, genome, outputCdsSequences, parameters, minIntron);
        return 0;
    }else{
        std::cerr << usage.str();
    }
    return 0;
}




int DenoveAssemblyVariantCalling( int argc, char** argv, std::map<std::string, std::string>& parameters ) {
    std::stringstream usage;
    usage << "Usage: " << softwareName
          << " varcallv2 -i refGffFile -r refGenome  -t targetGff -s targetGenome -o output GFF/GTF file " << std::endl <<
          "Options" << std::endl <<
          " -h        produce help message" << std::endl <<
          " -i FILE   reference GFF/GTF file" << std::endl <<
          " -r FILE   reference genome sequence" << std::endl <<
          " -a FILE   sam file" << std::endl <<
          " -s FILE   target genome sequence" << std::endl <<
          " -o FILE   output anchors" << std::endl <<
          " -m FILE   output file in maf format" << std::endl <<
          " -f FILE   output sequence alignment for each block in maf format (any block with size larger than window width would be ignored)" << std::endl <<
          " -v FILE   output variant calling in vcf format" << std::endl <<
          " -w INT    sequence alignment window width (default: 10000)" << std::endl << std::endl;

    InputParser inputParser(argc, argv);
    if (inputParser.cmdOptionExists("-h") || inputParser.cmdOptionExists("--help")) {
        std::cerr << usage.str();
    } else if (inputParser.cmdOptionExists("-i") && inputParser.cmdOptionExists("-r") &&
               inputParser.cmdOptionExists("-a") && inputParser.cmdOptionExists("-s") &&
             (inputParser.cmdOptionExists("-m") || inputParser.cmdOptionExists("-f") || inputParser.cmdOptionExists("-v")  || inputParser.cmdOptionExists("-o")  ) ) {
        std::string refGffFilePath = inputParser.getCmdOption("-i");
        std::string referenceGenomeSequence = inputParser.getCmdOption("-r");
        std::string targetGenomeSequence = inputParser.getCmdOption("-s");

        std::string outPutMafFile="";
        if (inputParser.cmdOptionExists("-m") ){
            outPutMafFile = inputParser.getCmdOption("-m");
        }

        std::string outPutVcfFile = "";
        if (inputParser.cmdOptionExists("-v") ){
            outPutVcfFile = inputParser.getCmdOption("-v");
        }

        std::string outPutFragedFile;
        if ( inputParser.cmdOptionExists("-f") ){
            outPutFragedFile = inputParser.getCmdOption("-f");
        }

        std::string samFilePath=inputParser.getCmdOption("-a");

        size_t widownWidth = 10000;
        if( inputParser.cmdOptionExists("-w") ){
            widownWidth = std::stoi( inputParser.getCmdOption("-w") );
        }

        std::map<std::string, std::vector<AlignmentMatch>> alignmentMatchsMap;
        setupAnchorsWithSpliceAlignmentResult( refGffFilePath, samFilePath, alignmentMatchsMap);

        if( inputParser.cmdOptionExists("-o") ) {
            std::ofstream ofile;
            ofile.open(inputParser.getCmdOption("-o"));
            int blockIndex = 0;
            ofile << "refChr" << "\t"
                  << "referenceStart" << "\t"
                  << "referenceEnd" << "\t"
                  << "queryStart" << "\t"
                  << "queryChr" << "\t"
                  << "queryStart" << "\t"
                  << "queryEnd" << "\t"
                  << "blockIndex" << std::endl;

            for ( std::map<std::string, std::vector<AlignmentMatch>>::iterator it = alignmentMatchsMap.begin(); it!=alignmentMatchsMap.end(); ++it ) {
                ofile << "#block begin" << std::endl;
                blockIndex++;

                for (int rangeIndex = 0; rangeIndex <  it->second.size(); ++rangeIndex) {
                    ofile << it->second[rangeIndex].getDatabaseChr() << "\t"
                          << it->second[rangeIndex].getDatabaseStart() << "\t"
                          << it->second[rangeIndex].getDatabaseEnd() << "\t"
                          << it->second[rangeIndex].getQueryChr() << "\t"
                          << it->second[rangeIndex].getQueryStart() << "\t"
                          << it->second[rangeIndex].getQueryEnd() << "\t"
                          << it->second[rangeIndex].getQueryStrand() << "\t"
                          << it->second[rangeIndex].getDatabaseName() << "\t" <<
                          blockIndex << std::endl;
                    if (rangeIndex > 0) {
                        if (it->second[rangeIndex].getQueryStrand() == POSITIVE) {
                            ofile << it->second[rangeIndex].getDatabaseChr() << "\t"
                                   << it->second[rangeIndex - 1].getDatabaseEnd() + 1 << "\t"
                                   << it->second[rangeIndex].getDatabaseStart() - 1 << "\t"
                                   << it->second[rangeIndex].getQueryChr() << "\t"
                                   << it->second[rangeIndex - 1].getQueryEnd() + 1 << "\t"
                                   << it->second[rangeIndex].getQueryStart() - 1 << "\t"
                                    << "intergenetic" << "\t" <<
                                                                        blockIndex << std::endl;
                        } else {
                            ofile << it->second[rangeIndex].getDatabaseChr() << "\t"
                                   << it->second[rangeIndex - 1].getDatabaseEnd() + 1 << "\t"
                                   << it->second[rangeIndex].getDatabaseStart() - 1 << "\t"
                                   << it->second[rangeIndex].getQueryChr() << "\t"
                                   << it->second[rangeIndex].getQueryEnd() + 1 << "\t"
                                   << it->second[rangeIndex - 1].getQueryStart() - 1 << "\t"
                                    << "intergenetic" << "\t" <<
                                    blockIndex << std::endl;
                        }
                    }
                }
                ofile << "#block end" << std::endl;
            }
            ofile.close();
        }

        deNovoGenomeVariantCalling(alignmentMatchsMap, referenceGenomeSequence,  targetGenomeSequence, widownWidth, outPutMafFile, outPutVcfFile, outPutFragedFile);

        return 0;
    }else{
        std::cerr << usage.str();
    }
    return 0;
}




int genomeAlignment( int argc, char** argv, std::map<std::string, std::string>& parameters ) {
    std::stringstream usage;
    int windownWidth = 15000;
    double calculateIndelDistance = 3;
    double GAP_OPEN_PENALTY=-0.03;
    double INDEL_SCORE=-0.01;
    double MIN_ALIGNMENT_SCORE = 4;
    int MAX_DIST_BETWEEN_MATCHES=25;  // between maize and sorghum set it as 27
    int refMaximumTimes=1;
    int queryMaximumTimes=2;
    bool outPutAlignmentForEachInterval = false;

    usage << "Usage: " << softwareName
          << " genoAli -i refGffFile -r refGenome -a samFile -s targetGenome -o output mafFile " << std::endl <<
          "Options" << std::endl <<
          " -h        produce help message" << std::endl <<
          " -i FILE   reference GFF/GTF file" << std::endl <<
          " -r FILE   reference genome sequence" << std::endl <<
          " -a FILE   sam file" << std::endl <<
          " -s FILE   target genome sequence" << std::endl <<
          " -o STRING output file prefix" << std::endl <<
          " -w INT    sequence alignment window width (default: " << windownWidth << ")" << std::endl <<
          " -R INT    reference coverage(default: " << refMaximumTimes << ")" << std::endl<<
          " -Q INT    query coverage (default: " << queryMaximumTimes << ")"  << std::endl<<
          " advanced options" << std::endl<<
          " -d DOUBLE calculateIndelDistance (default: " << calculateIndelDistance << ")"  << std::endl<<
          " -O DOUBLE chain open gap penalty (default: " << GAP_OPEN_PENALTY << ")"  << std::endl<<
          " -E DOUBLE chain extend gap penalty (default: " << INDEL_SCORE << ")"  << std::endl<<
          " -I DOUBLE minimum chain score (default: " << MIN_ALIGNMENT_SCORE << ")"  << std::endl<<
          " -D INT    maximum gap size for chain (default: " << MAX_DIST_BETWEEN_MATCHES << ")"  << std::endl<<
          " -f        output alignment for each interval (default: " << outPutAlignmentForEachInterval << ")"  << std::endl<<
          std::endl;

    InputParser inputParser(argc, argv);
    if (inputParser.cmdOptionExists("-h") || inputParser.cmdOptionExists("--help")) {
        std::cerr << usage.str();
    } else if (inputParser.cmdOptionExists("-i") && inputParser.cmdOptionExists("-r") &&
               inputParser.cmdOptionExists("-a") && inputParser.cmdOptionExists("-s") &&
               inputParser.cmdOptionExists("-o")) {
        std::string refGffFilePath = inputParser.getCmdOption("-i");
        std::string referenceGenomeSequence = inputParser.getCmdOption("-r");
        std::string targetGenomeSequence = inputParser.getCmdOption("-s");
        std::string outPutFilePath = inputParser.getCmdOption("-o");
        std::string samFilePath=inputParser.getCmdOption("-a");

        if( inputParser.cmdOptionExists("-w")){
            windownWidth = std::stoi(inputParser.getCmdOption("-w"));
        }
        if( inputParser.cmdOptionExists("-d")){
            calculateIndelDistance = std::stod(inputParser.getCmdOption("-d"));
        }
        if( inputParser.cmdOptionExists("-O")){
            GAP_OPEN_PENALTY = std::stod(inputParser.getCmdOption("-O"));
        }
        if( inputParser.cmdOptionExists("-E")){
            INDEL_SCORE = std::stod(inputParser.getCmdOption("-E"));
        }
        if( inputParser.cmdOptionExists("-I")){
            MIN_ALIGNMENT_SCORE = std::stod(inputParser.getCmdOption("-I"));
        }
        if( inputParser.cmdOptionExists("-D")){
            MAX_DIST_BETWEEN_MATCHES = std::stoi(inputParser.getCmdOption("-D"));
        }
        if( inputParser.cmdOptionExists("-R")){
            refMaximumTimes = std::stoi(inputParser.getCmdOption("-R"));
        }
        if( inputParser.cmdOptionExists("-Q")){
            queryMaximumTimes = std::stoi(inputParser.getCmdOption("-Q"));
        }
        if( inputParser.cmdOptionExists("-f")){
            outPutAlignmentForEachInterval = true;
        }

        std::vector<std::vector<OrthologPair2>> alignmentMatchsMap;

        setupAnchorsWithSpliceAlignmentResultQuota( refGffFilePath, samFilePath, alignmentMatchsMap, INDEL_SCORE, GAP_OPEN_PENALTY, MIN_ALIGNMENT_SCORE,
                                                    MAX_DIST_BETWEEN_MATCHES, refMaximumTimes, queryMaximumTimes,
                                                    calculateIndelDistance);

        std::ofstream ofile;
        ofile.open(outPutFilePath + ".forPlotQuota");

        ofile << "refChr" << "\t"
              << "referenceStart" << "\t"
              << "referenceEnd" << "\t"
              << "queryStart" << "\t"
              << "queryChr" << "\t"
              << "queryStart" << "\t"
              << "queryEnd" << "\t"
              << "blockIndex" <<"\t geneId" << std::endl;

        size_t  totalAnchors = 0;
        int blockIndex = 0;
        for ( std::vector<OrthologPair2> alignmentMatchs : alignmentMatchsMap ){
            ofile << "#block begin" << std::endl;
            totalAnchors += alignmentMatchs.size();
            blockIndex++;
            for( int rangeIndex = 0; rangeIndex <alignmentMatchs.size();  ++rangeIndex){
                ofile << alignmentMatchs[rangeIndex].getRefChr() << "\t"
                      << alignmentMatchs[rangeIndex].getRefStartPos() << "\t"
                      << alignmentMatchs[rangeIndex].getRefEndPos() << "\t"
                      << alignmentMatchs[rangeIndex].getQueryChr() << "\t"
                      << alignmentMatchs[rangeIndex].getQueryStartPos() << "\t"
                      << alignmentMatchs[rangeIndex].getQueryEndPos() << "\t"
                      << alignmentMatchs[rangeIndex].getStrand() << "\t"
                      << alignmentMatchs[rangeIndex].getReferenceGeneName() << "\t" <<
                        blockIndex << std::endl;

                if (  rangeIndex >0 ){
                    if( alignmentMatchs[rangeIndex].getStrand() == POSITIVE ){
                        ofile << alignmentMatchs[rangeIndex].getRefChr() << "\t"
                               << alignmentMatchs[rangeIndex-1].getRefEndPos()+1 << "\t"
                               << alignmentMatchs[rangeIndex].getRefStartPos() - 1 << "\t"
                               << alignmentMatchs[rangeIndex].getQueryChr() << "\t"
                               << alignmentMatchs[rangeIndex-1].getQueryEndPos()+1 << "\t"
                               << alignmentMatchs[rangeIndex].getQueryStartPos()-1 << "\t"
                                << alignmentMatchs[rangeIndex].getStrand() << "\t" <<
                                blockIndex << std::endl;
                    }else{
                        ofile << alignmentMatchs[rangeIndex].getRefChr() << "\t"
                               << alignmentMatchs[rangeIndex-1].getRefEndPos()+1 << "\t"
                               << alignmentMatchs[rangeIndex].getRefStartPos() - 1 << "\t"
                               << alignmentMatchs[rangeIndex].getQueryChr() << "\t"
                               << alignmentMatchs[rangeIndex].getQueryEndPos() +1 << "\t"
                               << alignmentMatchs[rangeIndex-1].getQueryStartPos()-1 << "\t"
                                << alignmentMatchs[rangeIndex].getStrand() << "\t" <<
                                blockIndex << std::endl;

                    }
                }
            }
            ofile << "#block end" << std::endl;

        }
        ofile.close();
//        std::cout << "-D:" << inputParser.getCmdOption("-D") << std::endl;
//        std::cout << "-E:" << inputParser.getCmdOption("-E") << std::endl;
//        std::cout << "-O:" << inputParser.getCmdOption("-O") << std::endl;
//        std::cout << "-d:" << inputParser.getCmdOption("-d") << std::endl;
//        std::cout << "-totalAnchors:" << totalAnchors << std::endl;
        genomeAlignment(alignmentMatchsMap, referenceGenomeSequence,  targetGenomeSequence, windownWidth, outPutFilePath, outPutAlignmentForEachInterval);

        return 0;
    }else{
        std::cerr << usage.str();
    }
    return 0;
}
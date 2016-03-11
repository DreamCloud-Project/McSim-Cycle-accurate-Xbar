/*
 * dcSimuParams.hxx
 *
 *  Created on: Sep 10, 2015
 *      Author: manu
 */

#ifndef MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_
#define MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_

#include <string>

class dcSimuParams {
public:
	std::string getOutputFolder();
	std::string getMappingHeuristic();
	std::string getMappingFile();
	unsigned int getMappingSeed();
	std::string getSchedulingStrategy();
	std::string getAppXml();
	std::string getModeFile();
	dcSimuParams(int argc, char** argv);
	void printHelp();
	bool getHelp();
	bool getSeqDep();
	bool getFullDuplex();
	bool getGenerateWaveforms();
	bool dontHandlePeriodic();
	bool getRandomNonDet();
	unsigned int getRows();
	unsigned int getCols();
	unsigned int getIterations();
	unsigned long int getCoresFrequencyInHz() const;
	unsigned long int getSimuEnd() const;
	double getCoresPeriodInNano() const;
	std::string getXbarPolicy();
	unsigned int getXbarBuffSize();
	unsigned int getLocalReadCost();
	unsigned int getLocalWriteCost();
	unsigned int getRemoteWriteCost();
	unsigned int getRemoteReadCost();
	unsigned int getDimension();

private:
	std::string outputFolder;
	std::string mappingHeuristic;
	std::string mappingFile;
	std::string schedulingStrategy;
	std::string appXml;
	std::string modeFile;
	bool dontHandlePer;
	bool help;
	bool fullDuplex;
	bool randomNonDet;
	bool seqDep;
	unsigned int rows;
	unsigned int cols;
	unsigned int its;
	unsigned int mappingSeed;
	char* binary;
	unsigned long int coresFrequencyInHz;
	unsigned long int simuEnd;
	std::string xbarPolicy; //"FD", "PR", "RR" for full duplex, priority, and round robin respectively
	unsigned int xbarBuffSize;
	unsigned int localReadCost;
	unsigned int localWriteCost;
	unsigned int remoteWriteCost;
	unsigned int remoteReadCost;
};



#endif /* MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_ */

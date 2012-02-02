/*
	testsuite <tests>
	testsuite -t <test>
	testsuite -s <suitefile>


	testsuite <tests>
	 -> loop over <tests>, performing them

	testsuite -s <suitefile>
	 -> read suitefile
	 -> loop over tests in <suitefile>, performing them
	 
	testsuite -t <test>
	 -> perform <test>

	To perform a test:
	- loop over all the commands
	  - save intermediate results
	  - verify intermediate results
*/

#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <limits.h>

#ifdef WIN32
#include <io.h>
#endif

using namespace std;

#include "Shell.h"
#include "FileSystem.h"
#include "MessageFormatter.h"
#include "DFTreeBCGNodeBuilder.h"
#include "dftcalc.h"
#include "compiletime.h"

const int DFTCalc::VERBOSITY_SEARCHING = 2;

const int VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 1;
const int VERBOSITY_EXECUTIONS = 2;

void print_help(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dftcalc [INPUTFILE.dft] [options]");
	messageFormatter->message("  Calculates the failure probability for the specified DFT file, given the");
	messageFormatter->message("  specified time constraints. Result is written to the specified output file.");
	messageFormatter->message("  Check dftcalc --help=output for more details regarding the output.");
	messageFormatter->message("");
	messageFormatter->notify ("General Options:");
	messageFormatter->message("  -h, --help      Show this help.");
	messageFormatter->message("  --color         Use colored messages.");
	messageFormatter->message("  --no-color      Do not use colored messages.");
	messageFormatter->message("  --version       Print version info and quit.");
	messageFormatter->message("");
	messageFormatter->notify ("Debug Options:");
	messageFormatter->message("  --verbose=x     Set verbosity to x, -1 <= x <= 5.");
	messageFormatter->message("  -v, --verbose   Increase verbosity. Up to 5 levels.");
	messageFormatter->message("  -q              Decrease verbosity.");
	messageFormatter->message("");
	messageFormatter->notify ("Output Options:");
	messageFormatter->message("  -r FILE         Output result to this file. (see dftcalc --help=output)");
	messageFormatter->flush();
}

void print_help_output(MessageFormatter* messageFormatter) {
}

void print_version(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dftcalc");
	messageFormatter->message(string("  built on ") + COMPILETIME_DATE);
	{
		FileWriter out;
		out << string("  git revision `") + COMPILETIME_GITREV + "'";
		if(COMPILETIME_GITCHANGED)
			out << " + uncommited changes";
		messageFormatter->message(out.toString());
	}
	messageFormatter->message("  ** Copyright statement. **");
	messageFormatter->flush();
}

std::string DFTCalc::getCoralRoot(MessageFormatter* messageFormatter) {
	
	if(!coralRoot.empty()) {
		return coralRoot;
	}
	
	char* root = getenv((const char*)"CORAL");
	std::string coralRoot = root?string(root):"";
	
	if(coralRoot=="") {
		//if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[coralRoot.length()+1];
		for(int i=coralRoot.length();i--;) {
			if(coralRoot[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = coralRoot[i];
		}
		buf[coralRoot.length()] = '\0';
		if(buf[coralRoot.length()-1]=='/') {
			buf[coralRoot.length()-1] = '\0';
		}
		coralRoot = string(buf);
	}
end:
	return coralRoot;
}

std::string DFTCalc::getRoot(MessageFormatter* messageFormatter) {
	
	if(!dft2lntRoot.empty()) {
		return dft2lntRoot;
	}
	
	char* root = getenv((const char*)"DFT2LNTROOT");
	std::string dft2lntRoot = root?string(root):"";
	
	if(dft2lntRoot=="") {
		//if(messageFormatter) messageFormatter->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where lntnodes/ can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[dft2lntRoot.length()+1];
		for(int i=dft2lntRoot.length();i--;) {
			if(dft2lntRoot[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = dft2lntRoot[i];
		}
		buf[dft2lntRoot.length()] = '\0';
		if(buf[dft2lntRoot.length()-1]=='/') {
			buf[dft2lntRoot.length()-1] = '\0';
		}
		dft2lntRoot = string(buf);
	}
	
	struct stat rootStat;
	if(stat((dft2lntRoot).c_str(),&rootStat)) {
		// report error
		if(messageFormatter) messageFormatter->reportError("Could not stat DFT2LNTROOT (`" + dft2lntRoot + "')");
		dft2lntRoot = "";
		goto end;
	}
	
	if(stat((dft2lntRoot+DFT2LNT::LNTSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::LNTSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create LNT Nodes directory (`" + dft2lntRoot+DFT2LNT::LNTSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}

	if(stat((dft2lntRoot+DFT2LNT::BCGSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::BCGSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create BCG Nodes directory (`" + dft2lntRoot+DFT2LNT::BCGSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}
	
	if(messageFormatter) messageFormatter->reportAction("DFT2LNTROOT is: " + dft2lntRoot,VERBOSITY_DATA);
end:
	return dft2lntRoot;
}

std::string DFTCalc::getCADPRoot(MessageFormatter* messageFormatter) {
	
	if(!cadpRoot.empty()) {
		return cadpRoot;
	}
	
	char* root = getenv((const char*)"CADP");
	std::string cadp = root?string(root):"";
	
	if(cadp=="") {
		//if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
		goto end;
	}
	
	// \ to /
	{
		char buf[cadp.length()+1];
		for(int i=cadp.length();i--;) {
			if(cadp[i]=='\\')
				buf[i] = '/';
			else
				buf[i] = cadp[i];
		}
		buf[cadp.length()] = '\0';
		if(buf[cadp.length()-1]=='/') {
			buf[cadp.length()-1] = '\0';
		}
		cadp = string(buf);
	}
end:
	return cadp;
}


std::string intToString(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

int DFTCalc::printOutput(const File& file) {
	std::string* outContents = FileSystem::load(file);
	if(outContents) {
		messageFormatter->notifyHighlighted("** OUTPUT of " + file.getFileName() + " **");
		messageFormatter->message(*outContents);
		messageFormatter->notifyHighlighted("** END output of " + file.getFileName() + " **");
		delete outContents;
	}
}

int DFTCalc::calculateDFT(const std::string& cwd, const File& dftOriginal, FileWriter& out) {
	File dft    = dftOriginal.newWithPathTo(cwd);
	File svl    = dft.newWithExtension("svl");
	File exp    = dft.newWithExtension("exp");
	File bcg    = dft.newWithExtension("bcg");
	File imc    = dft.newWithExtension("imc");
	File ctmdpi = dft.newWithExtension("ctmdpi");
	File lab    = ctmdpi.newWithExtension("lab");
	File dot    = dft.newWithExtension("dot");
	File png    = dot.newWithExtension("png");
	File input  = dft.newWithExtension("input");

	FileSystem::mkdir(File(cwd));

	if(FileSystem::exists(dft))    FileSystem::remove(dft);
	if(FileSystem::exists(svl))    FileSystem::remove(svl);
	if(FileSystem::exists(exp))    FileSystem::remove(exp);
	if(FileSystem::exists(bcg))    FileSystem::remove(bcg);
	if(FileSystem::exists(imc))    FileSystem::remove(imc);
	if(FileSystem::exists(ctmdpi)) FileSystem::remove(ctmdpi);
	if(FileSystem::exists(lab))    FileSystem::remove(lab);
	if(FileSystem::exists(dot))    FileSystem::remove(dot);
	if(FileSystem::exists(png))    FileSystem::remove(png);
	if(FileSystem::exists(input))  FileSystem::remove(input);

	//printf("Copying %s to %s\n",dftOriginal.getFileRealPath().c_str(),dft.getFileRealPath().c_str());
	FileSystem::copy(dftOriginal,dft);

	int com = 0;
	int result = 0;
	Shell::SystemOptions sysOps;
	sysOps.verbosity = VERBOSITY_EXECUTIONS;
	sysOps.cwd = cwd;
	
	messageFormatter->notify("Calculating `"+dft.getFileBase()+"'");
	
	// dft -> exp, svl
	messageFormatter->reportAction("Translating DFT to EXP...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dft2lntc.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".dft2lntc.out";
	std::stringstream ss;
	ss << dft2lntcExec.getFilePath()
	   << " --verbosity=" << messageFormatter->getVerbosity()
	   << " -s \"" + svl.getFileRealPath() + "\""
	   << " -x \"" + exp.getFileRealPath() + "\""
	   << " -b \"" + bcg.getFileRealPath() + "\""
	   << " \""    + dft.getFileRealPath() + "\""
	    ;
	sysOps.command = ss.str();
	result = Shell::system(sysOps);

	if(!FileSystem::exists(exp) || !FileSystem::exists(svl)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	// svl, exp -> bcg
	messageFormatter->reportAction("Building IMC...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".svl.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".svl.out";
	sysOps.command    = svlExec.getFilePath()
	                  + " \""    + svl.getFileRealPath() + "\"";
	result = Shell::system(sysOps);
	
	if(!FileSystem::exists(bcg)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	// bcg -> ctmdpi, lab
	messageFormatter->reportAction("Translating IMC to CTMDPI...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imc2ctmdpi.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".imc2ctmdpi.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".imc2ctmdpi.out";
	sysOps.command    = imc2ctmdpExec.getFilePath()
	                  + " -a FAIL"
	                  + " -o \"" + ctmdpi.getFileRealPath() + "\""
	                  + " \""    + bcg.getFileRealPath() + "\""
					  ;
	result = Shell::system(sysOps);

	if(!FileSystem::exists(ctmdpi)) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	// -> mrmcinput
	MRMC::FileHandler* fileHandler = new MRMC::FileHandler();
	fileHandler->generateInputFile(input);
	if(!FileSystem::exists(input)) {
		messageFormatter->reportError("Error generating MRMC input file `" + input.getFileRealPath() + "'");
		return 1;
	}

	// ctmdpi, lab, mrmcinput -> calculation
	messageFormatter->reportAction("Calculating probability...",VERBOSITY_FLOW);
	sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".mrmc.report";
	sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".mrmc.err";
	sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".mrmc.out";
	sysOps.command    = mrmcExec.getFilePath()
	                  + " ctmdpi"
	                  + " \""    + ctmdpi.getFileRealPath() + "\""
	                  + " \""    + lab.getFileRealPath() + "\""
	                  + " < \""  + input.getFileRealPath() + "\""
	                  ;
	result = Shell::system(sysOps);

	if(result) {
		printOutput(File(sysOps.outFile));
		printOutput(File(sysOps.errFile));
		return 1;
	}

	if(fileHandler->readOutputFile(File(sysOps.outFile))) {
		messageFormatter->reportError("Could not calculate");
		return 1;
	} else {
		std::stringstream out;
		float res = fileHandler->getResult();
		out << "Result: " << res << std::endl;
		messageFormatter->reportAction(out.str());
	}


	if(!buildDot.empty()) {
		// bcg -> dot
		messageFormatter->reportAction("Building DOT from IMC...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg_io.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".bcg_io.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".bcg_io.out";
		sysOps.command    = bcgioExec.getFilePath()
						  + " \""    + bcg.getFileRealPath() + "\""
						  + " \""    + dot.getFileRealPath() + "\""
						  ;
		result = Shell::system(sysOps);
		
		if(!FileSystem::exists(dot)) {
			printOutput(File(sysOps.outFile));
			printOutput(File(sysOps.errFile));
			return 1;
		}
		
		// dot -> png
		messageFormatter->reportAction("Translating DOT to " + buildDot + "...",VERBOSITY_FLOW);
		sysOps.reportFile = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dot.report";
		sysOps.errFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com  ) + ".dot.err";
		sysOps.outFile    = cwd + "/" + dft.getFileBase() + "." + intToString(com++) + ".dot.out";
		sysOps.command    = dotExec.getFilePath()
						  + " -T" + buildDot
						  + " \""    + dot.getFileRealPath() + "\""
						  + " -o \"" + png.getFileRealPath() + "\""
						  ;
		result = Shell::system(sysOps);

		if(!FileSystem::exists(png)) {
			printOutput(File(sysOps.outFile));
			printOutput(File(sysOps.errFile));
			return 1;
		}

	}

	return result;
}

int main(int argc, char** argv) {

	/* Command line arguments and their default settings */
	string times           = "";
	int    timesSet        = 0;
	string timeFileName    = "";
	int    timeFileSet     = 0;
	string resultFileName  = "";
	int    resultFileSet   = 0;
	string dotToType       = "png";
	int    dotToTypeSet    = 0;

	int verbosity            = 0;
	int print                = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	int printVersion         = 0;

	/* Parse command line arguments */
	char c;
	while( (c = getopt(argc,argv,"h:pqr:t:v-:")) >= 0 ) {
		switch(c) {
			
			// -r FILE
			case 'r':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					resultFileName = "";
					resultFileSet = 1;
				} else {
					resultFileName = string(optarg);
					resultFileSet = 1;
				}
				break;
			
			// -p
			case 'p':
				print = 1;
				break;
			
			// -t FILE
			case 't':
				if(strlen(optarg)==1 && optarg[0]=='-') {
					timeFileName = "";
					timeFileSet = 1;
				} else {
					timeFileName = string(optarg);
					timeFileSet = 1;
				}
				break;
			
			// -h
			case 'h':
				printHelp = true;
			
			// -v
			case 'v':
				++verbosity;
				break;

			// -q
			case 'q':
				--verbosity;
				break;

			// --
			case '-':
				if(!strcmp("help",optarg)) {
					printHelp = true;
				} else if(!strcmp("version",optarg)) {
					printVersion = true;
				} else if(!strcmp("color",optarg)) {
					useColoredMessages = true;
				} else if(!strcmp("times",optarg)) {
					timesSet = true;
					if(strlen(optarg)>6 && optarg[5]=='=') {
						times = string(optarg+6);
					} else {
						printf("%s: --times needs argument\n\n",argv[0]);
						printHelp = true;
					}
				} else if(!strcmp("dot",optarg)) {
					dotToTypeSet = true;
					if(strlen(optarg)>4 && optarg[3]=='=') {
						dotToType = string(optarg+4);
					}
				} else if(!strcmp("verbose",optarg)) {
					if(strlen(optarg)>8 && optarg[7]=='=') {
						verbosity = atoi(optarg+8);
					} else {
						++verbosity;
					}
				} else if(!strcmp("no-color",optarg)) {
					useColoredMessages = false;
				}
		}
	}

//	printf("args:\n");
//	for(unsigned int i=0; i<(unsigned int)argc; i++) {
//		printf("  %s\n",argv[i]);
//	}

	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);
	messageFormatter->useColoredMessages(useColoredMessages);
	messageFormatter->setVerbosity(verbosity);
	messageFormatter->setAutoFlush(true);

	/* Print help / version if requested and quit */
	if(printHelp) {
		print_help(messageFormatter);
		exit(0);
	}
	if(printVersion) {
		print_version(messageFormatter);
		exit(0);
	}

	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	vector<File> dfts;
	if(optind<argc) {
		int isSet = 0;
		for(unsigned int i=optind; i<(unsigned int)argc; ++i) {
			if(argv[i][0]=='-') {
				if(strlen(argv[i])==1) {
				}
			} else {
				dfts.push_back(File(FileSystem::getRealPath("."),string(argv[i])));
			}
		}
	}
	
	string cwd = FileSystem::getRealPath(".") + "/output";
	
	FileSystem::mkdir(File(cwd));
	
	Shell::messageFormatter = messageFormatter;
	
	DFTCalc calc;
	calc.setMessageFormatter(messageFormatter);
	if(dotToTypeSet) calc.setBuildDOT(dotToType);
	
	//std::cerr << "Yeah: " << dotToTypeSet << ", " << dotToType << std::endl;
	
	if(calc.checkNeededTools()) {
		return -1;
	}
	
	FileWriter out;
	// Empty result file
	
	// Calculate DFTs
	PushD workdir(cwd);
	bool hasInput = false;
	for(File dft: dfts) {
		hasInput = true;
		if(FileSystem::exists(dft)) {
			calc.calculateDFT(cwd,dft,out);
		} else {
			messageFormatter->reportError("DFT File `" + dft.getFileRealPath() + "' does not exist");
		}
	}
	workdir.popd();

	if(!hasInput) {
		messageFormatter->reportWarning("No calculations performed");
	}

	// Write result file
	if(resultFileSet && resultFileName!="") {
		std::fstream resultFile(resultFileName);
		if(resultFile.is_open()) {
			resultFile << out.toString();
		}
	}
	// Print result file
	if(print || (resultFileSet && resultFileName=="")) {
		std::cout << out.toString();
	}
	return 0;
}
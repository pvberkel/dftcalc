/*
 * mrmc.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#include "mrmc.h"

#include "FileSystem.h"
#include "FileWriter.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int MRMCParser::generateInputFile(const File& file) {
	FileWriter out;
	out << out.applyprefix << "set print on" << out.applypostfix;
	out << out.applyprefix << m_calcCommand << out.applypostfix;
	out << out.applyprefix << "$RESULT[1]" << out.applypostfix;
	out << out.applyprefix << "quit" << out.applypostfix;
	std::ofstream resultFile(file.getFileRealPath());
	if(resultFile.is_open()) {
		resultFile << out.toString();
		resultFile.flush();
	} else {
		return 1;
	}
	if(!FileSystem::exists(file)) {
		return 1;
	}
	return 0;
}

int MRMCParser::readOutputFile(const File& file) {
	if(!FileSystem::exists(file)) {
		return 1;
	}

	FILE* fp;
	long len;
	char* buffer;

	fp = fopen(file.getFileRealPath().c_str(),"rb");
	fseek(fp,0,SEEK_END);
	len = ftell(fp)+1;
	fseek(fp,0,SEEK_SET);
	buffer = (char *)malloc(len);
	fread(buffer,len,1,fp); //read into buffer
	fclose(fp);

	const char* resultString = NULL;
	const char* c = buffer;
	while ((c - buffer) < len && *c) {
		//printf("%c",*c);
		if(!strncmp("$MIN_RESULT",c,11) || !strncmp("$MAX_RESULT",c,11)) {
			c += 15;
			resultString = c;
			//printf("\nfound: %s\n",c);
			//printf("should be: '%s'\n",resultString.c_str());
			break;
		} else if (!strncmp("$RESULT:", c, 8)) {
			c += 11;
			resultString = c;
			break;
		} else if (!strncmp("$RESULT[1]", c, 10)) {
			c += 13;
			resultString = c;
			break;
		}
		c++;
	}

	if(!resultString) {
		return 1;
	}

	{
		results.clear();
		const char *c = resultString;
		const char *end = c;
		while (*end && *end != '\n')
			end++;
		std::string res(c, end - c);
		if (res.find('[') == std::string::npos) {
			end = c;
			while (*end && *end != ')' && *end != ',')
				end++;
			res = std::string(c, end - c);
			decnumber<> val(res);
			results.push_back(std::pair<decnumber<>, decnumber<>>(val, val));
			m_isCalculated = true;
		} else {
			size_t pos = res.find('[');
			std::string pre = res.substr(0, pos);
			res = res.substr(pos + 1);
			pos = res.find(']');
			std::string post = res.substr(pos + 1);
			res = res.substr(0, pos);
			pos = post.find(',');
			if (pos != std::string::npos)
				post = post.substr(0, pos);
			pos = res.find(';');
			if (pos == std::string::npos)
				pos = res.find(',');
			std::string low = pre + res.substr(0, pos) + post;
			std::string up = pre + res.substr(pos + 2) + post;
			results.push_back(std::pair<decnumber<>, decnumber<>>(low, up));
			m_isCalculated = true;
		}
	}


	free(buffer);
	return 0;
}

std::pair<decnumber<>, decnumber<>> MRMCParser::getResult() {
	if(results.size()<1) {
		return std::pair<decnumber<>, decnumber<>>(-1, -1);
	}
	return results[0];
}
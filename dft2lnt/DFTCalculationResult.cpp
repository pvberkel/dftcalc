#include "DFTCalculationResult.h"

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResult& result) {
	if(const YAML::Node itemNode = node["dft"]) {
		result.dftFile = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["failprobs"]) {
		std::vector<DFT::DFTCalculationResultItem> failprobs;
		itemNode >> failprobs;
		result.failprobs = failprobs;
	}
	if(const YAML::Node itemNode = node["stats"]) {
		Shell::RunStatistics stats;
		itemNode >> stats;
		result.stats = stats;
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResult& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "dft"  << YAML::Value << result.dftFile;
	out << YAML::Key << "failprobs"  << YAML::Value << result.failprobs;
	// for dfttest, when we have one item, show it under the key that dfttest expects
	// dfttest currently only has support for the "failprob" key, not for "failprobs"
	for(auto it: result.failprobs) {
		out << YAML::Key << "failprob"  << YAML::Value << it.failprob;
		break;
	}
	out << YAML::Key << "stats"  << YAML::Value << result.stats;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, map<std::string,DFT::DFTCalculationResult>& resultMap) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
		std::string dft;
		DFT::DFTCalculationResult result;
		dft = it->first.as<std::string>();
		it->second >> result;
		resultMap.insert(std::pair<std::string,DFT::DFTCalculationResult>(dft,result));
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const map<std::string,DFT::DFTCalculationResult>& resultMap) {
	out << YAML::BeginMap;
	for(auto it: resultMap) {
		out << YAML::Key   << it.first;
		out << YAML::Value << it.second;
	}
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, DFT::DFTCalculationResultItem& result) {
	if(const YAML::Node itemNode = node["missionTime"]) {
		result.missionTime = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["mrmcCommand"]) {
		result.mrmcCommand = itemNode.as<std::string>();
	}
	if(const YAML::Node itemNode = node["failprob"]) {
		result.failprob = itemNode.as<double>();
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const DFT::DFTCalculationResultItem& result) {
	out << YAML::BeginMap;
	out << YAML::Key << "missionTime"  << YAML::Value << result.missionTime;
	out << YAML::Key << "mrmcCommand"  << YAML::Value << result.mrmcCommand;
	out << YAML::Key << "failprob"  << YAML::Value << result.failprob;
	out << YAML::EndMap;
	return out;
}

const YAML::Node& operator>>(const YAML::Node& node, vector<DFT::DFTCalculationResultItem>& resultVector) {
	for(YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
		DFT::DFTCalculationResultItem result;
		*it >> result;
		resultVector.push_back(result);
	}
	return node;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const vector<DFT::DFTCalculationResultItem>& resultVector) {
	out << YAML::BeginSeq;
	for(auto it: resultVector) {
		out << it;
	}
	out << YAML::EndSeq;
	return out;
}

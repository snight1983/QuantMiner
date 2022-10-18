#include "Config.h"
#include <stdio.h>
#include <iostream>
#include <json/json.h>
#include <fstream>
#include <filesystem>
#include <glog/logging.h>
namespace fs = std::filesystem;

CConfig* CConfig::m_pObj = nullptr;

CConfig::CConfig() {
	fs::path lsPath = fs::current_path();
	lsPath.append("config.json");
	if (!fs::exists(lsPath)) { 
		LOG(ERROR) << "Not Found" << lsPath;
		exit(0); 
	}
	Json::Reader reader;
	Json::Value root;
	std::ifstream in(lsPath, std::ios::binary);
	if (!in.is_open()) {
		LOG(ERROR) << "Open Err" << lsPath; 
		exit(0); 
	}
	if (!reader.parse(in, root)){
		LOG(ERROR) << "Parse Err" << lsPath;
		exit(0); 
	}
	if (root.isMember("fullbfq") && root["fullbfq"].isString()) {
		m_sBfqDBPath = root["fullbfq"].asString();
	}
	if (root.isMember("fullqfq") && root["fullqfq"].isString()) {
		m_sQfqDBPath = root["fullqfq"].asString();
	}
	if (root.isMember("fullhfq") && root["fullhfq"].isString()) {
		m_sHfqDBPath = root["fullhfq"].asString();
	}
	LOG(INFO) << "BFQ DB " << m_sBfqDBPath;
	LOG(INFO) << "QFQ DB " << m_sQfqDBPath;
	LOG(INFO) << "HFQ DB " << m_sHfqDBPath;
}

CConfig::~CConfig() {

}
CConfig* CConfig::GetConfig() {
	if (nullptr == m_pObj) {
		m_pObj = new CConfig();
	}
	return m_pObj;
}

std::string CConfig::GetBFQDbPath() {
	CConfig* obj = CConfig::GetConfig();
	if (nullptr != obj){
		return obj->m_sBfqDBPath;
	}
	return "";
}

std::string CConfig::GetQFQDbPath() {
	CConfig* obj = CConfig::GetConfig();
	if (nullptr != obj) {
		return obj->m_sQfqDBPath;
	}
	return "";
}

std::string CConfig::GetHFQDbPath() {
	CConfig* obj = CConfig::GetConfig();
	if (nullptr != obj) {
		return obj->m_sHfqDBPath;
	}
	return "";
}

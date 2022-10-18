// QuantMiner.cpp: 定义应用程序的入口点。
//
#include "QuantMiner.h"
#include "Config.h"
#include <filesystem>
#include <glog/logging.h>
#include "db/CDBBase.h"

#include <sqlite3.h>

namespace fs = std::filesystem;

using namespace std;

int main()
{
	if (!std::filesystem::exists(fs::current_path().append("logs"))) {
		std::filesystem::create_directory(fs::current_path().append("logs"));
	}
	google::InitGoogleLogging("quantminer");
	FLAGS_alsologtostderr = true; 
	FLAGS_colorlogtostderr = true;    
	google::SetLogFilenameExtension(".log");
	
	std::u8string info_Path{ fs::current_path().append("logs").append("INFO_").u8string() };
	std::u8string warn_Path{ fs::current_path().append("logs").append("WARNING_").u8string() };
	std::u8string error_Path{ fs::current_path().append("logs").append("ERROR_").u8string() };
	
	google::SetLogDestination(google::GLOG_INFO, (const char*)info_Path.c_str());
	google::SetLogDestination(google::GLOG_WARNING, (const char*)warn_Path.c_str());
	google::SetLogDestination(google::GLOG_ERROR, (const char*)error_Path.c_str());

	CDBBase loBase;
	loBase.SetDBPath(E_BFQ, CConfig  ::GetBFQDbPath());
	loBase.SetDBPath(E_QFQ, CConfig::GetQFQDbPath());
	loBase.SetDBPath(E_HFQ, CConfig::GetHFQDbPath());

	// 计算活跃度
	// loBase.Statistics(E_BFQ);
	// loBase.Statistics(E_BFQ);
	// loBase.Statistics(E_QFQ);
	loBase.Statistics(E_HFQ);


	google::ShutdownGoogleLogging();

	return 0;
}

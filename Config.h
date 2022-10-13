#ifndef H_CONFIG
#define H_CONFIG
#include <string>

class Config {
public:
	~Config();
	static Config* GetConfig();
public:
	static std::string GetBfqDbPath();
	static std::string GetQFQDbPath();
	static std::string GetHFQDbPath();
private:
	Config();
private:
	static Config* m_pObj;
	std::string m_sBfqDBPath;
	std::string m_sQfqDBPath;
	std::string m_sHfqDBPath;
};

#endif // !H_CONFIG

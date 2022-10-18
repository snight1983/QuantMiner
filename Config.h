#ifndef H_CONFIG
#define H_CONFIG
#include <string>

class CConfig {
public:
	~CConfig();
	static CConfig* GetConfig();
public:
	static std::string GetBFQDbPath();
	static std::string GetQFQDbPath();
	static std::string GetHFQDbPath();
private:
	CConfig();
private:
	static CConfig* m_pObj;
	std::string m_sBfqDBPath;
	std::string m_sQfqDBPath;
	std::string m_sHfqDBPath;
};

#endif // !H_CONFIG

#ifndef H_DBBASE
#define H_DBBASE

#include <vector>
#include <queue>
#include <sqlite3.h>
#include <filesystem>

enum e_tstype { E_QFQ = 1, E_HFQ = 2, E_BFQ = 3 };

struct S_StockTS {
	int64_t m_nTS; 
	int64_t m_nDay;
	double  m_dOpen;  
	double  m_dClose; 
	double  m_dMax;   
	double  m_dMix; 
	double  m_dVolume;
	double  m_dAmount;
	
	void*   m_pCalc;
	int		m_nCalcLen;
	
	bool    m_bUp;
	bool	m_bDown;
	double  m_bDif;
	
	double  m_dPerc;    // 上震幅
	//double  m_dDownPerc;  // 下震幅

	//10, 20, 30, 40, 50, 60, 70, 80, 90, 100
	double  m_dUpActivity[10];    // 上涨活跃度
	double  m_dDownActivity[10];  // 下跌活跃度
	double  m_dDiffActivity[10];  // 涨跌活跃差

};

class CDBBase {
public:
	CDBBase();
	~CDBBase();
public:
	bool SetDBPath(
		e_tstype aeType,
		std::string aoPath);

	bool  Statistics(
		e_tstype aeType );
private:
	void calcActivity( std::ofstream& aoOutFile,
		const char* code,
		std::vector<S_StockTS*>& aoPerStockVct );

	void calcPerStock(
		std::vector<S_StockTS*>& aoPerStockVct);

	bool getTables(
		e_tstype aeType,
		std::vector<std::string>& aoStocks);

	sqlite3* getDB(e_tstype aeType);

	inline S_StockTS* getStockCache() {
		std::lock_guard<std::mutex> lock(m_oStockCacheLock);
		if (m_oStockCache.empty() == true) {
			return new S_StockTS();
		}
		S_StockTS* tmp = m_oStockCache.front();
		m_oStockCache.pop();
		return tmp;
	}

	inline void backStockCache(S_StockTS* ptr) {
		if (nullptr != ptr) {
			std::lock_guard<std::mutex> lock(m_oStockCacheLock);
			if (m_oStockCache.size() < 200000) {
				m_oStockCache.push(ptr);
			}
			else {
				delete ptr;
				ptr = nullptr;
			}
		}
	}
private:
	std::filesystem::path	m_oDBQFQPath;
	std::filesystem::path	m_oDBHFQPath;
	std::filesystem::path	m_oDBBFQPath;
	std::queue<S_StockTS*>	m_oStockCache;
	std::mutex				m_oStockCacheLock;
};

#endif // !H_DBBASE

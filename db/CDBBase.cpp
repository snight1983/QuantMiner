#include "CDBBase.h"
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <glog/logging.h>

namespace fs = std::filesystem;

CDBBase::CDBBase() {
}

CDBBase::~CDBBase() {
}

bool CDBBase::SetDBPath( e_tstype aeType, std::string aoPath ) {
	switch (aeType)
	{
	case E_QFQ:
		m_oDBQFQPath = aoPath;
		if (fs::exists(m_oDBQFQPath)) {
			return true;
		}
		break;
	case E_HFQ:
		m_oDBHFQPath = aoPath;
		if (fs::exists(m_oDBHFQPath)) {
			return true;
		}
		break;
	case E_BFQ:
		m_oDBBFQPath = aoPath;
		if (fs::exists(m_oDBBFQPath)) {
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

sqlite3* CDBBase::getDB(e_tstype aeType) {
	sqlite3* lpDB = nullptr;
	switch (aeType)
	{
	case E_QFQ: {
		std::u8string path_string{ m_oDBQFQPath.u8string() };
		sqlite3_open_v2((const char*)path_string.c_str(), &lpDB,
			SQLITE_OPEN_READWRITE |
			SQLITE_OPEN_NOMUTEX |
			SQLITE_OPEN_SHAREDCACHE, NULL);		
		break;
	}
	case E_HFQ:	{
		std::u8string path_string{ m_oDBHFQPath.u8string() };
		sqlite3_open_v2((const char*)path_string.c_str(), &lpDB,
			SQLITE_OPEN_READWRITE |
			SQLITE_OPEN_NOMUTEX |
			SQLITE_OPEN_SHAREDCACHE, NULL);
		break;
	}
	case E_BFQ: {
		std::u8string path_string{ m_oDBBFQPath.u8string() };
		sqlite3_open_v2((const char*)path_string.c_str(), &lpDB,
			SQLITE_OPEN_READWRITE |
			SQLITE_OPEN_NOMUTEX |
			SQLITE_OPEN_SHAREDCACHE, NULL);
		break;
	}
	default:
		break;
	}
	return  lpDB;
}

bool CDBBase::getTables(e_tstype aeType, std::vector<std::string>& aoStocksVct) {
	std::lock_guard<std::mutex> lock(m_oStockCacheLock);
	sqlite3* lpDB = getDB(aeType);
	if (nullptr != lpDB) {
		sqlite3_stmt* lpStmt = nullptr;
		const char* sql = "SELECT name FROM sqlite_master WHERE type='table';";
		if (sqlite3_prepare_v2(lpDB, sql, -1, &lpStmt, NULL) != SQLITE_OK) {
			LOG(ERROR) << "sqlite3_prepare_v2 error : " << (char*)sqlite3_errmsg(lpDB);
			return false;
		}
		while (sqlite3_step(lpStmt) == SQLITE_ROW) {
			if (sqlite3_column_count(lpStmt) == 1) {
				aoStocksVct.push_back((const char*)sqlite3_column_text(lpStmt, 0));
			}
		}
		sqlite3_finalize(lpStmt);
		sqlite3_close(lpDB);
	}
	return true;
}


struct S_QuantParam {
	double m_dUpPercSum;
	double m_dDownPercSum;
	double m_dUpTransSum;
	double m_dDownTransSum;
	double m_dMaxUpPerc;
	double m_dMaxUpDay;
	S_QuantParam();
};

S_QuantParam::S_QuantParam() {
	m_dUpPercSum = 0.0f;
	m_dDownPercSum = 0.0f;
	m_dUpTransSum = 0.0f;
	m_dDownTransSum = 0.0f;
	m_dMaxUpPerc = 0.0f;
	m_dMaxUpDay = 0.0f;
}

void CDBBase::calcPerStock(std::vector<S_StockTS*>& aoPerStockVct) {
	int lnCnt = aoPerStockVct.size();
	for (int i = 240; i < lnCnt; i++) {
		S_StockTS* lpDestTS = aoPerStockVct[i];
		S_QuantParam loParam;
		for (int j = i - 240; j < i; j++) {
			S_StockTS* lpCurTS = aoPerStockVct[j];
			if (lpCurTS->m_bUp) {
				loParam.m_dUpPercSum += lpCurTS->m_dPerc;
				loParam.m_dUpTransSum += lpCurTS->m_dAmount;
			}
			else{
				loParam.m_dDownPercSum += lpCurTS->m_dPerc;
				loParam.m_dDownTransSum += lpCurTS->m_dAmount;
			}
		}
		if (lpDestTS->m_dClose > 0) {
			loParam.m_dUpTransSum /= lpDestTS->m_dClose;
			loParam.m_dDownTransSum /= lpDestTS->m_dClose;
		}
		
		if (i + 240 < lnCnt) {
			for ( int k = i; k < i + 240; ++k ) {
				S_StockTS* lpAfterTS = aoPerStockVct[k];
				if (lpAfterTS->m_nDay > lpDestTS->m_nDay) {
					if (lpAfterTS->m_dMax > lpDestTS->m_dMix) {
						double ldTmpUpPerc = (lpAfterTS->m_dMax - lpDestTS->m_dMix) / lpDestTS->m_dMix;
						if (ldTmpUpPerc > loParam.m_dMaxUpPerc) {
							loParam.m_dMaxUpPerc = ldTmpUpPerc;
							loParam.m_dMaxUpDay = (k-i);
						}
					}
				}
			}
			loParam.m_dMaxUpPerc *= 100;
			loParam.m_dMaxUpDay /= 48;
			if (loParam.m_dMaxUpPerc > 2) {
				int a;
				a = 10;
			}
		}
	}
}
// 整体活跃度
void CDBBase::calcActivity ( 
	std::ofstream& aoOutFile, 
	const char* apCode,
	std::vector<S_StockTS*>& aoPerStockVct ) {
	int lnSize = aoPerStockVct.size();
	
	int liUpDays = 0;
	int liDownDays = 0;
	int lnTotalDays = 0;
	double ldUp[10] = { 0 };   
	double ldDown[10] = { 0 }; 
	double lnTotalUpPer = 0.0f;
	double lnTotalDownPer = 0.0f;
	double ldLastClose = 0.0f;
	double up2Cnt = 0.0f;
	
	int64_t lnDay = 0;
	S_StockTS* lpLastPerTS = nullptr;

	if (lnSize > 48 * 22 ) {
		for ( int i = 48 * 22; i < lnSize; ++i ) {
			S_StockTS* lpCurrentTS = aoPerStockVct[i];
			if (lpCurrentTS->m_nDay != lnDay) {
				lnTotalDays++;
				lnDay = lpCurrentTS->m_nDay;
				if (lpLastPerTS == nullptr) {
					lpLastPerTS = lpCurrentTS;
					continue;
				}
					if ((lpCurrentTS->m_dClose < lpLastPerTS->m_dClose) && 
						(lpCurrentTS->m_dClose > 0)){
						liDownDays += 1;
						// 下跌
						double ldPer = (lpLastPerTS->m_dClose - lpCurrentTS->m_dClose) / lpCurrentTS->m_dClose;
						if (ldPer > 0.0 && ldPer <= 0.01) {
							ldDown[0] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.01 && ldPer <= 0.02) {
							ldDown[1] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.02 && ldPer <= 0.03) {
							ldDown[2] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.03 && ldPer <= 0.04) {
							ldDown[3] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.04 && ldPer <= 0.05) {
							ldDown[4] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.05 && ldPer <= 0.06) {
							ldDown[5] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.06 && ldPer <= 0.07) {
							ldDown[6] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.07 && ldPer <= 0.08) {
							ldDown[7] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.08 && ldPer <= 0.09) {
							ldDown[8] += 1;
							lnTotalDownPer += ldPer;
						}
						else if (ldPer > 0.09) {
							ldDown[9] += 1;
							lnTotalDownPer += ldPer;
						}
					}
					else if (( lpCurrentTS->m_dClose > lpLastPerTS->m_dClose ) && 
						(lpLastPerTS->m_dClose > 0)){
						liUpDays += 1;
						// 上涨
						double ldPer = (lpCurrentTS->m_dClose - lpLastPerTS->m_dClose) / lpLastPerTS->m_dClose;
						if (ldPer > 0.0 && ldPer <= 0.01) {
							ldUp[0] += 1;
							lnTotalUpPer += ldPer;
						}
						else if (ldPer > 0.01 && ldPer <= 0.02) {
							ldUp[1] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.02 && ldPer <= 0.03) {
							ldUp[2] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.03 && ldPer <= 0.04) {
							ldUp[3] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.04 && ldPer <= 0.05) {
							ldUp[4] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.05 && ldPer <= 0.06) {
							ldUp[5] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.06 && ldPer <= 0.07) {
							ldUp[6] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.07 && ldPer <= 0.08) {
							ldUp[7] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.08 && ldPer <= 0.09) {
							ldUp[8] += 1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
						else if (ldPer > 0.09) {
							ldUp[9]+=1;
							lnTotalUpPer += ldPer;
							up2Cnt += 1;
						}
					}
				lpLastPerTS = lpCurrentTS;
			}
		}

			aoOutFile << apCode << ',';
			aoOutFile << std::to_string(lnTotalDays) << ',';
			aoOutFile << std::to_string(liUpDays) << ',';
			aoOutFile << std::to_string(double(liUpDays) / double( lnTotalDays)) << ',';
			aoOutFile << std::to_string(liDownDays) << ',';
			aoOutFile << std::to_string(double(liDownDays) / double(lnTotalDays)) << ',';

			aoOutFile << std::to_string(ldUp[0]) << ',' << std::to_string(ldUp[0] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[1]) << ',' << std::to_string(ldUp[1] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[2]) << ',' << std::to_string(ldUp[2] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[3]) << ',' << std::to_string(ldUp[3] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[4]) << ',' << std::to_string(ldUp[4] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[5]) << ',' << std::to_string(ldUp[5] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[6]) << ',' << std::to_string(ldUp[6] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[7]) << ',' << std::to_string(ldUp[7] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[8]) << ',' << std::to_string(ldUp[8] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldUp[9]) << ',' << std::to_string(ldUp[9] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[0]) << ',' << std::to_string(ldDown[0] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[1]) << ',' << std::to_string(ldDown[1] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[2]) << ',' << std::to_string(ldDown[2] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[3]) << ',' << std::to_string(ldDown[3] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[4]) << ',' << std::to_string(ldDown[4] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[5]) << ',' << std::to_string(ldDown[5] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[6]) << ',' << std::to_string(ldDown[6] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[7]) << ',' << std::to_string(ldDown[7] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[8]) << ',' << std::to_string(ldDown[8] / lnTotalDays) << ',';
			aoOutFile << std::to_string(ldDown[9]) << ',' << std::to_string(ldDown[9] / lnTotalDays) << ',';
			aoOutFile << std::to_string(lnTotalUpPer) << ',' << std::to_string(lnTotalDownPer) << ',' << std::to_string(lnTotalUpPer - lnTotalDownPer) <<  ',';
			aoOutFile << std::to_string(up2Cnt) << ',' << std::to_string(up2Cnt/ double(lnTotalDays)) << std::endl;
	}
	//outFile.close();

}

bool CDBBase::Statistics( e_tstype aeType ) {
	std::vector<std::string> aoStocks;
	getTables(aeType, aoStocks);
	sqlite3* lpDB = getDB(aeType);
	int lnCnt = 0;
	const char* sql = "SELECT TS,Date,Open,Close,Max,Min,Volume,Amount FROM %s ORDER BY TS ASC;";
	if (nullptr != lpDB) {
		for (std::string& tbName : aoStocks) {
			char lszSql[1000] = { 0 };
			sprintf_s(lszSql, sql, tbName.c_str());
			sqlite3_stmt* lpStmt = nullptr;
			if (sqlite3_prepare_v2(lpDB, lszSql, -1, &lpStmt, NULL) != SQLITE_OK) {
				LOG(ERROR) << "sqlite3_prepare_v2 error : " << (char*)sqlite3_errmsg(lpDB);
				continue;
			}
			std::vector<S_StockTS*> loItemVct;
			while (sqlite3_step(lpStmt) == SQLITE_ROW) {
				int colCnt = sqlite3_column_count(lpStmt);
				if ( colCnt >= 8 ) {
					S_StockTS* loItem = getStockCache();
					loItem->m_nTS  = sqlite3_column_int64(lpStmt, 0);
					loItem->m_nDay = sqlite3_column_int64(lpStmt, 1);
					loItem->m_dOpen = sqlite3_column_double(lpStmt, 2);
					loItem->m_dClose = sqlite3_column_double(lpStmt, 3);
					loItem->m_dMax = sqlite3_column_double(lpStmt, 4);
					loItem->m_dMix = sqlite3_column_double(lpStmt, 5);
					loItem->m_dVolume = sqlite3_column_double(lpStmt, 6);
					loItem->m_dAmount = sqlite3_column_double(lpStmt, 7);
					if (loItem->m_dAmount < 0) {
						loItem->m_dAmount = 0;
					}
					if ( (loItem->m_dOpen > loItem->m_dClose) && loItem->m_dClose > 0) {
						loItem->m_bDif = loItem->m_dOpen - loItem->m_dClose;
						loItem->m_bUp = true;
						loItem->m_bDown = false;
						loItem->m_dPerc = (loItem->m_bDif / loItem->m_dClose);
					}
					else if (( loItem->m_dClose > loItem->m_dOpen ) && (loItem->m_dOpen > 0)) {
						loItem->m_bDif = loItem->m_dClose - loItem->m_dOpen;
						loItem->m_bDown = true;
						loItem->m_bUp = false;
						loItem->m_dPerc = (loItem->m_bDif / loItem->m_dOpen);
					}
					else {
						loItem->m_bDif = 0;
						loItem->m_bUp = false;
						loItem->m_bDown = false;
						loItem->m_dPerc = 0;
					}
					loItemVct.push_back(loItem);
				}
			}
			/*
			std::ofstream outFile;

			outFile.open("activity.csv", std::ios::out | std::ios::in | std::ios::app);
			
			outFile << "代码" << ',' << "总天数" << ','
				<< "上涨天数" << ',' << "上涨天数占比" << ','
				<< "下跌天数" << ',' << "下跌天数占比" << ','
				<< "up0" << ',' << "up0Pec" << ','
				<< "up1" << ',' << "up1Pec" << ','
				<< "up2" << ',' << "up2Pec" << ','
				<< "up3" << ',' << "up3Pec" << ','
				<< "up4" << ',' << "up4Pec" << ','
				<< "up5" << ',' << "up5Pec" << ','
				<< "up6" << ',' << "up6Pec" << ','
				<< "up7" << ',' << "up7Pec" << ','
				<< "up8" << ',' << "up8Pec" << ','
				<< "up9" << ',' << "up9Pec" << ','
				<< "down0" << ',' << "down0Pec" << ','
				<< "down1" << ',' << "down1Pec" << ','
				<< "down2" << ',' << "down2Pec" << ','
				<< "down3" << ',' << "down3Pec" << ','
				<< "down4" << ',' << "down4Pec" << ','
				<< "down5" << ',' << "down5Pec" << ','
				<< "down6" << ',' << "down6Pec" << ','
				<< "down7" << ',' << "down7Pec" << ','
				<< "down8" << ',' << "down8Pec" << ','
				<< "down9" << ',' << "down9Pec" << ','
				<< "总上涨百分比" << ',' << "总下跌百分比" << ',' 
				<< "涨跌百分比差值" << ',' << "上涨 > 2%次数" <<',' <<"上涨 > 2%占比" << std::endl;
			
			calcActivity(outFile, tbName.c_str(),loItemVct);
			outFile.close();
			*/
			calcPerStock(loItemVct);
			for (S_StockTS* lpPerStocks : loItemVct) {
				backStockCache(lpPerStocks);
			}
			LOG(INFO) << "cnt : " << lnCnt++;
		}
		sqlite3_close(lpDB);
	}
	return false;
}


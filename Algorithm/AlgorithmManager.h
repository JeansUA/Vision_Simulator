#pragma once
#include "stdafx.h"
#include "Algorithm/AlgorithmBase.h"
#include <vector>

class CAlgorithmManager {
public:
    static CAlgorithmManager& GetInstance();
    void RegisterAlgorithms();
    int GetAlgorithmCount() const;
    CString GetAlgorithmName(int index) const;
    CAlgorithmBase* CreateAlgorithm(int index) const;
    CAlgorithmBase* CreateAlgorithm(const CString& name) const;
    ~CAlgorithmManager();

private:
    CAlgorithmManager();
    CAlgorithmManager(const CAlgorithmManager&) = delete;
    CAlgorithmManager& operator=(const CAlgorithmManager&) = delete;
    std::vector<CAlgorithmBase*> m_prototypes;
};

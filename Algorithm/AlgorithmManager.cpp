#include "stdafx.h"
#include "Algorithm/AlgorithmManager.h"
#include "Algorithm/Grayscale.h"
#include "Algorithm/Binarize.h"
#include "Algorithm/GaussianBlur.h"
#include "Algorithm/EdgeDetect.h"
#include "Algorithm/Morphology.h"
#include "Algorithm/BrightnessContrast.h"
#include "Algorithm/Sharpening.h"
#include "Algorithm/HoughCircle.h"
#include "Algorithm/HoughLine.h"
#include "Algorithm/Invert.h"

CAlgorithmManager::CAlgorithmManager()
{
}

CAlgorithmManager::~CAlgorithmManager()
{
    for (size_t i = 0; i < m_prototypes.size(); i++)
    {
        delete m_prototypes[i];
    }
    m_prototypes.clear();
}

CAlgorithmManager& CAlgorithmManager::GetInstance()
{
    static CAlgorithmManager instance;
    return instance;
}

void CAlgorithmManager::RegisterAlgorithms()
{
    // Clear any existing prototypes
    for (size_t i = 0; i < m_prototypes.size(); i++)
    {
        delete m_prototypes[i];
    }
    m_prototypes.clear();

    // Register all available algorithms
    m_prototypes.push_back(new CGrayscale());
    m_prototypes.push_back(new CBinarize());
    m_prototypes.push_back(new CGaussianBlur());
    m_prototypes.push_back(new CEdgeDetect());
    m_prototypes.push_back(new CMorphology());
    m_prototypes.push_back(new CBrightnessContrast());
    m_prototypes.push_back(new CSharpening());
    m_prototypes.push_back(new CHoughCircle());
    m_prototypes.push_back(new CHoughLine());
    m_prototypes.push_back(new CInvert());
}

int CAlgorithmManager::GetAlgorithmCount() const
{
    return (int)m_prototypes.size();
}

CString CAlgorithmManager::GetAlgorithmName(int index) const
{
    if (index < 0 || index >= (int)m_prototypes.size())
        return _T("");

    return m_prototypes[index]->GetName();
}

CAlgorithmBase* CAlgorithmManager::CreateAlgorithm(int index) const
{
    if (index < 0 || index >= (int)m_prototypes.size())
        return nullptr;

    return m_prototypes[index]->Clone();
}

CAlgorithmBase* CAlgorithmManager::CreateAlgorithm(const CString& name) const
{
    for (size_t i = 0; i < m_prototypes.size(); i++)
    {
        if (m_prototypes[i]->GetName() == name)
        {
            return m_prototypes[i]->Clone();
        }
    }
    return nullptr;
}

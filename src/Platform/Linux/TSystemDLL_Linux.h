#pragma once
#include <Toshi/Defines.h>
#include <Toshi/TString8.h>
TOSHI_NAMESPACE_START
class TSystemDLLLinux {
public:
    TSystemDLLLinux() = default;
    TSystemDLLLinux(const TSystemDLLLinux& other) : m_sFileName(other.m_sFileName) {}
    virtual ~TSystemDLLLinux();
    TBOOL Load(const TString8& name);
    void Unload();
    void* GetAddress(const TString8& symbol);
    const TString8& GetFileName() const { return m_sFileName; }
    void* GetDLL() const { return m_pDLL; }
    TSystemDLLLinux& operator=(const TSystemDLLLinux& other);
private:
    TString8 m_sFileName;
    void* m_pDLL = nullptr;
};
TOSHI_NAMESPACE_END

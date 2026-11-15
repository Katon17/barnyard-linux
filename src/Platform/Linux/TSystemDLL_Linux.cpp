#include <ToshiPCH.h>
#include "TSystemDLL_Linux.h"
#include <dlfcn.h>
TOSHI_NAMESPACE_START
TSystemDLLLinux::~TSystemDLLLinux() { Unload(); }
TBOOL TSystemDLLLinux::Load(const TString8& name) {
    Unload();
    m_sFileName = name;
    m_pDLL = dlopen(name.GetString(), RTLD_NOW | RTLD_LOCAL);
    return m_pDLL != nullptr;
}
void TSystemDLLLinux::Unload() {
    if (m_pDLL) dlclose(m_pDLL);
    m_pDLL = nullptr;
}
void* TSystemDLLLinux::GetAddress(const TString8& symbol) {
    return m_pDLL ? dlsym(m_pDLL, symbol.GetString()) : nullptr;
}
TSystemDLLLinux& TSystemDLLLinux::operator=(const TSystemDLLLinux& other) {
    if (this != &other) { Unload(); m_sFileName = other.m_sFileName; }
    return *this;
}
TOSHI_NAMESPACE_END

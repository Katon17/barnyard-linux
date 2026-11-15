// User-provided private samples are runtime inputs, NEVER bundled test fixtures.
#include <ToshiPCH.h>
#include <File/TTRB.h>
#include <T2Locale/T2Locale.h>
#include <GUI/T2GUIFont.h>
#include <Plugins/PPropertyParser/PBProperties.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
using namespace Toshi;
namespace fs=std::filesystem;
__attribute__((constructor(101))) static void init(){if(!TMemory::Initialise(128*1024*1024,0))_exit(90);}
static uint32_t u32(const std::vector<unsigned char>& data,size_t at){assert(at+4<=data.size());uint32_t v;memcpy(&v,data.data()+at,4);return v;}
static uint16_t u16(const std::vector<unsigned char>& data,size_t at){assert(at+2<=data.size());uint16_t v;memcpy(&v,data.data()+at,2);return v;}
int main(int argc,char** argv){
    assert(argc==2);static_assert(sizeof(void*)==4 && sizeof(TWCHAR)==2);
    fs::path game=fs::canonical(argv[1]);char temp[]="/tmp/barnyard-retail-trb-XXXXXX";char* user=mkdtemp(temp);assert(user);
    setenv("BARNYARD_GAME_DIR",game.c_str(),1);setenv("BARNYARD_USER_DIR",user,1);assert(TFileManager::Create());
    const char* paths[]={"Data/GUI/fonts.trb","Data/GUI/texsec.trb","Data/Matlibs/gui_Fonts.ttl","Data/lib_startup.trb","Data/Assets/lib_startup.trb","Data/Locale/eng-uk.trb"};
    for(const char* path:paths){
        std::ifstream in(game/path,std::ios::binary);std::vector<unsigned char> raw((std::istreambuf_iterator<char>(in)),{});
        assert(raw.size()>20 && !memcmp(raw.data(),"TSFL",4) && !memcmp(raw.data()+8,"TRBF",4));
        size_t reloc=0,relocBytes=0;uint32_t sections=0;bool compressed=false;
        for(size_t at=12;at+8<=raw.size();){
            const size_t size=u32(raw,at+4);assert(size<=raw.size()-at-8);
            if(!memcmp(raw.data()+at,"HDRX",4)) sections=u32(raw,at+12);
            if(!memcmp(raw.data()+at,"RELC",4)){reloc=at+8;relocBytes=size;}
            if(!memcmp(raw.data()+at,"SECC",4))compressed=true;
            at+=8+((size+3)&~size_t(3));
        }
        assert(sections>0 && sections<4096);
        TTRB trb;assert(trb.Load(path)==TTRB::ERROR_OK);
        auto contains=[&](const void* ptr,size_t bytes){
            const uintptr_t address=reinterpret_cast<uintptr_t>(ptr);
            for(uint32_t i=0;i<sections;++i){auto* info=trb.GetSectionInfo(i);uintptr_t base=reinterpret_cast<uintptr_t>(info->m_Data);if(address>=base && address-base<=info->m_Size && bytes<=info->m_Size-(address-base))return true;}
            return false;
        };
        for(int i=0;i<trb.GetNumSymbols();++i){
            auto* sym=trb.GetSymbol(i);assert(sym->HDRX<sections);
            auto* info=trb.GetSectionInfo(sym->HDRX);assert(sym->DataOffset<=info->m_Size);
            assert(trb.GetSymbolAddress(trb.GetSymbolName(i))==trb.GetSymbolAddress(*sym));
        }
        uint32_t count=0;
        if(reloc){
            count=u32(raw,reloc);assert(4ull+8ull*count<=relocBytes);
            for(uint32_t i=0;i<count;++i){
                size_t at=reloc+4+8*i;auto src=u16(raw,at),dst=u16(raw,at+2);auto offset=u32(raw,at+4);
                assert(src<sections && dst<sections);auto* source=trb.GetSectionInfo(src);auto* target=trb.GetSectionInfo(dst);
                assert(offset<=source->m_Size && source->m_Size-offset>=4);
                uintptr_t address;memcpy(&address,static_cast<unsigned char*>(source->m_Data)+offset,4);
                auto base=reinterpret_cast<uintptr_t>(target->m_Data);assert(address>=base && address-base<=target->m_Size);
            }
        }
        if(!strcmp(path,"Data/GUI/fonts.trb")){
            for(int i=0;i<trb.GetNumSymbols();++i){
                auto* def=trb.CastSymbol<T2GUIFontDef>(trb.GetSymbolName(i));assert(contains(def,sizeof(*def)));
                assert(def->uiNumMaterials>0 && def->uiNumMaterials<16 && def->uiLineHeight>0);
                assert(def->ui16MaxCharacter>=def->ui16MinCharacter);
                assert(contains(def->szTextureNames,1) && contains(def->pCharactersMap,4) && contains(def->pCharactersData,sizeof(T2GUIFontDefCharacterData)));
            }
        }
        if(!strcmp(path,"Data/Locale/eng-uk.trb")){
            auto* table=trb.CastSymbol<T2Locale::LocaleStrings>("LocaleStrings");assert(table && contains(table,sizeof(*table)));
            assert(table->m_numstrings>0 && table->m_numstrings<100000 && contains(table->Strings,table->m_numstrings*4));
            size_t units=0;
            for(int i=0;i<table->m_numstrings;++i){auto* p=table->Strings[i];assert(contains(p,2));size_t n=0;while(*p){assert(n++<65536 && contains(++p,2));}units+=n;}
            printf("LOCALE strings=%d validated_utf16_units=%zu (contents not exported)\n",table->m_numstrings,units);
        }
        if(!strcmp(path,"Data/lib_startup.trb")){
            auto* properties=PBProperties::LoadFromTRB(trb);assert(properties && contains(properties,sizeof(*properties)));
            // This startup descriptor lists materials/GUI, not a model library.
            assert(properties->GetPropertyCount()>0 && properties->GetProperty("matlib"));
        }
        printf("PASS %s sections=%u symbols=%d relocations=%u compressed=%d\n",path,sections,trb.GetNumSymbols(),count,compressed);fflush(stdout);
        trb.Close();assert(trb.Load(path)==TTRB::ERROR_OK);trb.Close();
    }
    TFileManager::Destroy();fs::remove_all(user);
    puts("PASS: six real retail TRB/TTL files loaded/reloaded, all symbol lookups and relocation ranges, font records and locale UTF16 pointers; NOT game/menu/rendering");
}

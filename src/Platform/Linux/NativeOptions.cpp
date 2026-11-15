#include "ToshiPCH.h"
#include "NativeOptions.h"
#include "File/TFile.h"
#include <charconv>
#include <cmath>
#include <memory>

namespace BarnyardLinux {
static std::string_view trim(std::string_view text) {
    auto first=text.find_first_not_of(" \t\r");
    if(first==text.npos) return {};
    return text.substr(first,text.find_last_not_of(" \t\r")-first+1);
}
template<class T> static bool number(std::string_view text,T& value) {
    auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    return !text.empty() && result.ec==std::errc{} && result.ptr==text.data()+text.size();
}
bool NativeOptions::parse(std::string_view text,std::string& error) {
    if(text.size()>8192 || text.find('\0')!=text.npos) { error="invalid config size or NUL byte"; return false; }
    NativeOptions candidate=*this;
    int line=0;
    while(!text.empty()) {
        ++line;
        auto end=text.find('\n');
        auto part=trim(text.substr(0,end));
        text=end==text.npos ? std::string_view{} : text.substr(end+1);
        if(part.empty() || part.front()=='#') continue;
        auto equal=part.find('=');
        if(equal==part.npos) { error="expected key=value on line "+std::to_string(line); return false; }
        auto key=trim(part.substr(0,equal)), value=trim(part.substr(equal+1));
        int integer=0; float real=0; bool valid=false;
        if(key=="width") { valid=number(value,integer)&&integer>=640&&integer<=3840; if(valid)candidate.width=integer; }
        else if(key=="height") { valid=number(value,integer)&&integer>=480&&integer<=2160; if(valid)candidate.height=integer; }
        else if(key=="gamma" || key=="sensitivity") {
            valid=number(value,real)&&std::isfinite(real)&&real>=0&&real<=1;
            if(valid) (key=="gamma" ? candidate.gamma : candidate.sensitivity)=real;
        } else if(key=="windowed" || key=="borderless" || key=="high_detail_grass" || key=="controller") {
            valid=number(value,integer)&&(integer==0||integer==1);
            if(valid) {
                if(key=="windowed")candidate.windowed=integer;
                if(key=="borderless")candidate.borderless=integer;
                if(key=="high_detail_grass")candidate.highDetailGrass=integer;
                if(key=="controller")candidate.controller=integer;
            }
        }
        if(!valid) { error="unknown key or invalid value on line "+std::to_string(line); return false; }
    }
    *this=candidate; error.clear(); return true;
}
NativeOptions::LoadResult NativeOptions::load(std::string& error) {
    auto* file=Toshi::TFile::Create("Settings/Linux.cfg");
    if(!file) { error.clear(); return LoadResult::Missing; }
    auto size=file->GetSize();
    if(size>8192) { file->Destroy(); error="config exceeds 8192 bytes"; return LoadResult::Invalid; }
    std::string text(size,'\0');
    auto read=file->Read(text.data(),size);
    file->Destroy();
    if(read!=size) { error="short config read"; return LoadResult::Invalid; }
    return parse(text,error) ? LoadResult::Loaded : LoadResult::Invalid;
}
}

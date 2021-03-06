#pragma once

#include <Tempest/Sound>
#include <unordered_map>
#include <atomic>

#include "segment.h"
#include "soundfont.h"
#include "style.h"

namespace Dx8 {

class DirectMusic;
class DlsCollection;
class SoundFont;
class Wave;

class Music final {
  public:
    Music()=default;
    Music(Music&&)=default;

    Music& operator = (Music&&)=default;
    void setVolume(float v);

    void dbgDump(const size_t patternId) const;

  private:
    Music(const Segment& s,DirectMusic& owner);

    struct Instrument final {
      const DlsCollection*  dls=nullptr;
      float                 volume=0.f;
      float                 pan=0.f;
      uint32_t              dwPatch=0;
      };

    struct InsInternal final {
      uint32_t              key;
      Dx8::SoundFont        font;
      float                 volume  =0.f;
      float                 pan     =0.f;

      mutable float         volumeLast=1.f;
      };

    struct Note final {
      uint64_t              at      =0;
      uint64_t              duration=0;
      uint8_t               note    =0;
      uint8_t               velosity=127;
      InsInternal*          inst=nullptr;
      };

    struct Curve final {
      uint64_t              at      =0;
      uint64_t              duration=0;
      Shape                 shape   =DMUS_CURVES_LINEAR;
      float                 startV   =0.f;
      float                 endV     =0.f;
      Control               ctrl    =BankSelect;
      InsInternal*          inst=nullptr;
      };

    struct PatternInternal final {
      std::vector<InsInternal> instruments;

      DMUS_IO_STYLE      styh;
      uint64_t           timeTotal=0;
      std::vector<Note>  waves;
      std::vector<Curve> volume;
      std::string        dbgName;
      };

    struct Internal final {
      std::vector<PatternInternal> pptn;
      uint64_t                     timeTotal=0;
      std::atomic<float>           volume{1.f};
      };

    void index();
    void index(const Style &stl, PatternInternal& inst, const Pattern &pattern);
    void index(Music::PatternInternal &idx, InsInternal *inst, const Style &stl, const Style::Part &part);

    void dbgDump(const Style &stl, const Pattern::PartRef &pref, const Style::Part &part) const;

    DirectMusic*                  owner=nullptr;

    uint32_t                      cordHeader=0;
    std::vector<DMUS_IO_SUBCHORD> subchord;
    const Style*                  style=nullptr;

    std::unordered_map<uint32_t,Instrument> instruments;
    std::shared_ptr<Internal>     intern;


    friend class DirectMusic;
    friend class Mixer;
  };

}

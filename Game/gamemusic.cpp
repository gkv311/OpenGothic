﻿#include "gamemusic.h"

#include <Tempest/Sound>
#include <Tempest/Log>

#include "dmusic/mixer.h"
#include "resources.h"

using namespace Tempest;

struct GameMusic::MusicProducer : Tempest::SoundProducer {
  MusicProducer():SoundProducer(44100,2){
    }

  void renderSound(int16_t* out,size_t n) override {
    const Daedalus::GEngineClasses::C_MusicTheme* theme=nullptr;
    {
      std::lock_guard<std::mutex> guard(pendingSync);
      theme = pendingMusic;
      pendingMusic = nullptr;
    }

    if(theme!=nullptr) {
      try {
        //Dx8::Music m = Resources::loadDxMusic("OWD_DayStd.sgt");
        Dx8::Music m = Resources::loadDxMusic(theme->file.c_str());
        m.setVolume(theme->vol);
        mix.setMusic(m);
        }
      catch(std::runtime_error&) {
        Log::e("unable to load sound: ",theme->file.c_str());
        }
      }
    mix.mix(out,n);
    }

  void setMusic(const Daedalus::GEngineClasses::C_MusicTheme &theme){
    std::lock_guard<std::mutex> guard(pendingSync);
    pendingMusic = &theme;
    }

  void stopMusic() {
    std::lock_guard<std::mutex> guard(pendingSync);
    pendingMusic = nullptr;
    mix.setMusic(Dx8::Music());
    }

  void setVolume(float v) {
    mix.setVolume(v);
    }

  Dx8::Mixer                                    mix;

  std::mutex                                    pendingSync;
  const Daedalus::GEngineClasses::C_MusicTheme* pendingMusic=nullptr;
  };

struct GameMusic::Impl final {
  Impl() {
    std::unique_ptr<MusicProducer> mix(new MusicProducer());
    dxMixer = mix.get();
    sound   = device.load(std::move(mix));

    dxMixer->setVolume(masterVolume);
    }

  void setMusic(const Daedalus::GEngineClasses::C_MusicTheme &theme) {
    if(currentMusic.file==theme.file)
      return;
    currentMusic = theme;
    dxMixer->setMusic(theme);
    sound.play();
    }

  void stopMusic() {
    dxMixer->stopMusic();
    }

  Tempest::SoundDevice                          device;
  Tempest::SoundEffect                          sound;

  MusicProducer*                                dxMixer=nullptr;
  Daedalus::GEngineClasses::C_MusicTheme        currentMusic;
  float                                         masterVolume=0.3f;
  bool                                          enableMusic=false;
  };

GameMusic::GameMusic() {
  impl.reset(new Impl());
  }

GameMusic::~GameMusic() {
  }

void GameMusic::setEnabled(bool e) {
  impl->enableMusic = e;
  if(!e)
    impl->stopMusic();
  }

bool GameMusic::isEnabled() const {
  return impl->enableMusic;
  }

void GameMusic::setMusic(const Daedalus::GEngineClasses::C_MusicTheme &theme) {
  if(!impl->enableMusic)
    return;
  impl->setMusic(theme);
  }

void GameMusic::stopMusic() {
  impl->stopMusic();
  }

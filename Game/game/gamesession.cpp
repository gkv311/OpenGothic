#include "gamesession.h"
#include "serialize.h"

#include "world/world.h"
#include "gothic.h"

// rate 14.5 to 1
const uint64_t GameSession::multTime=29;
const uint64_t GameSession::divTime =2;

GameSession::GameSession(Gothic &gothic, const RendererStorage &storage, std::string file)
  :gothic(gothic), storage(storage) {
  gothic.setLoadingProgress(0);
  setTime(gtime(8,0));
  uint8_t ver = gothic.isGothic2() ? 2 : 1;

  vm.reset(new WorldScript(*this));
  setWorld(std::unique_ptr<World>(new World(*this,storage,file,ver,[&](int v){
    gothic.setLoadingProgress(int(v*0.55));
    })));

  vm->initDialogs(gothic);
  gothic.setLoadingProgress(70);

  //const char* hero="PC_HERO";
  const char* hero="PC_ROCKEFELLER";
  //const char* hero="Giant_Bug";
  //const char* hero="OrcWarrior_Rest";
  //const char* hero = "Snapper";
  //const char* hero = "Lurker";
  //const char* hero="Scavenger";
  wrld->createPlayer(hero);
  wrld->postInit();

  initScripts(true);
  gothic.setLoadingProgress(96);
  }

GameSession::GameSession(Gothic &gothic, const RendererStorage &storage, Serialize &fin)
  :gothic(gothic), storage(storage) {
  gothic.setLoadingProgress(0);
  bool isG2=false;
  fin.read(ticks,wrldTimePart,wrldTime,isG2);

  uint8_t ver = isG2 ? 2 : 1;

  vm.reset(new WorldScript(*this,fin));
  setWorld(std::unique_ptr<World>(new World(*this,storage,fin,ver,[&](int v){
    gothic.setLoadingProgress(int(v*0.55));
    })));

  vm->initDialogs(gothic);
  gothic.setLoadingProgress(70);
  wrld->load(fin);
  vm->loadVar(fin);
  gothic.setLoadingProgress(96);
  }

GameSession::~GameSession() {
  }

void GameSession::save(Serialize &fout) {
  fout.write(ticks,wrldTimePart,wrldTime,bool(gothic.isGothic2()));
  vm->save(fout);
  if(wrld)
    wrld->save(fout);
  vm->saveVar(fout);
  }

void GameSession::setWorld(std::unique_ptr<World> &&w) {
  wrld = std::move(w);
  }

std::unique_ptr<World> GameSession::clearWorld() {
  if(wrld)
    wrld->view()->resetCmd();
  return std::move(wrld);
  }

void GameSession::changeWorld(const std::string& world, const std::string& wayPoint) {
  char buf[128]={};
  size_t beg = world.rfind('\\');
  size_t end = world.rfind('.');

  std::string wname;
  if(beg!=std::string::npos && end!=std::string::npos)
    wname = world.substr(beg+1,end-beg-1); else
    wname = world;

  std::snprintf(buf,sizeof(buf),"LOADING_%s.TGA",wname.c_str());  // final load-screen name, like "LOADING_OLDWORLD.TGA"

  // don't waste resources on update/sync
  auto hero = wrld->takeHero();
  hero->resetView(true);

  auto h = hero.release();
  gothic.startLoading(buf,[this,world,wayPoint,h](){
    implChangeWorld(std::unique_ptr<Npc>(h),world,wayPoint);
    });
  }

void GameSession::implChangeWorld(std::unique_ptr<Npc>&& hero,const std::string& world, const std::string& wayPoint) {
  const uint8_t ver = gothic.isGothic2() ? 2 : 1;
  const char*   w   = world.c_str();
  size_t        cut = world.rfind('\\');
  if(cut!=std::string::npos)
    w = w+cut+1;

  setWorld(std::unique_ptr<World>(new World(*this,storage,w,ver,[&](int v){
    gothic.setLoadingProgress(v);
    })));

  vm->resetVarPointers();

  hero->setTarget(nullptr);
  hero->setOther (nullptr);
  hero->resetView(false);
  wrld->insertPlayer(std::move(hero),wayPoint.c_str());

  initScripts(false);
  }

bool GameSession::isRamboMode() const {
  return gothic.isRamboMode();
  }

WorldView *GameSession::view() const {
  if(wrld)
    return wrld->view();
  return nullptr;
  }

std::vector<uint8_t> GameSession::loadScriptCode() {
  Tempest::RFile f(gothic.path()+u"/_work/data/Scripts/_compiled/GOTHIC.DAT");
  std::vector<uint8_t> ret(f.size());
  f.read(ret.data(),ret.size());
  return ret;
  }

SoundFx *GameSession::loadSoundFx(const char *name) {
  return gothic.loadSoundFx(name);
  }

void GameSession::emitGlobalSound(const Tempest::Sound &sfx) {
  gothic.emitGlobalSound(sfx);
  }

void GameSession::emitGlobalSound(const std::string &sfx) {
  gothic.emitGlobalSound(sfx);
  }

Npc* GameSession::player() {
  if(wrld)
    return wrld->player();
  return nullptr;
  }

void GameSession::setTime(gtime t) {
  wrldTime = t;
  }

void GameSession::tick(uint64_t dt) {
  ticks+=dt;

  uint64_t add = (dt+wrldTimePart)*multTime;
  wrldTimePart=add%divTime;

  wrldTime.addMilis(add/divTime);
  wrld->tick(dt);
  // std::this_thread::sleep_for(std::chrono::milliseconds(60));

  if(pendingChapter){
    if(aiIsDlgFinished()) {
      gothic.onIntroChapter(chapter);
      pendingChapter=false;
      }
    }
  }

void GameSession::updateAnimation() {
  if(wrld)
    wrld->updateAnimation();
  }

std::vector<WorldScript::DlgChoise> GameSession::updateDialog(const WorldScript::DlgChoise &dlg, Npc& player, Npc& npc) {
  return vm->updateDialog(dlg,player,npc);
  }

void GameSession::dialogExec(const WorldScript::DlgChoise &dlg, Npc& player, Npc& npc) {
  return vm->exec(dlg,player.handle(),npc.handle());
  }

const std::string &GameSession::messageByName(const std::string &id) const {
  if(!wrld){
    static std::string empty;
    return empty;
    }
  return vm->messageByName(id);
  }

uint32_t GameSession::messageTime(const std::string &id) const {
  if(!wrld){
    return 0;
    }
  return vm->messageTime(id);
  }

void GameSession::aiProcessInfos(Npc &player, Npc &npc) {
  gothic.aiProcessInfos(player,npc);
  }

bool GameSession::aiOutput(Npc &player, const char *msg) {
  return gothic.aiOuput(player,msg);
  }

void GameSession::aiForwardOutput(Npc &player, const char *msg) {
  return gothic.aiForwardOutput(player,msg);
  }

bool GameSession::aiCloseDialog() {
  return gothic.aiCloseDialog();
  }

bool GameSession::aiIsDlgFinished() {
  return gothic.aiIsDlgFinished();
  }

void GameSession::printScreen(const char *msg, int x, int y, int time, const Tempest::Font &font) {
  gothic.printScreen(msg,x,y,time,font);
  }

void GameSession::print(const char *msg) {
  gothic.print(msg);
  }

void GameSession::introChapter(const ChapterScreen::Show &s) {
  pendingChapter = true;
  chapter        = s;
  }

const FightAi::FA &GameSession::getFightAi(size_t i) const {
  return gothic.getFightAi(i);
  }

const Daedalus::GEngineClasses::C_MusicTheme &GameSession::getMusicTheme(const char *name) const {
  return gothic.getMusicTheme(name);
  }

void GameSession::initScripts(bool firstTime) {
  auto& wname = wrld->name();
  auto dot    = wname.rfind('.');
  auto name   = (dot==std::string::npos ? wname : wname.substr(0,dot));
  if( firstTime ) {
    std::string startup = "startup_"+name;

    if(vm->hasSymbolName(startup.c_str()))
      vm->runFunction(startup.c_str());
    }

  std::string init = "init_"+name;
  if(vm->hasSymbolName(init.c_str()))
    vm->runFunction(init.c_str());

  wrld->resetPositionToTA();
  }

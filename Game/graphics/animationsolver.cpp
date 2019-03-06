#include "animationsolver.h"

#include "skeleton.h"
#include "pose.h"
#include "posepool.h"
#include "world/interactive.h"

#include "resources.h"

using namespace Tempest;

AnimationSolver::AnimationSolver() {
  }

void AnimationSolver::setPos(const Matrix4x4 &m) {
  pos = m;
  head  .setObjMatrix(pos);
  sword .setObjMatrix(pos);
  bow   .setObjMatrix(pos);
  if(armour.isEmpty()) {
    view  .setObjMatrix(pos);
    } else {
    armour.setObjMatrix(pos);
    view  .setObjMatrix(Matrix4x4());
    }
  }

void AnimationSolver::setVisual(const Skeleton *v,uint64_t tickCount,WeaponState ws,WalkBit walk,Interactive* inter) {
  skeleton = v;

  current=NoAnim;
  setAnim(Idle,tickCount,ws,ws,walk,inter);

  head  .setSkeleton(skeleton);
  view  .setSkeleton(skeleton);
  armour.setSkeleton(skeleton);
  invalidateAnim(animSq,skeleton,tickCount);
  setPos(pos); // update obj matrix
  }

void AnimationSolver::setVisualBody(StaticObjects::Mesh&& h, StaticObjects::Mesh &&body) {
  head    = std::move(h);
  view    = std::move(body);

  head.setSkeleton(skeleton,"BIP01 HEAD");
  view.setSkeleton(skeleton);
  }

bool AnimationSolver::setAnim(Anim a,uint64_t tickCount,WeaponState nextSt,WeaponState weaponSt,WalkBit walk,Interactive* inter) {
  if(animSq!=nullptr){
    if(current==a && nextSt==weaponSt && animSq.cls==Animation::Loop)
      return true;
    if((animSq.cls==Animation::Transition &&
        current!=RotL && current!=RotR && current!=MoveL && current!=MoveR && // no idea why this animations maked as Transition
        !(current==Move && a==Jump)) && // allow to jump at any point of run animation
       !animSq.isFinished(tickCount-sAnim))
      return false;
    if(current==Atack && !animSq.isFinished(tickCount-sAnim) && tickCount-sAnim<1000)
      return false;
    }
  auto ani = solveAnim(a,weaponSt,current,nextSt,walk,inter);
  if(ani==nullptr)
    ani = solveAnim(Idle,WeaponState::NoWeapon,Idle,WeaponState::NoWeapon,WalkBit::WM_Run,nullptr);
  prevAni  = current;
  current  = a;
  if(ani==animSq) {
    if(animSq.cls==Animation::Transition){
      invalidateAnim(ani,skeleton,tickCount); // restart anim
      }
    return true;
    }
  invalidateAnim(ani,skeleton,tickCount);
  return true;
  }

bool AnimationSolver::isFlyAnim(uint64_t tickCount) const {
  if(animSq==nullptr)
    return false;
  return animSq.isFly() && !animSq.isFinished(tickCount-sAnim) &&
         current!=Fall && current!=FallDeep;
  }

void AnimationSolver::invalidateAnim(const Sequence ani,const Skeleton* sk,uint64_t tickCount) {
  animSq = ani;
  sAnim  = tickCount;
  if(ani.l0)
    skInst = PosePool::get(sk,ani.l0,ani.l1,sAnim); else
    skInst = PosePool::get(sk,ani.l1,sAnim);
  }

void AnimationSolver::addOverlay(const Skeleton* sk,uint64_t time,uint64_t tickCount,WalkBit wlk,Interactive* inter) {
  if(sk==nullptr)
    return;
  if(time!=0)
    time+=tickCount;

  Overlay ov = {sk,time};
  overlay.push_back(ov);
  if(animSq!=nullptr) {
    auto ani=animSequence(animSq.name());
    invalidateAnim(ani,skeleton,tickCount);
    } else {
    // fallback
    setAnim(Idle,tickCount,WeaponState::NoWeapon,WeaponState::NoWeapon,wlk,inter);
    }
  }

void AnimationSolver::delOverlay(const char *sk) {
  if(overlay.size()==0)
    return;
  auto skelet = Resources::loadSkeleton(sk);
  delOverlay(skelet);
  }

void AnimationSolver::delOverlay(const Skeleton *sk) {
  for(size_t i=0;i<overlay.size();++i)
    if(overlay[i].sk==sk){
      overlay.erase(overlay.begin()+int(i));
      return;
      }
  }

void AnimationSolver::updateAnimation(uint64_t tickCount) {
  for(size_t i=0;i<overlay.size();){
    auto& ov = overlay[i];
    if(ov.time!=0 && ov.time<tickCount)
      overlay.erase(overlay.begin()+int(i)); else
      ++i;
    }

  if(skInst!=nullptr){
    uint64_t dt = tickCount - sAnim;
    skInst->update(dt);

    head .setSkeleton(*skInst,pos);
    sword.setSkeleton(*skInst,pos);
    bow  .setSkeleton(*skInst,pos);
    if(armour.isEmpty())
      view  .setSkeleton(*skInst,pos); else
      armour.setSkeleton(*skInst,pos);
    }
  }

AnimationSolver::Sequence AnimationSolver::solveAnim(Anim a,   WeaponState st0,
                                                      Anim cur, WeaponState st,
                                                      WalkBit wlkMode,
                                                      Interactive* inter) const {
  if(skeleton==nullptr)
    return nullptr;

  if(st0==WeaponState::NoWeapon){
    if(a==Anim::Idle && cur==a && st==WeaponState::W1H)
      return animSequence("T_1H_2_1HRUN");
    if(a==Anim::Move && cur==a && st==WeaponState::W1H)
      return animSequence("T_MOVE_2_1HMOVE");
    if(a==Anim::Idle && cur==a && st==WeaponState::W2H)
      return animSequence("T_RUN_2_2H");
    if(a==Anim::Move && cur==a && st==WeaponState::W2H)
      return animSequence("T_MOVE_2_2HMOVE");
    if(a==Anim::Idle && cur==a && st==WeaponState::Bow)
      return animSequence("T_RUN_2_BOW");
    if(a==Anim::Move && cur==a && st==WeaponState::Bow)
      return animSequence("T_MOVE_2_BOWMOVE");
    if(a==Anim::Idle && cur==a && st==WeaponState::CBow)
      return animSequence("T_RUN_2_CBOW");
    if(a==Anim::Move && cur==a && st==WeaponState::CBow)
      return animSequence("T_MOVE_2_CBOWMOVE");
    if(a==Anim::Idle && cur==a && st==WeaponState::Mage)
      return animSequence("T_MOVE_2_MAGMOVE");
    if(a==Anim::Move && cur==a && st==WeaponState::Mage)
      return animSequence("T_MOVE_2_MAGMOVE");
    }
  if(st0==WeaponState::W1H && st==WeaponState::NoWeapon){
    if(a==Anim::Idle && cur==a)
      return animSequence("T_1HMOVE_2_MOVE");
    if(a==Anim::Move && cur==a)
      return animSequence("T_1HMOVE_2_MOVE");
    }
  if(st0==WeaponState::W2H && st==WeaponState::NoWeapon){
    if(a==Anim::Idle && cur==a)
      return animSequence("T_RUN_2_2H");
    if(a==Anim::Move && cur==a)
      return animSequence("T_2HMOVE_2_MOVE");
    }
  if(st0==WeaponState::Mage && st==WeaponState::NoWeapon){
    if(a==Anim::Idle && cur==a)
      return animSequence("T_RUN_2_MAG");
    if(a==Anim::Move && cur==a)
      return animSequence("T_MAGMOVE_2_MOVE");
    }

  if(true) {
    if(st0==WeaponState::Fist && st==WeaponState::NoWeapon)
      return animSequence("T_FISTMOVE_2_MOVE");
    if(st0==WeaponState::NoWeapon && st==WeaponState::Fist)
      return solveAnim("S_%sRUN",st);
    }

  if(inter!=nullptr) {
    if(cur!=Interact && a==Interact)
      return animSequence(inter->anim(Interactive::In));
    if(cur==Interact && a==Interact)
      return animSequence(inter->anim(Interactive::Active));
    if(cur==Interact && a!=Interact)
      return animSequence(inter->anim(Interactive::Out));
    }

  if(st==WeaponState::Fist) {
    if(a==Anim::Atack && cur==Move)
      return animSequence("T_FISTATTACKMOVE");
    if(a==Anim::Atack)
      return animSequence("S_FISTATTACK");
    if(a==Anim::AtackBlock)
      return animSequence("T_FISTPARADE_0");
    }
  else if(st==WeaponState::W1H) {
    if(a==Anim::Atack && cur==Move)
      return layredSequence("S_1HRUNL","T_1HATTACKMOVE");
    if(a==Anim::Atack)
      return animSequence("S_1HATTACK");
    if(a==Anim::AtackL)
      return animSequence("T_1HATTACKL");
    if(a==Anim::AtackR)
      return animSequence("T_1HATTACKR");
    if(a==Anim::AtackBlock) {
      switch(std::rand()%3){
        case 0: return animSequence("T_1HPARADE_0");
        case 1: return animSequence("T_1HPARADE_0_A2");
        case 2: return animSequence("T_1HPARADE_0_A3");
        }
      }
    }
  else if(st==WeaponState::W2H) {
    if(a==Anim::Atack && cur==Move)
      return layredSequence("S_2HRUNL","T_2HATTACKMOVE");
    if(a==Anim::Atack)
      return animSequence("S_2HATTACK");
    if(a==Anim::AtackL)
      return animSequence("T_2HATTACKL");
    if(a==Anim::AtackR)
      return animSequence("T_2HATTACKR");
    if(a==Anim::AtackBlock) {
      switch(std::rand()%3){
        case 0: return animSequence("T_2HPARADE_0");
        case 1: return animSequence("T_2HPARADE_0_A2");
        case 2: return animSequence("T_2HPARADE_0_A3");
        }
      }
    }
  else if(st==WeaponState::Mage) {
    if(cur!=Anim::Atack && a==Atack)
      return animSequence("T_MAGRUN_2_TRFSHOOT");
    if(cur==Anim::Atack && a==Atack)
      return animSequence("S_TRFSHOOT");
    if(cur==Anim::Atack && a==Idle)
      return animSequence("T_TRFSHOOT_2_STAND");
    }

  if((cur==Anim::Idle || cur==Anim::NoAnim) && a==Anim::Idle){
    if(bool(wlkMode&WalkBit::WM_Walk))
      return solveAnim("S_%sWALK",st); else
      return solveAnim("S_%sRUN", st);
    }
  if(cur==Anim::Idle && a==Move) {
    if(bool(wlkMode&WalkBit::WM_Walk))
      return solveAnim("T_%sWALK_2_%sWALKL",st); else
      return solveAnim("T_%sRUN_2_%sRUNL",  st);
    }
  if(cur==Anim::Move && a==cur){
    if(bool(wlkMode&WalkBit::WM_Walk))
      return solveAnim("S_%sWALKL",st); else
      return solveAnim("S_%sRUNL", st);
    }
  if(cur==Anim::Move && a==Idle) {
    if(bool(wlkMode&WalkBit::WM_Walk))
      return solveAnim("T_%sWALKL_2_%sWALK",st); else
      return solveAnim("T_%sRUNL_2_%sRUN",st);
    }

  if(a==Anim::RotL){
    if(bool(wlkMode&WalkBit::WM_Walk)){
      return solveAnim("T_%sWALKWTURNL",st);
      } else
      return solveAnim("T_%sRUNTURNL",st);
    }
  if(a==Anim::RotR){
    if(bool(wlkMode&WalkBit::WM_Walk)){
      return solveAnim("T_%sWALKWTURNR",st);
      } else
      return solveAnim("T_%sRUNTURNR",st);
    }
  if(a==Anim::MoveL) {
    return solveAnim("T_%sRUNSTRAFEL",st);
    }
  if(a==Anim::MoveR) {
    return solveAnim("T_%sRUNSTRAFER",st);
    }
  if(a==Anim::MoveBack)
    return solveAnim("T_%sJUMPB",st);

  if(cur==Anim::Move && a==Jump)
    return animSequence("T_RUNL_2_JUMP");
  if(cur==Anim::Idle && a==Anim::Jump)
    return animSequence("T_STAND_2_JUMP");
  if(cur==Anim::Jump && a==Anim::Idle)
    return animSequence("T_JUMP_2_STAND");
  if(a==Anim::Jump)
    return animSequence("S_JUMP");
  if(cur==Anim::Fall && a==Move)
    return animSequence("T_RUN_2_RUNL");

  if(cur==Anim::Idle && a==Anim::JumpUpLow)
    return animSequence("T_STAND_2_JUMPUPLOW");
  if(cur==Anim::JumpUpLow && a==Anim::Idle)
    return animSequence("T_JUMPUPLOW_2_STAND");
  if(a==Anim::JumpUpLow)
    return animSequence("S_JUMPUPLOW");

  if(cur==Anim::Idle && a==Anim::JumpUpMid)
    return animSequence("T_STAND_2_JUMPUPMID");
  if(cur==Anim::JumpUpMid && a==Anim::Idle)
    return animSequence("T_JUMPUPMID_2_STAND");
  if(a==Anim::JumpUpMid)
    return animSequence("S_JUMPUPMID");

  if(cur==Anim::Idle && a==Anim::JumpUp)
    return animSequence("T_STAND_2_JUMPUP");
  if(a==Anim::JumpUp)
    return animSequence("S_JUMPUP");

  if(cur==Anim::Idle && a==Anim::GuardL)
    return animSequence("T_STAND_2_LGUARD");
  if(a==Anim::GuardL)
    return animSequence("S_LGUARD");

  if(cur==Anim::Idle && a==Anim::GuardH)
    return animSequence("T_STAND_2_HGUARD");
  if(a==Anim::GuardH)
    return animSequence("S_HGUARD");

  if(cur==Anim::Idle && a==Anim::Talk)
    return animSequence("T_STAND_2_TALK");
  if(a==Anim::Talk)
    return animSequence("S_TALK");

  if(cur==Anim::Idle && a==Anim::Eat)
    return animSequence("T_STAND_2_EAT");
  if(cur==Anim::Eat && a==Anim::Idle)
    return animSequence("T_EAT_2_STAND");
  if(a==Anim::Eat)
    return animSequence("S_EAT");

  if(cur==Anim::Idle && a==Anim::Sleep)
    return animSequence("T_STAND_2_SLEEP");
  if(cur==Anim::Sleep && a==Anim::Idle)
    return animSequence("T_SLEEP_2_STAND");
  if(a==Anim::Sleep)
    return animSequence("S_SLEEP");

  if(cur==Anim::Idle && a==Anim::GuardSleep)
    return animSequence("T_STAND_2_GUARDSLEEP");
  if(cur==Anim::GuardSleep && a==Anim::Idle)
    return animSequence("T_GUARDSLEEP_2_STAND");
  if(a==Anim::GuardSleep)
    return animSequence("S_GUARDSLEEP");

  if(cur==Anim::Idle && (Anim::MagFirst<=a && a<=Anim::MagLast))
    return solveMag("T_MAGRUN_2_%sSHOOT",a);
  if((Anim::MagFirst<=cur && cur<=Anim::MagLast) && a==Anim::Idle)
    return solveMag("T_%sSHOOT_2_STAND",cur);
  if(Anim::MagFirst<=a && a<=Anim::MagLast)
    return solveMag("S_%sSHOOT",a);
  if(a==Anim::MagNoMana)
    return animSequence("T_CASTFAIL");

  if(cur==Anim::Idle && a==Anim::Sit)
    return animSequence("T_STAND_2_SIT");
  if(cur==Anim::Sit && a==Anim::Idle)
    return animSequence("T_SIT_2_STAND");
  if(a==Anim::Sit)
    return animSequence("S_SIT");

  if(a==Anim::GuardLChLeg)
    return animSequence("T_LGUARD_CHANGELEG");
  if(a==Anim::GuardLScratch)
    return animSequence("T_LGUARD_SCRATCH");
  if(a==Anim::GuardLStrectch)
    return animSequence("T_LGUARD_STRETCH");
  if(a==Anim::Perception)
    return animSequence("T_PERCEPTION");
  if(a==Anim::Lookaround)
    return animSequence("T_HGUARD_LOOKAROUND");
  if(a==Anim::Training)
    return animSequence("T_1HSFREE");
  if(a==Anim::Warn)
    return animSequence("T_WARN");

  if(a==Anim::Fall)
    return animSequence("S_FALLDN");
  //if(cur==Fall && a==Anim::FallDeep)
  //  return animSequence("T_FALL_2_FALLEN");
  if(a==Anim::FallDeep)
    return animSequence("S_FALL");
  if(a==Anim::SlideA)
    return animSequence("S_SLIDE");
  if(a==Anim::SlideB)
    return animSequence("S_SLIDEB");

  if(a==Anim::Chair1)
    return animSequence("R_CHAIR_RANDOM_1");
  if(a==Anim::Chair2)
    return animSequence("R_CHAIR_RANDOM_2");
  if(a==Anim::Chair3)
    return animSequence("R_CHAIR_RANDOM_3");
  if(a==Anim::Chair4)
    return animSequence("R_CHAIR_RANDOM_4");

  if(a==Anim::Roam1)
    return animSequence("R_ROAM1");
  if(a==Anim::Roam2)
    return animSequence("R_ROAM2");
  if(a==Anim::Roam3)
    return animSequence("R_ROAM3");

  // FALLBACK
  if(a==Anim::Move)
    return solveAnim("S_%sRUNL",st);
  if(a==Anim::Idle)  {
    if(auto idle=solveAnim("S_%sRUN",st))
      return idle;
    return solveAnim("S_%sWALK",st);
    }
  return nullptr;
  }

AnimationSolver::Sequence AnimationSolver::solveAnim(const char *format, WeaponState st) const {
  static const char* weapon[] = {
    "",
    "FIST",
    "1H",
    "2H",
    "BOW",
    "CBOW",
    "MAG"
    };
  char name[128]={};
  std::snprintf(name,sizeof(name),format,weapon[int(st)],weapon[int(st)]);
  if(auto ret=animSequence(name))
    return ret;
  std::snprintf(name,sizeof(name),format,"");
  if(auto ret=animSequence(name))
    return ret;
  std::snprintf(name,sizeof(name),format,"FIST");
  return animSequence(name);
  }

AnimationSolver::Sequence AnimationSolver::solveMag(const char *format,Anim spell) const {
  static const char* mg[]={"FIB", "WND", "HEA", "PYR", "FEA", "TRF", "SUM", "MSD", "STM", "FRZ", "SLE", "WHI", "SCK"};

  char name[128]={};
  std::snprintf(name,sizeof(name),format,mg[spell-Anim::MagFirst]);
  return animSequence(name);
  }

AnimationSolver::Sequence AnimationSolver::animSequence(const char *name) const {
  for(size_t i=overlay.size();i>0;){
    --i;
    if(auto s = overlay[i].sk->sequence(name))
      return s;
    }
  return skeleton ? skeleton->sequence(name) : nullptr;
  }

AnimationSolver::Sequence AnimationSolver::layredSequence(const char *name,const char* base) const {
  auto a = animSequence(name);
  auto b = animSequence(base);
  return Sequence(a.l1,b.l1);
  }

AnimationSolver::Anim AnimationSolver::animByName(const std::string &name) const {
  if(name=="T_STAND_2_LGUARD" || name=="S_LGUARD")
    return Anim::GuardL;
  if(name=="T_STAND_2_HGUARD")
    return Anim::GuardH;
  if(name=="T_LGUARD_2_STAND" || name=="T_HGUARD_2_STAND")
    return Anim::Idle;
  if(name=="T_STAND_2_TALK" || name=="S_TALK")
    return Anim::Talk;
  if(name=="T_PERCEPTION")
    return Anim::Perception;
  if(name=="T_HGUARD_LOOKAROUND")
    return Anim::Lookaround;
  if(name=="T_STAND_2_EAT" || name=="T_EAT_2_STAND" || name=="S_EAT")
    return Anim::Eat;
  if(name=="T_STAND_2_SLEEP" || name=="T_SLEEP_2_STAND" || name=="S_SLEEP")
    return Anim::Sleep;
  if(name=="T_STAND_2_GUARDSLEEP" || name=="T_GUARDSLEEP_2_STAND" || name=="S_GUARDSLEEP")
    return Anim::GuardSleep;
  if(name=="T_STAND_2_SIT" || name=="T_SIT_2_STAND" || name=="S_SIT")
    return Anim::Sit;
  if(name=="T_LGUARD_CHANGELEG")
    return Anim::GuardLChLeg;
  if(name=="T_LGUARD_STRETCH")
    return Anim::GuardLStrectch;
  if(name=="T_LGUARD_SCRATCH")
    return Anim::GuardLScratch;
  if(name=="T_1HSFREE")
    return Anim::Training;
  if(name=="T_MAGRUN_2_HEASHOOT" || name=="S_HEASHOOT" || name=="T_HEASHOOT_2_STAND")
    return Anim::MagHea;
  if(name=="T_WARN")
    return Anim::Warn;
  if(name=="R_CHAIR_RANDOM_1")
    return Anim::Chair1;
  if(name=="R_CHAIR_RANDOM_2")
    return Anim::Chair2;
  if(name=="R_CHAIR_RANDOM_3")
    return Anim::Chair3;
  if(name=="R_CHAIR_RANDOM_4")
    return Anim::Chair4;
  if(name=="R_ROAM1")
    return Anim::Roam1;
  if(name=="R_ROAM2")
    return Anim::Roam2;
  if(name=="R_ROAM3")
    return Anim::Roam3;
  return Anim::NoAnim;
  }
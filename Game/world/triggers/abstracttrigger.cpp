#include "abstracttrigger.h"

#include <Tempest/Log>

using namespace Tempest;

AbstractTrigger::AbstractTrigger(ZenLoad::zCVobData &&data, World &owner)
  :data(std::move(data)), owner(owner) {
  }

ZenLoad::zCVobData::EVobType AbstractTrigger::vobType() const {
  return data.vobType;
  }

const std::string &AbstractTrigger::name() const {
  return data.vobName;
  }

void AbstractTrigger::onTrigger(const TriggerEvent&) {
  Log::d("TODO: trigger[",name(),";",data.objectClass,"]");
  }

void AbstractTrigger::onIntersect(Npc &) {
  TriggerEvent e(false);
  onTrigger(e);
  }

bool AbstractTrigger::hasVolume() const {
  auto& b = data.bbox;
  if( b[0].x < b[1].x &&
      b[0].y < b[1].y &&
      b[0].z < b[1].z)
    return true;
  return false;
  }

bool AbstractTrigger::checkPos(float x,float y,float z) const{
  auto& b = data.bbox;
  if( b[0].x < x && x < b[1].x &&
      b[0].y < y && y < b[1].y &&
      b[0].z < z && z < b[1].z)
    return true;
  return false;
  }

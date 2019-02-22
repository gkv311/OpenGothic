#include "item.h"

#include "world.h"
#include "worldscript.h"

Item::Item(WorldScript& owner,Daedalus::GameState::ItemHandle hitem)
  :owner(owner),hitem(hitem){
  }

void Item::setView(StaticObjects::Mesh &&m) {
  view = std::move(m);
  updateMatrix();
  }

void Item::setPosition(float x, float y, float z) {
  pos={{x,y,z}};
  updateMatrix();
  }

void Item::setDirection(float, float, float) {
  }

void Item::setMatrix(const Tempest::Matrix4x4 &m) {
  pos[0] = m.at(3,0);
  pos[1] = m.at(3,1);
  pos[2] = m.at(3,2);
  view.setObjMatrix(m);
  }

const char *Item::displayName() const {
  return owner.vmItem(hitem).name.c_str();
  }

const char *Item::description() const {
  return owner.vmItem(hitem).description.c_str();
  }

std::array<float,3> Item::position() const {
  return pos;
  }

bool Item::isGold() const {
  return owner.vmItem(hitem).instanceSymbol==owner.goldId();
  }

const char *Item::uiText(size_t id) const {
  auto& v = owner.vmItem(hitem);
  return v.text[id].c_str();
  }

int32_t Item::uiValue(size_t id) const {
  auto& v = owner.vmItem(hitem);
  return v.count[id];
  }

size_t Item::count() const {
  auto& v = owner.vmItem(hitem);
  return v.amount;
  }

int32_t Item::cost() const {
  auto& v = owner.vmItem(hitem);
  return v.value;
  }

size_t Item::clsId() const {
  return owner.vmItem(hitem).instanceSymbol;
  }

void Item::updateMatrix() {
  Tempest::Matrix4x4 m;
  m.identity();
  m.translate(pos[0],pos[1],pos[2]);
  view.setObjMatrix(m);
  }
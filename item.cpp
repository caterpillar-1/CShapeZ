#include "item.h"

Item::Item() {}

void Item::paint(QPainter *painter) const {
  qWarning() << "default item image is painted.";
  painter->save();
  painter->setPen(Qt::red);
  painter->drawRect(QRectF(-R, -R, R, R));
  painter->drawLine(-R, -R, R, R);
  painter->drawLine(R, -R, -R, R);
  painter->restore();
}

TraitMine::TraitMine(trait_t trait) : trait(trait) {}

trait_t TraitMine::getTrait() const { return trait; }

Mine::Mine(type_t type, shape_t shape, rotate_t rotate, trait_t trait)
    : type(type), shape(shape), rotate(rotate), trait(trait) {}

Mine::Mine(const Mine &o)
    : type(o.type), shape(o.shape), rotate(o.rotate), trait(o.trait) {}

bool Mine::operator==(const Mine &o) const {
  return type == o.type && shape == o.shape && trait == o.trait;
}

const Mine *Mine::setTrait(trait_t trait) const {
  return new Mine(type, shape, rotate, trait);
}

const Mine *Mine::rotateR() const {
  return new Mine(type, shape, rotate_t((rotate + 3) % 4), trait);
}

const Mine *Mine::cutUpper() const {
  switch (shape) {
  case FULL:
    return new Mine(type, HALF, R0, trait);
  case HALF:
    switch (rotate) {
    case R0:
      return new Mine(*this);
    case R90:
      return new Mine(type, QUARTER, R90, trait);
    case R180:
      return nullptr;
    case R270:
      return new Mine(type, QUARTER, R0, trait);
    }
  case QUARTER:
    switch (rotate) {
    case R0:
    case R90:
      return new Mine(*this);
    default:
      return nullptr;
    }
  }
}

const Mine *Mine::cutLower() const {
  switch (shape) {
  case FULL:
    return new Mine(type, HALF, R180, trait);
  case HALF:
    switch (rotate) {
    case R0:
      return nullptr;
    case R90:
      return new Mine(type, QUARTER, R180, trait);
    case R180:
      return new Mine(*this);
    case R270:
      return new Mine(type, QUARTER, R270, trait);
    }
  case QUARTER:
    switch (rotate) {
    case R0:
    case R90:
      return nullptr;
    default:
      return new Mine(*this);
    }
  }
}

void Mine::paint(QPainter *painter) const {
  painter->save();
  painter->setBrush(QBrush(Qt::GlobalColor(trait)));
  QRectF bound(-R, -R, 2*R, 2*R);
  switch (type) {
  case ROUND:
    painter->drawPie(bound, rotate * 90 * 16, shape * 90 * 16);
    break;
  case SQUARE:
    painter->rotate(-rotate * 90);
    switch (shape) {
    case FULL:
      painter->drawRect(QRectF(-R, 0, R, R));
      painter->drawRect(QRectF(0, 0, R, R));
    case HALF: // fall through
      painter->drawRect(QRectF(-R, -R, R, R));
    case QUARTER: // fall through
      painter->drawRect(QRectF(0, -R, R, R));
    }
  }
  painter->restore();
}

ItemFactory::~ItemFactory() {}

MineFactory::MineFactory(type_t type, trait_t trait)
    : type(type), trait(trait) {}

Mine *MineFactory::createItem() const {
  return new Mine(type, FULL, R0, trait);
}

TraitFactory::TraitFactory(trait_t trait) : trait(trait) {}

TraitMine *TraitFactory::createItem() const { return new TraitMine(trait); }

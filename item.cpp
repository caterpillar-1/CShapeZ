#include "item.h"
#include "config.h"

Item::Item() {}

Mine::Mine(enum type_t type_, enum shape_t shape_, rotate_t rotate_,
           enum trait_t trait_)
    : type_(type_), shape_(shape_), rotate_(rotate_), trait_(trait_) {}

Mine::Mine(const Mine &o)
    : type_(o.type_), shape_(o.shape_), rotate_(o.rotate_), trait_(o.trait_) {}

bool Mine::operator==(const Mine &o) {
  return type_ == o.type_ && shape_ == o.shape_ && trait_ == o.trait_;
}

void Item::advance(int phase) {}

// format: off

Mine *Mine::getUpperHalf() {
  switch (shape_) {
  case FULL:
    return new Mine(type_, HALF, R0, trait_);
  case HALF:
    switch (rotate_) {
    case R0:
      return new Mine(*this);
    case R90:
      return new Mine(type_, QUARTER, R90, trait_);
    case R180:
      return nullptr;
    case R270:
      return new Mine(type_, QUARTER, R0, trait_);
    }
  case QUARTER:
    switch (rotate_) {
    case R0:
    case R90:
      return new Mine(*this);
    default:
      return nullptr;
    }
  }
  return nullptr;
}

Mine *Mine::getLowerHalf() {
  switch (shape_) {
  case FULL:
    return new Mine(type_, HALF, R180, trait_);
  case HALF:
    switch (rotate_) {
    case R0:
      return nullptr;
    case R90:
      return new Mine(type_, QUARTER, R180, trait_);
    case R180:
      return new Mine(*this);
    case R270:
      return new Mine(type_, QUARTER, R270, trait_);
    }
  case QUARTER:
    switch (rotate_) {
    case R0:
    case R90:
      return nullptr;
    default:
      return new Mine(*this);
    }
  }
  return nullptr;
}

Mine *Mine::getLeftHalf() {
  switch (shape_) {
  case FULL:
    return new Mine(type_, HALF, R90, trait_);
  case HALF:
    switch (rotate_) {
    case R0:
      return new Mine(type_, QUARTER, R90, trait_);
    case R90:
      return new Mine(*this);
    case R180:
      return new Mine(type_, QUARTER, R180, trait_);
    case R270:
      return nullptr;
    }
  case QUARTER:
    switch (rotate_) {
    case R90:
    case R180:
      return new Mine(*this);
    default:
      return nullptr;
    }
  }
  return nullptr;
}

Mine *Mine::getRightHalf() {
  switch (shape_) {
  case FULL:
    return new Mine(type_, HALF, R270, trait_);
  case HALF:
    switch (rotate_) {
    case R0:
      return new Mine(type_, QUARTER, R0, trait_);
    case R90:
      return nullptr;
    case R180:
      return new Mine(type_, QUARTER, R270, trait_);
    case R270:
      return new Mine(*this);
    }
  case QUARTER:
    switch (rotate_) {
    case R0:
    case R270:
      return new Mine(*this);
    default:
      return nullptr;
    }
  }
  return nullptr;
}

Mine *Mine::getRotateLeft() {
  return new Mine(type_, shape_, rotate_t((rotate_ + 1) % 4), trait_);
}

Mine *Mine::getRotateRight() {
  return new Mine(type_, shape_, rotate_t((rotate_ + 3) % 4), trait_);
}

void Mine::addTrait(TraitMine &t) { trait_ = t.getTrait(); }
// format: on

QRectF Mine::boundingRect() const { return QRectF(0, 0, TILE_W, TILE_H); }

QPainterPath Mine::shape() const {
  QPainterPath path;
  QRectF bound = boundingRect();
  path.addEllipse(bound);
  return path;
}

void Mine::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                 QWidget *) {
  painter->save();
  QBrush brush((Qt::GlobalColor)trait_);
  QRectF bound = boundingRect();
  painter->setBrush(brush);
  switch (type_) {
  case ROUND:
    painter->drawPie(bound, rotate_ * 90 * 16, shape_ * 90 * 16);
    break;
  case SQUARE:
    painter->translate(TILE_W / 2, TILE_H / 2);
    painter->rotate(-rotate_ * 90);
    switch (shape_) {
    case FULL:
      painter->drawRect(QRectF(-TILE_W / 2, 0, TILE_W / 2, TILE_H / 2));
      painter->drawRect(QRectF(0, 0, TILE_W / 2, TILE_H / 2));
    case HALF: // fall through
      painter->drawRect(
          QRectF(-TILE_W / 2, -TILE_H / 2, TILE_W / 2, TILE_H / 2));
    case QUARTER: // fall through
      painter->drawRect(QRectF(0, -TILE_H / 2, TILE_W / 2, TILE_H / 2));
    }
  }
  painter->restore();
}

TraitMine::TraitMine(trait_t trait_) : trait_(trait_) {}

trait_t TraitMine::getTrait() { return trait_; }

QRectF TraitMine::boundingRect() const { return QRect(0, 0, TILE_W, TILE_H); }

QPainterPath TraitMine::shape() const {
  QPainterPath path;
  QRectF bound = boundingRect();
  path.addEllipse(bound);
  return path;
}

void TraitMine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
  painter->save();
  QBrush brush((Qt::GlobalColor)trait_);
  painter->setBrush(brush);
  painter->drawEllipse(QPoint(TILE_W / 2, TILE_H / 2), TILE_W / 2, TILE_H / 4);
  painter->restore();
}


MineFactory::MineFactory(type_t type, trait_t trait)
  : type(type), trait(trait)
{

}

Mine *MineFactory::createItem()
{
  return new Mine(type, FULL, R0, trait);
}

TraitFactory::TraitFactory(trait_t trait)
  : trait(trait)
{

}

TraitMine *TraitFactory::createItem()
{
  return new TraitMine(trait);
}

ItemFactory::~ItemFactory()
{

}

#include "item.h"

static int getInt(QDataStream &in) {
  int x;
  in >> x;
  return x;
}

Item::Item() {}

void Item::paint(QPainter *painter) const {
  qWarning() << "default item image is painted.";
  painter->save();
  painter->setPen(Qt::red);
  painter->drawRect(QRectF(-R, -R, 2*R, 2*R));
  painter->drawLine(-R, -R, R, R);
  painter->drawLine(R, -R, -R, R);
  painter->restore();
}

TraitMine::TraitMine(trait_t trait) : trait(trait) {}

trait_t TraitMine::getTrait() const { return trait; }

Mine::Mine(type_t type, shape_t shape, rotate_t rotate, trait_t trait)
    : type(type), shape(shape), rotate(rotate), trait(trait) {

}

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
  painter->setPen(QPen(Qt::darkGray, L/16));
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

void ItemFactory::save(QDataStream &out)
{
}

ItemFactory::ItemFactory()
{

}

ItemFactory::~ItemFactory() {}

ItemFactory::ItemFactory(QDataStream &in)
{

}

MineFactory::MineFactory(type_t type, trait_t trait)
  : type(type), trait(trait) {}

MineFactory::MineFactory(QDataStream &in)
  : ItemFactory(in)
{
  in >> type >> trait;
}

void MineFactory::save(QDataStream &out)
{
  ItemFactory::save(out);
  out << type << trait;
}

Mine *MineFactory::createItem() const {
  return new Mine(type, FULL, R0, trait);
}

TraitFactory::TraitFactory(trait_t trait) : trait(trait) {}

TraitFactory::TraitFactory(QDataStream &in)
  : ItemFactory(in)
{
  in >> trait;
}

void TraitFactory::save(QDataStream &out)
{
  out << trait;
}

QColor TraitFactory::color()
{
  return Qt::GlobalColor(trait);
}

TraitMine *TraitFactory::createItem() const { return new TraitMine(trait); }

QDataStream &operator<<(QDataStream &out, Mine *&mine) {
  assert(mine);
  out << mine->type << mine->shape << mine->rotate << mine->trait;
  return out;
}

QDataStream &operator>>(QDataStream &in, Mine *&mine) {
  type_t type;
  shape_t shape;
  rotate_t rotate;
  trait_t trait;
  in >> type >> shape >> rotate >> trait;
  mine = new Mine(type, shape, rotate, trait);
  return in;
}

QDataStream &operator<<(QDataStream &out, TraitMine *&tmine) {
  assert(tmine);
  out << tmine->trait;
  return out;
}

QDataStream &operator>>(QDataStream &in, TraitMine *&tmine) {
  trait_t trait;
  in >> trait;
  tmine = new TraitMine(trait);
  return in;
}

QDataStream &operator<<(QDataStream &out, Item *&item) {
  if (Mine *mine = dynamic_cast<Mine *>(item)) {
    out << QChar('M') << mine;
  } else if (TraitMine *tmine = dynamic_cast<TraitMine *>(item)) {
    out << QChar('T') << tmine;
  } else {
    assert(false);
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, Item *&item) {
  QChar c;
  in >> c;
  if (c == 'M') {
    Mine *mine;
    in >> mine;
    item = mine;
  } else if (c == 'T') {
    TraitMine *tmine;
    in >> tmine;
    item = tmine;
  } else {
    assert(false);
  }
  return in;
}

ItemFactory *randomItemFactory()
{
  if (rng.generate() % 1000 < 300) { // TraitFactory
    qDebug() << "CREATE TRAITFACTORY";
    trait_t trait = (rng.generate() & 1) ? RED : BLUE;
    return new TraitFactory(trait);
  } else {
    qDebug() << "CREATE MINEFACTORY";
    trait_t trait;
    type_t type;
    trait = trait_t(rng.generate() % 3);
    type = type_t(rng.generate() % 2);
    return new MineFactory(type, trait);
  }
}

// serialize
void saveItemFactory(QDataStream &out, ItemFactory *f) {
  if (f == nullptr) {
    out << QChar('N');
    return;
  }
  if (auto mf = dynamic_cast<MineFactory *>(f)) {
    out << QChar('M');
    mf->save(out);
  } else if (auto tf = dynamic_cast<TraitFactory *>(f)) {
    out << QChar('T');
    tf->save(out);
  } else {
    assert(false);
  }
}

ItemFactory *loadItemFactory(QDataStream &in) {
  QChar c;
  in >> c;

  if (c == 'N') {
    return nullptr;
  } else if (c == 'M') {
    return new MineFactory(in);
  } else if (c == 'T') {
    return new TraitFactory(in);
  } else {
    assert(false);
  }
}


QColor MineFactory::color()
{
  return Qt::GlobalColor(trait);
}



void TraitMine::paint(QPainter *painter) const
{
  painter->save();
  QString file = ":/item/";
  switch (trait) {
  case BLACK:
    file += "BLACK";
    break;
  case BLUE:
    file += "BLUE";
    break;
  case RED:
    file += "RED";
    break;
  }

  qDebug() << file << trait;

  QImage image(":/item/BLUE");
  if (image.isNull()) {
    Item::paint(painter);
  } else {
    painter->drawImage(QRectF(-R, -R, 2*R, 2*R), image);
  }
  painter->restore();
}

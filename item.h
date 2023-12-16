#ifndef ITEM_H
#define ITEM_H

#include "config.h"
#include "util.h"
#include <QtWidgets>

enum trait_t { BLACK = Qt::darkGray, RED = Qt::darkRed, BLUE = Qt::darkBlue };
enum type_t { ROUND, SQUARE };
enum shape_t { QUARTER = 1, HALF = 2, FULL = 4 };

class Item
{
public:
  Item();

  virtual void paint(QPainter *painter) const;

  // serialize
  friend QDataStream &operator<<(QDataStream &out, Item *&item);
  friend QDataStream &operator>>(QDataStream &in, Item *&item);
};

class TraitMine: public Item {
public:
  TraitMine(trait_t trait);
  trait_t getTrait() const;

  // serialize
  friend QDataStream &operator<<(QDataStream &out, TraitMine *&tmine);
  friend QDataStream &operator>>(QDataStream &in, TraitMine *&tmine);

private:
  const trait_t trait;

  // Item interface
public:
  void paint(QPainter *painter) const override;
};

class Mine: public Item {
public:
  explicit Mine(type_t type, shape_t shape, rotate_t rotate, trait_t trait);
  explicit Mine(const Mine& o);
  bool operator==(const Mine& o) const;

  const Mine *setTrait(trait_t trait) const;
  const Mine *rotateR() const;
  const Mine *cutUpper() const;
  const Mine *cutLower() const;

  // Item interface
  void paint(QPainter *painter) const;

  // serialize
  friend QDataStream &operator<<(QDataStream &out, Mine *&mine);
  friend QDataStream &operator>>(QDataStream &in, Mine *&mine);

private:
  const type_t type;
  const shape_t shape;
  const rotate_t rotate;
  const trait_t trait;
};

class ItemFactory {
public:
  ItemFactory();
  virtual ~ItemFactory();
  virtual Item *createItem() const = 0;
  virtual QColor color() = 0;

  // serialize
  explicit ItemFactory(QDataStream &in);
  virtual void save(QDataStream &out);
};

class MineFactory: public ItemFactory {
public:
  explicit MineFactory(type_t type, trait_t trait);

  // serialize
  explicit MineFactory(QDataStream &in);
  virtual void save(QDataStream &out) override;
  QColor color() override;

  // ItemFactory interface
public:
  Mine *createItem() const override;
private:
  type_t type;
  trait_t trait;
};

class TraitFactory: public ItemFactory {
public:
  explicit TraitFactory(trait_t trait);

  // serialize
  explicit TraitFactory(QDataStream &in);
  virtual void save(QDataStream &out) override;
  QColor color() override;

public:
  TraitMine *createItem() const override;
private:
  trait_t trait;
};

ItemFactory *randomItemFactory();

// serialize
void saveItemFactory(QDataStream &out, ItemFactory *f);
ItemFactory *loadItemFactory(QDataStream &in);

#endif // ITEM_H

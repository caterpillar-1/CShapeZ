#ifndef ITEM_H
#define ITEM_H

#include "config.h"
#include "util.h"
#include <QtWidgets>

enum trait_t { BLACK = Qt::darkGray, RED = Qt::red, BLUE = Qt::blue };
enum type_t { ROUND, SQUARE };
enum shape_t { QUARTER = 1, HALF = 2, FULL = 4 };

class Item
{
public:
  Item();

  virtual void paint(QPainter *painter) const;
};

class TraitMine: public Item {
public:
  TraitMine(trait_t trait);
  trait_t getTrait() const;

private:
  const trait_t trait;
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

private:
  const type_t type;
  const shape_t shape;
  const rotate_t rotate;
  const trait_t trait;
};

class ItemFactory {
public:
  virtual Item *createItem() const = 0;
  virtual ~ItemFactory();
};

class MineFactory: public ItemFactory {
public:
  explicit MineFactory(type_t type, trait_t trait);

  // ItemFactory interface
public:
  Mine *createItem() const override;
private:
  const type_t type;
  const trait_t trait;
};

class TraitFactory: public ItemFactory {
public:
  explicit TraitFactory(trait_t trait);

public:
  TraitMine *createItem() const override;
private:
  const trait_t trait;
};
#endif // ITEM_H

#ifndef ITEM_H
#define ITEM_H

#include <QtWidgets>
#include "util.h"

enum type_t { ROUND, SQUARE };
enum shape_t { QUARTER = 1, HALF = 2, FULL = 4 };

enum trait_t { BLACK = Qt::darkGray, RED = Qt::red, BLUE = Qt::blue };

class Item : public QObject, public QGraphicsItem
{
  Q_OBJECT
public:

  Item();

private:

  // QGraphicsItem interface
public:
  void advance(int phase) override;
};

class TraitMine: public Item {
  Q_OBJECT
private:
  enum trait_t trait_;

public:
  TraitMine(enum trait_t trait_);

  enum trait_t getTrait();

  // QGraphicsItem interface
public:
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class Mine: public Item {
  Q_OBJECT
private:
  enum type_t type_;
  enum shape_t shape_;
  enum rotate_t rotate_;
  enum trait_t trait_;

public:
  Mine(enum type_t type_, enum shape_t shape_, enum rotate_t rotate_, enum trait_t trait_);
  Mine(const Mine& o);
  bool operator==(const Mine& o);

  Mine *getUpperHalf();
  Mine *getLowerHalf();
  Mine *getLeftHalf();
  Mine *getRightHalf();

  Mine *getRotateLeft();
  Mine *getRotateRight();

  void addTrait(TraitMine& t);

  // QGraphicsItem interface
public:
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class ItemFactory: public QObject {
  Q_OBJECT
public:
  virtual Item *createItem() = 0;
  virtual ~ItemFactory();
};

class MineFactory: public ItemFactory {
public:
  MineFactory(type_t type, trait_t trait);

  // ItemFactory interface
public:
  Mine *createItem() override;
private:
  type_t type;
  trait_t trait;
};

class TraitFactory: public ItemFactory {
public:
  TraitFactory(trait_t trait);

public:
  TraitMine *createItem() override;
private:
  trait_t trait;
};

#endif // ITEM_H

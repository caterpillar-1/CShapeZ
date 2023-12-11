#ifndef DEVICE_H
#define DEVICE_H

#include "port.h"
#include <QtWidgets>

enum device_id_t { MINER, BELT, CUTTER, MIXER, ROTATOR, TRASH, DEV_NONE};

class Device : public QObject, public QGraphicsItem
{
  Q_OBJECT
public:
  explicit Device(int speed, const QList<QPoint>& blocks = QList<QPoint>({QPoint(0, 0)}));
  const QList<QPoint>& blocks() const;
  virtual const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() = 0;

  // QGraphicsItem interface
public:
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  void advance(int phase) override final; // convert timerEvent to next() calls
  void setSpeed(int speed);

protected:
  virtual void next() = 0;

private:
  const QList<QPoint> blocks_;
  int speed; // frames per opeartion
  int frameCount;
};

class DeviceFactory {
public:
  explicit DeviceFactory(int speed);
  virtual Device *createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory = nullptr) = 0;
protected:
  int speed();
  void setSpeed(int speed);

private:
  int speed_;
};

DeviceFactory *getDeviceFactory(device_id_t id);

class Miner: public Device {
  Q_OBJECT
public:
  static constexpr int MINER_SPEED = 120;
  explicit Miner(ItemFactory *factory, int speed = MINER_SPEED);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

protected:
  void next() override;

private:
  ItemFactory *factory;
  OutputPort out;
};

class MinerFactory: public DeviceFactory {
public:
  explicit MinerFactory(int speed = Miner::MINER_SPEED);

  // DeviceFactory interface
  Miner *createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) override;
};

class Belt: public Device {
  Q_OBJECT
public:
  static constexpr int BELT_SPEED = 60;
  explicit Belt(int speed = BELT_SPEED, const QList<QPoint> &blocks = QList<QPoint>({QPoint(0, 0)}));
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

protected:
  void next() override;

private:
  InputPort in;
  OutputPort out;
  std::vector<const Item *> buffer;
};

class BeltFactory: public DeviceFactory {
public:
  explicit BeltFactory(int speed = Belt::BELT_SPEED);

  // DeviceFactory interface
  Belt *createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) override;
};

class Trash: public Device {
  Q_OBJECT
public:
  static constexpr int TRASH_SPEED = 60;
  explicit Trash(int speed = TRASH_SPEED);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

protected:
  void next() override;

private:
  QList<std::pair<InputPort, std::pair<QPoint, rotate_t>>> in = {
    { InputPort(), { QPoint(0, 0), R0 } },
    { InputPort(), { QPoint(0, 0), R90 } },
    { InputPort(), { QPoint(0, 0), R180 } },
    { InputPort(), { QPoint(0, 0), R270 } },
  };
};

class TrashFactory: public DeviceFactory {
public:
  explicit TrashFactory(int speed = Trash::TRASH_SPEED);

  // DeviceFactory interface
  Trash *createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) override;
};

class Center: public Device {
  Q_OBJECT
public:
  static constexpr int CENTER_SPEED = 10;
  explicit Center(int speed = CENTER_SPEED);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

signals:
  void receiveItem(const Item *item);

protected:
  void next() override;

private:
  QList<std::pair<InputPort, std::pair<QPoint, rotate_t>>> in = {
    { InputPort(), { QPoint(0, 0), R90} },
    { InputPort(), { QPoint(1, 0), R90} },
    { InputPort(), { QPoint(2, 0), R90} },
    { InputPort(), { QPoint(3, 0), R90} },
    { InputPort(), { QPoint(0, 0), R180} },
    { InputPort(), { QPoint(0, 1), R180} },
    { InputPort(), { QPoint(0, 2), R180} },
    { InputPort(), { QPoint(0, 3), R180} },
    { InputPort(), { QPoint(3, 0), R0} },
    { InputPort(), { QPoint(3, 1), R0} },
    { InputPort(), { QPoint(3, 2), R0} },
    { InputPort(), { QPoint(3, 3), R0} },
    { InputPort(), { QPoint(0, 3), R270} },
    { InputPort(), { QPoint(1, 3), R270} },
    { InputPort(), { QPoint(2, 3), R270} },
    { InputPort(), { QPoint(3, 3), R270} },
  };
};

#endif // DEVICE_H

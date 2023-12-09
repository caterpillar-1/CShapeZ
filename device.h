#ifndef DEVICE_H
#define DEVICE_H

#include "item.h"
#include "port.h"
#include "gamestate.h"
#include <QtWidgets>


class Device : public QObject, public QGraphicsItem
{
  Q_OBJECT
public:
  explicit Device(GameState& game, const QPoint& base, rotate_t rotate, const QList<QPoint>& blocks);
  explicit Device(GameState& game, const QPoint& base, rotate_t rotate);
  virtual ~Device();
  void install();
  void remove();
  virtual void connectPorts() = 0;
  virtual void disconnectPorts() = 0;

protected:
  const QList<QPoint>& getBlocks();
  const QPoint& getBase();
  const rotate_t getRotate();
  void connectPortAt(Port *port, int i, rotate_t d);
  void disconnectPortAt(Port *port, int i, rotate_t d);
  const QPoint coordinate(int i);
  GameState& game;

  // QGraphicsItem interface
public:
  QRectF boundingRect() const override final;
  QPainterPath shape() const override final;
private:
  void convertToMap(int i, rotate_t d, int &x, int& y, rotate_t &o);
  QList<QPoint> blocks;
  QPoint base_;
  rotate_t rotate_;
};

class Miner : public Device {
  Q_OBJECT
public:
  explicit Miner(GameState &game, const QPoint &base, rotate_t rotate);
  // speed: produce one item per SPEED phase
  static constexpr int MINER_SPEED = 120;

private:
  OutputPort out;
  int phaseCount;

  // QGraphicsItem interface
public:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  // Device interface
public:
  void connectPorts() override;
  void disconnectPorts() override;

  // QGraphicsItem interface
public:
  void advance(int phase) override;
};

class Belt: public Device {
  Q_OBJECT
private:
  InputPort in;
  OutputPort out;

  std::vector<Item *> buffer;
  int phaseCount;

public:
  explicit Belt(GameState &game, QPoint base, rotate_t rotate, const QList<QPoint>& blocks);
  // speed: move one block per SPEED phase
  static constexpr int BELT_SPEED = 60;

  // QGraphicsItem interface
public:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

  // Device interface
public:
  void connectPorts() override;
  void disconnectPorts() override;

  // QGraphicsItem interface
public:
  void advance(int phase) override;
};

class DeviceFactory: public QObject {
  Q_OBJECT
public:
  explicit DeviceFactory(GameState &game);
  virtual Device *createDevice(QPoint base, rotate_t rotate, const QList<QPoint> &blocks) = 0;
  virtual ~DeviceFactory();
protected:
  GameState &getGame();
private:
  GameState &game_;
};

class MinerFactory: public DeviceFactory {
public:
  explicit MinerFactory(GameState &game);
  // DeviceFactory interface
public:
  Miner *createDevice(QPoint base, rotate_t rotate, const QList<QPoint> &blocks) override;
};

class BeltFactory: public DeviceFactory {
public:
  explicit BeltFactory(GameState &game);
public:
  Belt *createDevice(QPoint base, rotate_t rotate, const QList<QPoint> &blocks) override;
};

#endif // DEVICE_H

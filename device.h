#ifndef DEVICE_H
#define DEVICE_H

#include "port.h"
#include <QtWidgets>

enum device_id_t { MINER, BELT, CUTTER, MIXER, ROTATOR, TRASH, DEV_NONE };

class Device : public QObject, public QGraphicsItem {
  Q_OBJECT
public:
  explicit Device(const QList<QPoint> &blocks = QList<QPoint>({QPoint(0, 0)}));
  const QList<QPoint> &blocks() const;
  virtual const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>>
  ports() = 0;

  // QGraphicsItem interface
public:
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  void advance(int phase) override final; // convert timerEvent to next() calls

  // serialize
  explicit Device(QDataStream &in);
  virtual void save(QDataStream &out); // all devices must save Device first
                                       // (call this function)

protected:
  // timing
  // next() will be called speed() * ratio() times in one sec
  virtual void next() = 0;
  virtual qreal speed() = 0;
  virtual qreal ratio() = 0;

private:
  QList<QPoint> blocks_;
  int frameCount;
};

class DeviceFactory {
public:
  explicit DeviceFactory();
  virtual Device *createDevice(const QList<QPoint> &blocks,
                               const QList<PortHint> &hints,
                               ItemFactory *itemFactory = nullptr) = 0;
};

DeviceFactory *getDeviceFactory(device_id_t id);

class Miner : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Miner(ItemFactory *factory);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  // serialize
  explicit Miner(QDataStream &in);
  void save(QDataStream &out) override;
  void restore(ItemFactory *f);
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  static constexpr qreal MINER_SPEED = 0.5; // 0.5 items / sec
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;

private:
  ItemFactory *factory;
  OutputPort out;
};

class MinerFactory : public DeviceFactory {
public:
  explicit MinerFactory();

  // DeviceFactory interface
  Miner *createDevice(const QList<QPoint> &blocks, const QList<PortHint> &hints,
                      ItemFactory *itemFactory) override;
};

class Belt : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Belt(const QList<QPoint> &blocks, rotate_t inDirection,
                rotate_t outDirection);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  enum turn_t { PASS_THROUGH, TURN_LEFT, TURN_RIGHT };

  // serialize
  explicit Belt(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  // Belt has unique refreshing logic, so its BELT_SPEED means BELT_SPEED *
  // ratio() * 0.1 blocks on the belt per sec
  static constexpr qreal BELT_SPEED = 15; // beginning at 1.5 blocks per second
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;

private:
  InputPort in;
  OutputPort out;

  rotate_t inDirection, outDirection;
  std::vector<rotate_t> direction;
  std::vector<turn_t> turn;
  int length;
  std::vector<const Item *> buffer;
};

class BeltFactory : public DeviceFactory {
public:
  explicit BeltFactory();

  // DeviceFactory interface
  Belt *createDevice(const QList<QPoint> &blocks, const QList<PortHint> &hints,
                     ItemFactory *itemFactory) override;
};

class Cutter : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Cutter();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  // serialize
  explicit Cutter(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  static constexpr qreal CUTTER_SPEED = 0.25;
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;

private:
  InputPort in;
  OutputPort outU, outL;

  bool stall;
};

class CutterFactory : public DeviceFactory {
public:
  explicit CutterFactory();

  // DeviceFactory interface
  Cutter *createDevice(const QList<QPoint> &blocks,
                       const QList<PortHint> &hints,
                       ItemFactory *itemFactory) override;
};

class Rotator : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Rotator();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  // serialize
  explicit Rotator(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  static constexpr qreal ROTATOR_SPEED = 0.65;
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;
public:
  static void setRatio(qreal ratio);

private:
  InputPort in;
  OutputPort out;
};

class RotatorFactory : public DeviceFactory {
public:
  explicit RotatorFactory();

  // DeviceFactory interface
  Rotator *createDevice(const QList<QPoint> &blocks,
                        const QList<PortHint> &hints,
                        ItemFactory *itemFactory) override;
};

class Mixer : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Mixer();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  // serialize
  explicit Mixer(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  static constexpr qreal MIXER_SPEED = 0.25;
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;
public:
  static void setRatio(qreal ratio);

private:
  InputPort inMine, inTrait;
  OutputPort out;
  bool stall;
};

class MixerFactory : public DeviceFactory {
public:
  explicit MixerFactory();

  // DeviceFactory interface
  Mixer *createDevice(const QList<QPoint> &blocks, const QList<PortHint> &hints,
                      ItemFactory *itemFactory) override;
};

class Trash : public Device {
  Q_OBJECT
  friend void setDeviceRatio(device_id_t id, qreal ratio);
  friend void resetDeviceRatio();
public:
  explicit Trash();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;

  // serialize
  explicit Trash(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

protected:
  // timing
  static constexpr qreal TRASH_SPEED = 1;
  static qreal ratio_;
  void next() override;
  qreal speed() override;
  qreal ratio() override;

private:
  QList<std::pair<InputPort, std::pair<QPoint, rotate_t>>> in = {
      {InputPort(), {QPoint(0, 0), R0}},
      {InputPort(), {QPoint(0, 0), R90}},
      {InputPort(), {QPoint(0, 0), R180}},
      {InputPort(), {QPoint(0, 0), R270}},
  };
};

class TrashFactory : public DeviceFactory {
public:
  explicit TrashFactory();

  // DeviceFactory interface
  Trash *createDevice(const QList<QPoint> &blocks, const QList<PortHint> &hints,
                      ItemFactory *itemFactory) override;
};

class Center : public Device {
  Q_OBJECT
public:
  explicit Center(int size);
  ~Center();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ports() override;
  // serialize
  explicit Center(QDataStream &in);
  void save(QDataStream &out) override;
  friend void saveDeviceRatio(QDataStream &out);
  friend void loadDeviceRatio(QDataStream &in);

signals:
  void receiveItem(const Item *item);

public slots:
  void updateGoal(int problemSet, int task, int received, int required,
                  const QPicture *icon);

protected:
  // timing
  static constexpr qreal CENTER_SPEED = 10;
  static constexpr qreal ratio_ = 1;
  void next() override;
  qreal speed() override;
  qreal ratio() override;

private:
  int size;
  int problemSet, task, received, required;
  const QPicture *icon;
  QList<std::pair<InputPort *, std::pair<QPoint, rotate_t>>> in;
};

// serialize
void saveDevice(QDataStream &out, Device *dev);
Device *loadDevice(QDataStream &in);
void restoreDevice(Device *dev, ItemFactory *itemFactory);
void saveDeviceRatio(QDataStream &out);
void loadDeviceRatio(QDataStream &in);
// timing
void resetDeviceRatio();
void setDeviceRatio(device_id_t id, qreal ratio);

void saveCenter(QDataStream &out, Center *center);
Center *loadCenter(QDataStream &in);

#endif // DEVICE_H

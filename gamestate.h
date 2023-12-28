#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "device.h"
#include "item.h"
#include "goalmanager.h"
#include "shop.h"
#include <QtWidgets>
#include <map>

class Selector : public QObject, public QGraphicsItem {
  Q_OBJECT
public:
  explicit Selector(QObject *parent = nullptr);
  void clear();
  void move(rotate_t d);
  QList<QPoint> path();

  // QGraphicsItem interface
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  int x, y;
  QList<QPoint> path_;
};

struct DeviceDescription {
  const QPoint p;
  const rotate_t r;

  DeviceDescription(QPoint point, rotate_t rotate);
  DeviceDescription(int x, int y, rotate_t rotate);
};

class GameState;

class Scene : public QGraphicsScene {
  Q_OBJECT
public:
  explicit Scene(int w, int h, GameState &game, QObject *parent = nullptr);

  // QGraphicsScene interface
protected:
  void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
  const int w, h;
  GameState &game;

  // QGraphicsScene interface
protected:
  void drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                 const QStyleOptionGraphicsItem options[],
                 QWidget *widget) override;
};

class GameState : public QWidget {
  Q_OBJECT
public:
  explicit GameState(int w, int h, Scene *&scene, GoalManager *&goal,
                     QMainWindow *parent = nullptr);
  void pause(bool paused);

  // serialize
  explicit GameState(QDataStream &in, Scene *&scene, GoalManager *&goal,
                     QMainWindow *parent = nullptr);
  void init();
  void save(QDataStream &out);

  // QObject interface
protected:
  void timerEvent(QTimerEvent *) override;
  bool eventFilter(QObject *object, QEvent *event) override;
public slots:
  void keyPressEvent(QKeyEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;
  void enhanceChange();
  void moneyChange(int delta);
  void shopOpenEvent();
  void mapConstructEvent();

signals:
  void deviceChangeEvent(device_id_t id);
  void deviceRatioChangeEvent(device_id_t id, qreal ratio);
  void saveEvent();
  void enhanceChangeEvent(int enhance);
  void moneyChangeEvent(int money);
  void zoomIn();
  void zoomOut();
  void zoomReset();

private:
  // interfaces for self
  bool installDevice(QPoint base, rotate_t rotate, Device *device);
  void removeDevice(Device *device);
  void removeDevice(int x, int y);
  void removeDevice(QPoint p);
  void changeDevice(device_id_t id);

private: // helper functions
  bool inRange(int x, int y);
  bool inRange(QPoint p);
  QPoint mapToMap(QPoint p, QPoint base, rotate_t rotate);
  ItemFactory *&groundMap(int x, int y);
  ItemFactory *&groundMap(QPoint p);
  Device *&deviceMap(int x, int y);
  Device *&deviceMap(QPoint p);
  Port *&portMap(int x, int y, rotate_t r);
  Port *&portMap(QPoint p, rotate_t r);
  Port *otherPort(QPoint p, rotate_t r);
  void moveSelector(rotate_t d);
  void shiftSelector(rotate_t d);
  QList<PortHint> getPortHint(QPoint base, rotate_t rotate,
                              const QList<QPoint> &blocks);
  void naiveInitMap(int w, int h);
  void loadMap(QDataStream &in);
  bool enhanceDevice(device_id_t id);

private: // states
  int w, h;
  /* GUI elements */
  // window
  QMainWindow *window;
  // scene
  friend Scene;
  Scene *scene;
  // selector
  Selector *selector;
  QPoint base, offset;
  rotate_t rotate;
  // selector FSM
  bool selectorState;

  /* mapping */
  std::vector<std::vector<ItemFactory *>> groundMap_;
  std::vector<std::vector<Device *>> deviceMap_;
  std::vector<std::vector<std::array<Port *, 4>>> portMap_;

  std::map<Device *, DeviceDescription> devices;

  /* game control */
  int timerId;
  bool pause_;
  device_id_t deviceId;
  DeviceFactory *deviceFactory;

  /* game stage */
  Center *center;
  GoalManager *goalManager;
  int money;
  int enhance;
  qreal moneyRatio;
  qreal itemRatio;
  int nextW, nextH;
};

#endif // GAMESTATE_H

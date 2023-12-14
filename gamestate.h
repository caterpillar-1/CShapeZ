#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "device.h"
#include "item.h"
#include <map>
#include <QtWidgets>

class Selector: public QObject, public QGraphicsItem {
  Q_OBJECT
public:
  explicit Selector(QObject *parent = nullptr);
  void clear();
  void move(rotate_t d);
  QList<QPoint> path();

  // QGraphicsItem interface
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

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

class GameState : public QObject
{
  Q_OBJECT
public:
  explicit GameState(QGraphicsScene *scene, QMainWindow *parent = nullptr);
  void pause(bool paused);

  // QObject interface
protected:
  void timerEvent(QTimerEvent *) override;
  bool eventFilter(QObject *object, QEvent *event) override;
public slots:
  void keyPressEvent(QKeyEvent *e);
  void keyReleaseEvent(QKeyEvent *e);


signals:
  void deviceChangeEvent(device_id_t id);

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
  QList<PortHint> getPortHint(QPoint base, rotate_t rotate, const QList<QPoint> &blocks);

private: // states
  /* GUI elements */
  // window
  QMainWindow *window;
  // scene
  QGraphicsScene *scene;
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
  bool pause_;
  DeviceFactory *deviceFactory;

  /* game stage */

};

#endif // GAMESTATE_H

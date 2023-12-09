#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QtWidgets>
#include "config.h"
#include "util.h"

class Item;
class ItemFactory;

class Device;
class DeviceFactory;
class Port;

class GameState : public QObject {
  Q_OBJECT
public:
  explicit GameState(QGraphicsScene *scene, QObject *parent = nullptr);
  void loadMap();

public slots:
  void pause();
  void selectMachine(int id);
  void shiftSelectedTile(int x, int y);

  // map functions
  Item *getItem(int x, int y);
  Device *getDevice(int x, int y);
  Port *getPort(int x, int y, rotate_t d);
  Port *getOtherPort(int x, int y, rotate_t d);
  void setDevice(int x, int y, Device* device);
  void setPort(int x, int y, rotate_t d, Port *port);

private:
  QGraphicsScene *scene;
  bool stall;
  int selectedMachine;
  int selectedTileX, selectedTileY;
  QGraphicsItem *selectedTile;
  std::vector<std::vector<ItemFactory *>> groundMap;
  std::vector<std::vector<Device *>> deviceMap;
  std::vector<std::vector<std::array<Port *, 4>>> portMap;

  void handleKeyPressed(QKeyEvent *event);
  bool eventFilter(QObject *obj, QEvent *event) override;

  // helper functions
  bool inRange(int x, int y);
  int timerId;

signals:

  // QObject interface
protected:
  void timerEvent(QTimerEvent *event) override;
};

#endif // GAMESTATE_H

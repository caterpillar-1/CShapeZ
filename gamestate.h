#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QtWidgets>

class Item;
class ItemFactory;

class GameState : public QObject {
  Q_OBJECT
public:
  explicit GameState(QGraphicsScene *scene, QObject *parent = nullptr);
  void loadMap();

public slots:
  void pause();
  void resume();
  void selectMachine(int id);
  void shiftSelectedTile(int x, int y);

private:
  QGraphicsScene *scene;
  bool stall;
  int selectedMachine;
  int selectedTileX, selectedTileY;
  QGraphicsItem *selectedTile;
  QList<QList<ItemFactory *>> map;

  void handleKeyPressed(QKeyEvent *event);
  bool eventFilter(QObject *obj, QEvent *event) override;

signals:
};

#endif // GAMESTATE_H

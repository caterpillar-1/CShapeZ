#include "gamestate.h"
#include "config.h"

GameState::GameState(QGraphicsScene *scene, QObject *parent)
    : QObject{parent}, scene(scene), stall(true),
      selectedMachine(0), selectedTileX(0), selectedTileY(0)
//      selectedMachine(MACHINE_MINER), selectedTileX(0), selectedTileY(0) {
{
  loadMap();
  scene->installEventFilter(this);
  selectedTile = new QGraphicsRectItem(0, 0, TILE_W, TILE_H);
  scene->addItem(selectedTile);
}

void GameState::loadMap() {
//  for (int y = 0; y < TILES_Y; y++) {
//    QList<ItemFactory *> col;
//    for (int x = 0; x < TILES_X; x++) {
//      if (3 < x && x < 7 && 2 < y && y < 5) {
//        col.append(new ItemFactory(ITEM_ROUND));
//      } else if (8 < x && x < 11 && 6 < y && y < 15) {
//        col.append(new ItemFactory(ITEM_RECT));
//      } else {
//        col.append(new ItemFactory(ITEM_VOID));
//      }
//    }
//    map.append(col);
//  }
}

void GameState::pause() { stall = true; }

void GameState::resume() { stall = false; }

void GameState::selectMachine(int id) {
  qDebug() << "Select Machine:" << id;
  selectedMachine = id;
}

void GameState::shiftSelectedTile(int x, int y) {
  int nx = selectedTileX + x, ny = selectedTileY + y;
  nx = std::min(std::max(nx, 0), TILES_X - 1);
  ny = std::min(std::max(ny, 0), TILES_Y - 1);
  selectedTileX = nx;
  selectedTileY = ny;
}

void GameState::handleKeyPressed(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Left:
  case Qt::Key_H:
    shiftSelectedTile(-1, 0);
    break;
  case Qt::Key_Right:
  case Qt::Key_L:
    shiftSelectedTile(1, 0);
    break;
  case Qt::Key_J:
  case Qt::Key_Up:
    shiftSelectedTile(0, -1);
    break;
  case Qt::Key_K:
  case Qt::Key_Down:
    shiftSelectedTile(0, 1);
    break;
  }
}

bool GameState::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    handleKeyPressed(dynamic_cast<QKeyEvent *>(event));
    return true;
  } else {
    return QObject::eventFilter(obj, event);
  }
}

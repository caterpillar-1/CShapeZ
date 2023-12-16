#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "gamestate.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void addMenus();
  void addToolBars();

public slots:
  void deviceChangeEvent(device_id_t id);

private:
  GameState *game;

  Scene *scene;
  QGraphicsView *view;
  QList<QAction *> deviceButtons;


};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include "gamestate.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  QGraphicsScene *scene;
  QGraphicsView *view;

  GameState *game;

  QList<QAction *> machines;

private slots:
  void handleMachineChange(bool checked);
};
#endif // MAINWINDOW_H

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "mainwindow.h"
#include "gamestate.h"
#include <QtWidgets>

class Launcher : public QWidget
{
  Q_OBJECT
public:
  explicit Launcher(QWidget *parent = nullptr);

public slots:
  void constrainSize();
  void startGame();
  void openSaveSlot();
  void newSaveSlot();
  void exit();

signals:

private:
  bool newGame;
  QString slotname;

};

#endif // LAUNCHER_H

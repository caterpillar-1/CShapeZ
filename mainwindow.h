#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "gamestate.h"
#include "goalmanager.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(bool newGame, QString filename, QWidget *parent = nullptr);
  ~MainWindow();

signals:

public slots:
  void updateGoal(int problemSet, int task, int received, int required, const QPicture *icon);
  void deviceChangeEvent(device_id_t id);
  void deviceRatioUpdateEvent(device_id_t id, qreal ratio);
  void saveEvent();
  void enhanceChangeEvent(int enhance);
  void moneyChangeEvent(int money);
  void zoomIn();
  void zoomOut();
  void zoomReset();

private:
  void addDeviceButtons(QHBoxLayout *l);
  void addDeviceRatios(QVBoxLayout *r);

private:
  QString filename;
  GameState *game;

  Scene *scene;
  GoalManager *goal;
  QGraphicsView *view;
  QList<QPushButton *> deviceButtons;
  QList<QLabel *> deviceRatioLabels;
  QLabel *moneyLabel, *enhanceLa;

  QLabel *problemLabel, *taskLabel, *itemLabel, *enhanceLabel;
};

#endif // MAINWINDOW_H

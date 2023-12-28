#include "mainwindow.h"

MainWindow::MainWindow(bool newGame, QString filename, QWidget *parent)
    : QMainWindow(parent), filename(filename) {
  problemLabel = new QLabel("ProblemSet 0", this);
  taskLabel = new QLabel("Task 0 / 0", this);
  itemLabel = new QLabel("0 / 0", this);
  enhanceLabel = new QLabel("Local Enhance: 0", this);
  moneyLabel = new QLabel("Money: 0", this);

  if (newGame) {
    game = new GameState(32, 24, scene, goal, this);
  } else {
    QFile saveslot(filename);
    saveslot.open(QIODevice::ReadOnly);
    QDataStream savestream(&saveslot);
    game = new GameState(savestream, scene, goal, this);
    saveslot.close();
  }
  connect(game, &GameState::saveEvent, this, &MainWindow::saveEvent);
  connect(goal, &GoalManager::updateGoal, this, &MainWindow::updateGoal);

  view = new QGraphicsView(scene, this);

  QVBoxLayout *left = new QVBoxLayout, *right = new QVBoxLayout;
  QHBoxLayout *dButtons = new QHBoxLayout(this);
  QWidget *buttonGroupWidget = new QWidget(this);
  buttonGroupWidget->setLayout(dButtons);

  addDeviceButtons(dButtons);

  left->addWidget(view);
  left->addWidget(buttonGroupWidget);

  right->addWidget(problemLabel);
  right->addWidget(taskLabel);
  right->addWidget(itemLabel);

  right->addWidget(enhanceLabel);

  addDeviceRatios(right);

  QWidget *rightShopWidget = new QWidget(this);
  QHBoxLayout *shopLayout = new QHBoxLayout(rightShopWidget);
  shopLayout->addWidget(moneyLabel);
  right->addWidget(rightShopWidget);

  QHBoxLayout *layout = new QHBoxLayout;
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(left);
  layout->addLayout(right);

  QWidget *central = new QWidget(this);
  central->setLayout(layout);
  setCentralWidget(central);

  connect(game, &GameState::deviceChangeEvent, this,
          &MainWindow::deviceChangeEvent);
  connect(game, &GameState::deviceRatioChangeEvent, this, &MainWindow::deviceRatioUpdateEvent);
  connect(game, &GameState::enhanceChangeEvent, this, &MainWindow::enhanceChangeEvent);
  connect(game, &GameState::moneyChangeEvent, this, &MainWindow::moneyChangeEvent);
  connect(this, &MainWindow::keyPressEvent, game, &GameState::keyPressEvent);
  connect(this, &MainWindow::keyReleaseEvent, game,
          &GameState::keyReleaseEvent);
  connect(game, &GameState::zoomIn, this, &MainWindow::zoomIn);
  connect(game, &GameState::zoomOut, this, &MainWindow::zoomOut);
  connect(game, &GameState::zoomReset, this, &MainWindow::zoomReset);

  game->init();
}

MainWindow::~MainWindow() {}

void MainWindow::updateGoal(int problemSet, int task, int received, int required, const QPicture *icon)
{
  using std::to_string;
  problemLabel->setText(("ProblemSet " + to_string(problemSet)).c_str());
  taskLabel->setText(("Task " + to_string(task)).c_str());
  itemLabel->setPicture(*icon);
  itemLabel->setText((to_string(received) + " / " + to_string(required)).c_str());
}

void MainWindow::deviceChangeEvent(device_id_t id) {
  if (id == DEV_NONE) {
    for (auto x: deviceButtons) {
      x->setChecked(false);
    }
  } else {
    for (int i = 0; i < 6; i ++) {
      if (i == id) {
        deviceButtons[i]->setChecked(true);
      } else {
        deviceButtons[i]->setChecked(false);
      }
    }
  }
}

void MainWindow::deviceRatioUpdateEvent(device_id_t id, qreal ratio)
{
  using std::to_string;
  assert((int)id < (int)DEV_NONE);
  deviceRatioLabels[id]->setText(QString::number(ratio));
}

void MainWindow::saveEvent() {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(this, "Invalid saveslot",
                          "The passed in saveslot name is invalid, please "
                          "choose or create a new one and then save again.");
    filename = QFileDialog::getSaveFileName(this, "New Save File");
  }
  QDataStream out(&file);
  game->save(out);
}

void MainWindow::enhanceChangeEvent(int enhance)
{
  this->enhanceLabel->setText(("Local Enhancement: " + std::to_string(enhance)).c_str());
}

void MainWindow::moneyChangeEvent(int money)
{
  using std::to_string;
  this->moneyLabel->setText(("Money: " + to_string(money)).c_str());
}

void MainWindow::zoomIn()
{
  view->scale(1.2, 1.2);
}

void MainWindow::zoomOut()
{
  view->scale(1/1.2, 1/1.2);
}

void MainWindow::zoomReset()
{
  view->resetTransform();
}

void MainWindow::addDeviceButtons(QHBoxLayout *dButtons)
{
  QIcon minerIcon(":/device_icon/miner");
  QIcon beltIcon(":/device_icon/belt");
  QIcon cutterIcon(":/device_icon/cutter");
  QIcon mixerIcon(":/device_icon/mixer");
  QIcon rotatorIcon(":/device_icon/rotater");
  QIcon trashIcon(":/device_icon/trash");
  QPushButton *miner = new QPushButton(minerIcon, "Miner");
  QPushButton *belt = new QPushButton(beltIcon, "belt");
  QPushButton *cutter = new QPushButton(cutterIcon, "Cutter");
  QPushButton *mixer = new QPushButton(mixerIcon, "Mixer");
  QPushButton *rotator = new QPushButton(rotatorIcon, "Rotator");
  QPushButton *trash = new QPushButton(trashIcon, "Trash");

  auto list = {miner, belt, cutter, mixer, rotator, trash};
  for (auto x : list) {
    x->setCheckable(true);
    dButtons->addWidget(x, 0, Qt::AlignLeft);
    deviceButtons.push_back(x);
  }
}

void MainWindow::addDeviceRatios(QVBoxLayout *r)
{
  for (int i = 0; i < DEV_NONE; i ++) {
    QWidget *widget = new QWidget();
    QLayout *layout = new QHBoxLayout(widget);
    QLabel *name = new QLabel(), *ratio = new QLabel();
    name->setText(getDeviceName(device_id_t(i)));
    layout->addWidget(name);
    layout->addWidget(ratio);
    deviceRatioLabels.push_back(ratio);
    r->addWidget(widget);
  }
}

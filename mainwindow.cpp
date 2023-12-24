#include "mainwindow.h"

MainWindow::MainWindow(bool newGame, QString filename, QWidget *parent)
    : QMainWindow(parent), filename(filename) {
  if (newGame) {
    game = new GameState(25, 18, scene, this);
  } else {
    QFile saveslot(filename);
    saveslot.open(QIODevice::ReadOnly);
    QDataStream savestream(&saveslot);
    game = new GameState(savestream, scene, this);
    saveslot.close();
  }
  connect(game, &GameState::saveEvent, this, &MainWindow::saveEvent);

  view = new QGraphicsView(scene, this);

  QVBoxLayout *left = new QVBoxLayout, *right = new QVBoxLayout;
  QHBoxLayout *dButtons = new QHBoxLayout(this);
  QWidget *buttonGroupWidget = new QWidget(this);
  buttonGroupWidget->setLayout(dButtons);

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
  int cnt = 1;
  for (auto x : list) {
    x->setCheckable(true);
    dButtons->addWidget(x, 0, Qt::AlignLeft);
    deviceButtons.push_back(x);
  }


  left->addWidget(view);
  left->addWidget(buttonGroupWidget);


  QLabel *problemLabel = new QLabel("ProblemSet 0");
  QLabel *taskLabel = new QLabel("Task 0 / 0");
  QLabel *itemLabel = new QLabel("0 / 0");
  right->addWidget(problemLabel);
  right->addWidget(taskLabel);
  right->addWidget(itemLabel);

  QLabel *remainLocal = new QLabel("Local Improvement Remain: ");
  right->addWidget(remainLocal);


  QWidget *rightShopWidget = new QWidget(this);
  QHBoxLayout *shopLayout = new QHBoxLayout(this);

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
  connect(this, &MainWindow::keyPressEvent, game, &GameState::keyPressEvent);
  connect(this, &MainWindow::keyReleaseEvent, game,
          &GameState::keyReleaseEvent);

//  addMenus();
//  addToolBars();
}

MainWindow::~MainWindow() {}

void MainWindow::addMenus() {
  QMenu *fileMenu = menuBar()->addMenu("&File");

  {
    QAction *newGameAction = fileMenu->addAction("&New Game");
    //    newGameAction->setShortcut(Qt::CTRL | Qt::Key_N);
    QAction *loadAction = fileMenu->addAction("&Load");
    //    loadAction->setShortcut(Qt::CTRL | Qt::Key_L);
    QAction *saveAction = fileMenu->addAction("&Save");
    //    saveAction->setShortcut(Qt::CTRL | Qt::Key_S);
    QAction *screenshotAction = fileMenu->addAction("Screenshot");
    QAction *quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(Qt::CTRL | Qt::Key_Q);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
  }

  QMenu *gameMenu = menuBar()->addMenu("&Game");

  {
    QAction *pauseAction = gameMenu->addAction("&Pause");
    pauseAction->setCheckable(true);
    pauseAction->setChecked(true);
    pauseAction->setShortcut(Qt::CTRL | Qt::Key_P);
    connect(pauseAction, &QAction::triggered, game, &GameState::pause);
  }

  QMenu *helpMenu = menuBar()->addMenu("&Help");

  {
    QAction *manualAction = helpMenu->addAction("&Manual");
    QAction *aboutAction = helpMenu->addAction("&About");
  }
}

void MainWindow::addToolBars() {
  QToolBar *deviceBar = addToolBar("Devices");

  QAction *miner = deviceBar->addAction("Miner");
  QAction *belt = deviceBar->addAction("Belt");
  QAction *cutter = deviceBar->addAction("Cutter");
  QAction *mixer = deviceBar->addAction("Mixer");
  QAction *rotator = deviceBar->addAction("Rotator");
  QAction *trash = deviceBar->addAction("Trash");

  auto list = {miner, belt, cutter, mixer, rotator, trash};
  for (auto &x : list) {
    x->setCheckable(true);
  }
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

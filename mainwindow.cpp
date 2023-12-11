#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), scene(new QGraphicsScene(this)),
      view(new QGraphicsView(scene, this)), game(new GameState(scene, this)) {
  setCentralWidget(view);

  connect(game, &GameState::deviceChangeEvent, this, &MainWindow::deviceChangeEvent);
  connect(this, &MainWindow::keyPressEvent, game, &GameState::keyPressEvent);
  connect(this, &MainWindow::keyReleaseEvent, game, &GameState::keyReleaseEvent);

  addMenus();
  addToolBars();
}

MainWindow::~MainWindow() {}

void MainWindow::addMenus() {
  QMenu *fileMenu = menuBar()->addMenu("&File");

  {
    QAction *newGameAction = fileMenu->addAction("&New Game");
    newGameAction->setShortcut(Qt::CTRL | Qt::Key_N);
    QAction *loadAction = fileMenu->addAction("&Load");
    loadAction->setShortcut(Qt::CTRL | Qt::Key_L);
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(Qt::CTRL | Qt::Key_S);
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

  deviceButtons = {miner, belt, cutter, mixer, rotator, trash};
  for (auto &x : deviceButtons) {
    x->setCheckable(true);
  }
}

void MainWindow::deviceChangeEvent(device_id_t id) {
  switch (id) {
  case DEV_NONE:
    for (auto x : deviceButtons) {
      x->setChecked(false);
    }
    break;
  default:
    for (auto x : deviceButtons) {
      x->setChecked(false);
    }
    deviceButtons[id]->setChecked(true);
  }
}

#include "launcher.h"

Launcher::Launcher(QWidget *parent)
  : QWidget{parent}, newGame(true)
{

  QImage image(":/logo/logo");
  QPixmap pixmap = QPixmap::fromImage(image);

  QLabel *backgroundPicture = new QLabel(this);
  backgroundPicture->setPixmap(pixmap);

  QLabel *label = new QLabel("Saveslot:");
  QPushButton *open = new QPushButton("Open");
  connect(open, &QPushButton::clicked, this, &Launcher::openSaveSlot);
  QPushButton *newGame = new QPushButton("New Game");
  connect(newGame, &QPushButton::clicked, this, &Launcher::newSaveSlot);
  QPushButton *start = new QPushButton("Start");
  connect(start, &QPushButton::clicked, this, &Launcher::startGame);

  QHBoxLayout *bar = new QHBoxLayout;
  bar->addWidget(label, 0, Qt::AlignLeft);
  bar->addWidget(open, 0, Qt::AlignLeft);
  bar->addWidget(newGame, 0, Qt::AlignLeft);
  bar->addWidget(start, 0, Qt::AlignRight);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(backgroundPicture);
  layout->addLayout(bar);

  setLayout(layout);

  QTimer::singleShot(0, this, &Launcher::constrainSize);
}

void Launcher::constrainSize()
{
  setFixedSize(size());
}

void Launcher::startGame()
{
  try {
    MainWindow *window = new MainWindow(newGame, slotname);
    connect(window, &MainWindow::destroyed, this, &Launcher::exit);
    window->show();
    hide();
  } catch(std::invalid_argument) {
    QMessageBox::critical(this, "Invalid saveslot", "Please open or create a saveslot.");
  } catch(...) {
    QMessageBox::critical(this, "Unknown Error", "Error staring game.");
  }
}

void Launcher::openSaveSlot()
{
  slotname = QFileDialog::getOpenFileName(this, "Open saveslot");
  QFile saveslot(slotname);
  if (!saveslot.open(QIODevice::ReadOnly)) {
    QMessageBox::information(this, "Invalid saveslot", "Please open a valid saveslot file or create a new one.");
  } else {
    newGame = false;
  }
  saveslot.close();
}

void Launcher::newSaveSlot()
{
  slotname = QFileDialog::getSaveFileName(this, "New saveslot");
  newGame = true;
}

void Launcher::exit()
{
  std::exit(0);
}

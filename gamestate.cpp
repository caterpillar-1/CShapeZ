#include "gamestate.h"

GameState::GameState(int w, int h, Scene *&scene, GoalManager *&goal, QMainWindow *parent)
    : QWidget(parent), window(parent), w(w), h(h), money(0), enhance(0), deviceId(DEV_NONE),
      selector(new Selector(this)), base(QPoint(0, 0)), offset(QPoint(0, 0)),
      rotate(R0), selectorState(false), pause_(false), deviceFactory(nullptr),
      moneyRatio(1), itemRatio(0.2), nextW(w), nextH(h) {
  assert(window);
  assert(selector);
  assert(w >= 8 && h >= 8);

  scene = new Scene(w, h, *this, parent);
  this->scene = scene;

  scene->addItem(selector);
  selector->setZValue(1000);
  selector->setPos(L / 2, L / 2);
  scene->installEventFilter(this);

  naiveInitMap(w, h);

  center = new Center(4);
  QPoint centerBase = {rng.bounded(w - 4), rng.bounded(h - 4)};
  rotate_t centerRotate = R0;
  installDevice(centerBase, centerRotate, center);
  resetDeviceRatio();

  goalManager = new GoalManager();
  goal = goalManager;
}

void GameState::save(QDataStream &out) {
  out << w << h;

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      saveItemFactory(out, groundMap(i, j));
    }
  }

  out << (int)devices.size();
  for (auto &[p, desc] : devices) {
    out << desc.p << desc.r;
    saveDevice(out, p);
  }
  saveDeviceRatio(out);

  goalManager->save(out);
  out << money << enhance;

  out << moneyRatio << itemRatio << nextW << nextH;
}

void GameState::pause(bool paused) { this->pause_ = paused; }

GameState::GameState(QDataStream &in, Scene *&scene, GoalManager *&goal, QMainWindow *parent)
    : QWidget(parent), window(parent), selector(new Selector(this)), base(QPoint(0, 0)), offset(QPoint(0, 0)), deviceId(DEV_NONE),
      rotate(R0), selectorState(false), pause_(false), deviceFactory(nullptr),
      center(nullptr) {
  loadMap(in);
  scene = new Scene(w, h, *this, parent);
  this->scene = scene;

  scene->addItem(selector);
  selector->setZValue(1000);
  selector->setPos(L / 2, L / 2);
  scene->installEventFilter(this);

  int nr_device;
  in >> nr_device;
  // sanity check
  assert(nr_device >= 0);
  assert(devices.empty());

  for (int i = 0; i < nr_device; i++) {
    QPoint base;
    rotate_t rotate;
    in >> base >> rotate;
//    qDebug() << "Loading device at" << base << rotate;
    Device *dev = loadDevice(in);

    devices.insert({dev, {base, rotate}});
  }

  for (auto &[dev, desc] : devices) {
    assert(dev);
    if (Center *c = dynamic_cast<Center *>(dev)) {
      center = c;
    }
    installDevice(desc.p, desc.r, dev);
    restoreDevice(dev, groundMap(desc.p));
  }
  loadDeviceRatio(in);

  assert(center);

  goalManager = new GoalManager(in);
  goal = goalManager;

  in >> money >> enhance;

  in >> moneyRatio >> itemRatio >> nextW >> nextH;
}

void GameState::init()
{
  connect(center, &Center::receiveItem, goalManager, &GoalManager::receiveItem);
  connect(goalManager, &GoalManager::updateGoal, center, &Center::updateGoal);
  connect(goalManager, &GoalManager::enhanceChange, this, &GameState::enhanceChange);
  connect(goalManager, &GoalManager::moneyChange, this, &GameState::moneyChange);
  connect(goalManager, &GoalManager::mapConstructEvent, this, &GameState::mapConstructEvent);

  goalManager->init();

  emit moneyChangeEvent(money);
  emit enhanceChangeEvent(enhance);
  for (int i = 0; i < DEV_NONE; i ++) {
    emit deviceRatioChangeEvent(device_id_t(i), getDeviceRatio(device_id_t(i)));
  }

  timerId = startTimer(1000 / FPS);
}

void GameState::loadMap(QDataStream &in) {
  in >> w >> h;

  groundMap_.resize(w);
  for (auto &col : groundMap_) {
    col.resize(h);
  }

  deviceMap_.resize(w);
  for (auto &col : deviceMap_) {
    col.resize(h);
  }

  portMap_.resize(w);
  for (auto &col : portMap_) {
    col.resize(h);
  }

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      groundMap_[i][j] = loadItemFactory(in);
    }
  }
}

bool GameState::enhanceDevice(device_id_t id)
{
  if (enhance <= 0) {
    return false;
  }
  qreal ratio = getDeviceRatio(id), nr;
  if (abs(ratio - 1) < EPS) {
    nr = 1.5;
  } else if (abs(ratio - 1.5) < EPS) {
    nr = 2;
  } else if (abs(ratio - 2) < EPS) {
    nr = 2.5;
  } else if (abs(ratio - 2.5) < EPS) {
    nr = 3;
  } else {
    return false;
  }
  setDeviceRatio(id, nr);
  enhance --;
  emit enhanceChangeEvent(enhance);
  emit deviceRatioChangeEvent(id, nr);
  return true;
}

bool GameState::installDevice(QPoint base, rotate_t rotate, Device *device) {
  assert(device);
  auto blocks = device->blocks();
  auto portEntries = device->ports();

  // check bound
  for (auto &block : blocks) {
    auto p = mapToMap(block, base, rotate);
    if (!inRange(p)) {
      return false;
    }
    if (dynamic_cast<Center *>(deviceMap(p))) {
      return false;
    }
  }

  // allocating blocks
  for (auto &block : blocks) {
    auto p = mapToMap(block, base, rotate);

    auto &d = deviceMap(p);
    if (d) {
      removeDevice(d);
    }
    d = device;
  }

  // connecting ports
  for (auto &e : portEntries) {
    Port *port = e.first;
    const auto &[block, portRotate] = e.second;

    rotate_t r = rotate_t((rotate + portRotate) % 4);
    auto p = mapToMap(block, base, rotate);

    auto &portSlot = portMap(p, r);
    assert(portSlot == nullptr);
    portSlot = port;

    Port *op = otherPort(p, r);
    if (op) {
      port->connect(op);
      op->connect(port);
    }
  }

  // add to gui
  scene->addItem(device);
  device->setPos(base.x() * L + L / 2, base.y() * L + L / 2);
  device->setRotation(-rotate * 90);

  devices.insert({device, {base, rotate}});
  return true;
}

void GameState::removeDevice(Device *device) {
  assert(devices.find(device) != devices.end());
//  if (dynamic_cast<Center *>(device)) {
//    return; // center is not removable
//  }
  const auto &[base, rotate] = devices.at(device);
  auto blocks = device->blocks();
  auto portEntries = device->ports();

  // disconnecting ports
  for (auto &e : portEntries) {
    Port *port = e.first;
    auto &[block, portRotate] = e.second;

    rotate_t r = rotate_t((rotate + portRotate) % 4);
    auto p = mapToMap(block, base, rotate);

    auto &portSlot = portMap(p, r);
    assert(portSlot);
    portSlot = nullptr;

    Port *op = otherPort(p, r);
    port->disconnect();
    if (op) {
      op->disconnect();
    }
  }

  // freeing blocks
  for (auto &block : blocks) {
    auto p = mapToMap(block, base, rotate);
    assert(inRange(p));

    auto &d = deviceMap(p);
    d = nullptr;
  }

  // remove from gui
  // device is automatically deleted by Qt
  scene->removeItem(device);

  devices.erase(device);
}

void GameState::removeDevice(int x, int y) {
  Device *d = deviceMap(x, y);
  if (dynamic_cast<Center *>(d)) {
    return;
  }
  if (d) {
    removeDevice(d);
  }
}

void GameState::removeDevice(QPoint p) { removeDevice(p.x(), p.y()); }

void GameState::changeDevice(device_id_t id) {
  DeviceFactory *nf = getDeviceFactory(id);
  if (nf == deviceFactory) {
    deviceFactory = nullptr;
    deviceId = DEV_NONE;
    emit deviceChangeEvent(DEV_NONE);
  } else {
    deviceFactory = nf;
    deviceId = id;
    emit deviceChangeEvent(id);
  }
}

bool GameState::inRange(int x, int y) {
  return 0 <= x && x < w && 0 <= y && y < h;
}

bool GameState::inRange(QPoint p) { return inRange(p.x(), p.y()); }

QPoint GameState::mapToMap(QPoint p, QPoint base, rotate_t rotate) {
  int bx = base.x(), by = base.y();
  int x = p.x(), y = p.y();
  switch (rotate) {
  case R0:
    return base + p;
  case R90:
    return QPoint(bx + y, by - x);
  case R180:
    return base - p;
  case R270:
    return QPoint(bx - y, by + x);
  }
  assert(false);
}

ItemFactory *&GameState::groundMap(int x, int y) { return groundMap_[x][y]; }

ItemFactory *&GameState::groundMap(QPoint p) {
  return groundMap_[p.x()][p.y()];
}

Device *&GameState::deviceMap(int x, int y) { return deviceMap_[x][y]; }

Device *&GameState::deviceMap(QPoint p) { return deviceMap_[p.x()][p.y()]; }

Port *&GameState::portMap(int x, int y, rotate_t r) {
  return portMap_[x][y][r];
}

Port *&GameState::portMap(QPoint p, rotate_t r) {
  return portMap_[p.x()][p.y()][r];
}

Port *GameState::otherPort(QPoint p, rotate_t r) {
  int nx = p.x() + dx[r], ny = p.y() + dy[r];
  rotate_t nr = rotate_t((r + 2) % 4);
  if (!inRange(nx, ny)) {
    return nullptr;
  }

  return portMap(nx, ny, nr);
}

void GameState::moveSelector(rotate_t d) {
  QPoint cur_p = base + offset, np = cur_p + QPoint(dx[d], dy[d]);
  if (!inRange(np)) {
    return;
  }
  offset += QPoint(dx[d], dy[d]);
  selector->move(rotate_t((d + 4 - rotate) % 4));
}

void GameState::shiftSelector(rotate_t d) {
  assert(inRange(base));
  int nx = base.x() + dx[d], ny = base.y() + dy[d];
  if (!inRange(nx, ny)) {
    return;
  }

  base = {nx, ny};
  selector->setPos(nx * L + L / 2, ny * L + L / 2);
  selector->ensureVisible();
}

QList<PortHint> GameState::getPortHint(QPoint base, rotate_t rotate,
                                       const QList<QPoint> &blocks) {
  QList<PortHint> hints;
  for (auto p : blocks) {
    std::array<Port *, 4> ports = {};
    // cpp grammar check
    assert(ports[0] == nullptr);
    assert(ports[1] == nullptr);
    assert(ports[2] == nullptr);
    assert(ports[3] == nullptr);

    QPoint realp = mapToMap(p, base, rotate);

    for (int d = 0; d < 4; d++) {
      // out-of-shape check
      QPoint np = p + QPoint(dx[d], dy[d]);
      if (blocks.contains(np))
        continue;

      rotate_t r = rotate_t((rotate + d) % 4);
      ports[d] = otherPort(realp, r);
    }

    hints.push_back(PortHint(ports));
  }

  return hints;
}

void GameState::naiveInitMap(int w, int h) {
  this->w = w;
  this->h = h;

  for (auto &col: groundMap_) {
    for (auto &block: col) {
      delete block;
    }
  }
  groundMap_.resize(0);
  groundMap_.resize(w);
  for (auto &col : groundMap_) {
    col.resize(h);
  }

  deviceMap_.resize(0);
  deviceMap_.resize(w);
  for (auto &col : deviceMap_) {
    col.resize(h, nullptr);
  }

  portMap_.resize(0);
  portMap_.resize(w);
  for (auto &col : portMap_) {
    col.resize(h, std::array<Port *, 4>());
  }

  QRandomGenerator gen;

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int rand = rng.generate() % 1000;
      groundMap_[i][j] = (rand < 200) ? randomItemFactory() : nullptr;
    }
  }
}

Selector::Selector(QObject *parent)
    : QObject(parent), x(0), y(0), path_({QPoint(0, 0)}) {}

void Selector::clear() {
  x = y = 0;
  path_ = QList<QPoint>({QPoint(x, y)});
  prepareGeometryChange();
  ensureVisible();
  update();
}

void Selector::move(rotate_t d) {
  x = x + dx[d];
  y = y + dy[d];
  path_.push_back(QPoint(x, y));
  prepareGeometryChange();
  ensureVisible();
  update();
}

QList<QPoint> Selector::path() { return path_; }

QRectF Selector::boundingRect() const {
  int xmin, xmax, ymin, ymax;
  xmin = xmax = path_.first().x();
  ymin = ymax = path_.first().y();
  for (const auto &p : path_) {
    xmin = std::min(xmin, p.x());
    xmax = std::max(xmax, p.x());
    ymin = std::min(ymin, p.y());
    ymax = std::max(ymax, p.y());
  }
  return QRectF(xmin * L - L / 2, ymin * L - L / 2, (xmax - xmin + 1) * L,
                (ymax - ymin + 1) * L);
}

QPainterPath Selector::shape() const {
  QPainterPath path;
  path.setFillRule(Qt::WindingFill);

  for (const auto &p : path_) {
    int x = p.x(), y = p.y();
    path.addRect(QRectF(x * L - L / 2, y * L - L / 2, L, L));
  }

  return path;
}

void Selector::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget) {
  assert(path_.size() >= 1);
  painter->save();
  painter->setBrush(QColor(0, 0, 255, 50));
  for (QPoint p : path_) {
    int x = p.x(), y = p.y();
    painter->drawRect(QRectF(x * L - L / 2, y * L - L / 2, L, L));
  }
  painter->drawRect(
      QRectF(path_.back().x() * L - L / 2, path_.back().y() * L - L / 2, L, L));
  painter->restore();
}

void GameState::timerEvent(QTimerEvent *e) {
  if (!pause_ && e->timerId() == timerId)
    scene->advance();
}

void GameState::keyPressEvent(QKeyEvent *e) {
  using namespace Qt;
  if (selectorState) {
    if (e->key() == Key_Space)
      return;
    switch (e->key()) {
    case Key_H:
    case Key_Left:
      moveSelector(R180);
      break;
    case Key_J:
    case Key_Down:
      moveSelector(R270);
      break;
    case Key_K:
    case Key_Up:
      moveSelector(R90);
      break;
    case Key_L:
    case Key_Right:
      moveSelector(R0);
      break;
    }
  } else {
    if (e->modifiers() == Qt::ControlModifier) {
      switch (e->key()) {
      case Qt::Key_S:
        emit saveEvent();
        break;
      case Qt::Key_Equal:
        emit zoomIn();
        break;
      case Qt::Key_Minus:
        emit zoomOut();
        break;
      case Qt::Key_0:
        emit zoomReset();
        break;
      }
      return;
    }
    if (e->key() == Key_Space) {
      selectorState = true;
      offset = QPoint(0, 0);
      return;
    }
    switch (e->key()) {
    case Key_H:
    case Key_Left:
      shiftSelector(R180);
      break;
    case Key_J:
    case Key_Down:
      shiftSelector(R270);
      break;
    case Key_K:
    case Key_Up:
      shiftSelector(R90);
      break;
    case Key_L:
    case Key_Right:
      shiftSelector(R0);
      break;
    case Key_R:
      rotate = rotate_t((rotate + 1) % 4);
      selector->setRotation(-rotate * 90);
      break;
    case Key_1:
      changeDevice(device_id_t(0));
      break;
    case Key_2:
      changeDevice(device_id_t(1));
      break;
    case Key_3:
      changeDevice(device_id_t(2));
      break;
    case Key_4:
      changeDevice(device_id_t(3));
      break;
    case Key_5:
      changeDevice(device_id_t(4));
      break;
    case Key_6:
      changeDevice(device_id_t(5));
      break;
    case Key_D:
      removeDevice(base);
      break;
    case Key_Equal:
      enhanceDevice(deviceId);
      break;
    case Key_S:
      shopOpenEvent();
      break;
    case Key_C:
      center->ensureVisible();
      break;
    }
  }
}

void GameState::keyReleaseEvent(QKeyEvent *e) {
  using namespace Qt;
  if (selectorState) {
    if (e->key() == Key_Space) {
      selectorState = false;
      if (deviceFactory == nullptr) {
        selector->clear();
        return;
      }
      ItemFactory *ground = groundMap(base);
      QList<PortHint> hints = getPortHint(base, rotate, selector->path());
      Device *device =
          deviceFactory->createDevice(selector->path(), hints, ground);
      selector->clear();
      if (device == nullptr) {
        qCritical() << "create device failed.";
        return;
      }
      installDevice(base, rotate, device);
      return;
    }
  }
}

void GameState::enhanceChange()
{
  enhance ++;
  emit enhanceChangeEvent(enhance);
}

void GameState::moneyChange(int delta)
{
  money += delta * moneyRatio;
  emit moneyChangeEvent(money);
}

void GameState::shopOpenEvent()
{
  Shop shop(money, moneyRatio, itemRatio, nextW, nextH, window);
  shop.exec();
  emit moneyChangeEvent(money);
}

void GameState::mapConstructEvent()
{
  pause_ = true;
  killTimer(timerId);
  std::vector<Device *> devList;
  for (auto &[dev, desc]: devices) {
    devList.push_back(dev);
  }

  for (auto &dev: devList) {
    if (dynamic_cast<Center *>(dev)) {
      continue;
    }
    removeDevice(dev);
  }
  scene->removeItem(center);
  devices.erase(center);

  w = nextW; h = nextH;
  naiveInitMap(w, h);

  QPoint centerBase = {rng.bounded(w - 4), rng.bounded(h - 4)};
  rotate_t centerRotate = R0;
  installDevice(centerBase, centerRotate, center);
  resetDeviceRatio();
  for (int i = 0; i < DEV_NONE; i ++) {
    emit deviceRatioChangeEvent(device_id_t(i), getDeviceRatio(device_id_t(i)));
  }
  assert(scene->items().size() == 2);

  scene->update(0, 0, w * L, h * L);
  pause_ = false;
  timerId = startTimer(1000/FPS);
}

bool GameState::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    keyPressEvent(static_cast<QKeyEvent *>(event));
    return true;
  } else if (event->type() == QEvent::KeyRelease) {
    keyReleaseEvent(static_cast<QKeyEvent *>(event));
    return true;
  } else {
    return QObject::eventFilter(object, event);
  }
}

DeviceDescription::DeviceDescription(QPoint point, rotate_t rotate)
    : p(point), r(rotate) {}

DeviceDescription::DeviceDescription(int x, int y, rotate_t rotate)
    : p(QPoint(x, y)), r(rotate) {}

Scene::Scene(int w, int h, GameState &game, QObject *parent)
  : QGraphicsScene(parent), w(w), h(h), game(game) {}

void Scene::drawBackground(QPainter *painter, const QRectF &rect) {
  painter->save();
  painter->setPen(Qt::gray);

  qreal x1, y1, x2, y2;
  rect.getCoords(&x1, &y1, &x2, &y2);

  int sx, ex, sy, ey;
  sx = floor(x1 / L) * L;
  sy = floor(y1 / L) * L;
  ex = ceil(x2 / L) * L;
  ey = ceil(y2 / L) * L;

  for (int x = sx; x < ex; x += L) {
    for (int y = sy; y < ey; y += L) {
      painter->save();
      if (game.inRange(x / L, y / L)) {
        if (auto f = game.groundMap(x / L, y / L)) {
          QColor color = f->color();
          if (color == Qt::gray) {
            color = Qt::darkGray;
          }
          painter->setBrush(QColor(f->color()).lighter());
        }
      }
      painter->drawRect(x, y, L, L);
      painter->restore();
    }
  }

  painter->restore();
}

void Scene::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                      const QStyleOptionGraphicsItem options[],
                      QWidget *widget) {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  QGraphicsScene::drawItems(painter, numItems, items, options, widget);
  painter->restore();
}

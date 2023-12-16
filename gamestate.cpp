#include "gamestate.h"

GameState::GameState(int w, int h, Scene *&scene, QMainWindow *parent)
    : QGraphicsScene(parent), window(parent), w(w), h(h),
      selector(new Selector(this)), base(QPoint(0, 0)), offset(QPoint(0, 0)),
      rotate(R0), selectorState(false), pause_(false), deviceFactory(nullptr) {
  assert(window);
  assert(selector);

  scene = new Scene(w, h, *this, parent);
  this->scene = scene;

  scene->addItem(selector);
  selector->setPos(L / 2, L / 2);
  scene->installEventFilter(this);

  navieInitMap(w, h);

  startTimer(1000 / FPS);
}

void GameState::save(QDataStream &out)
{
  out << w << h;

  for (int i = 0; i < w; i ++) {
    for (int j = 0; j < h; j ++) {
      saveItemFactory(out, groundMap(i, j));
    }
  }

  out << (int) devices.size();
  for (auto &[p, desc]: devices) {
    out << desc.p << desc.r;
    saveDevice(out, p);
  }
}

void GameState::pause(bool paused) { this->pause_ = paused; }

GameState::GameState(QDataStream &in, Scene *&scene, QMainWindow *parent)
  : selector(new Selector(this)), base(QPoint(0, 0)), offset(QPoint(0, 0)),
    rotate(R0), selectorState(false), pause_(false), deviceFactory(nullptr)
{
  loadMap(in);
  scene = new Scene(w, h, *this, parent);
  this->scene = scene;

  scene->addItem(selector);
  selector->setPos(L / 2, L / 2);
  scene->installEventFilter(this);

  int nr_device;
  in >> nr_device;
  // sanity check
  assert(nr_device >= 0);
  assert(devices.empty());

  for (int i = 0; i < nr_device; i ++) {
    QPoint base;
    rotate_t rotate;
    in >> base >> rotate;
    qDebug() << "Loading device at" << base << rotate;
    Device *dev = loadDevice(in);

    devices.insert({dev, {base, rotate}});
  }

  for (auto &[dev, desc]: devices) {
    assert(dev);
    installDevice(desc.p, desc.r, dev);
    restoreDevice(dev, groundMap(desc.p));
  }

  startTimer(1000 / FPS);
}

void GameState::loadMap(QDataStream &in) {
  in >> w >> h;

  groundMap_.resize(w);
  for (auto &col: groundMap_) {
    col.resize(h);
  }

  deviceMap_.resize(w);
  for (auto &col: deviceMap_) {
    col.resize(h);
  }

  portMap_.resize(w);
  for (auto &col: portMap_) {
    col.resize(h);
  }

  for (int i = 0; i < w; i ++) {
    for (int j = 0; j < h; j ++) {
      groundMap_[i][j] = loadItemFactory(in);
    }
  }
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
  scene->removeItem(device);

  devices.erase(device);
  delete device;
}

void GameState::removeDevice(int x, int y) {
  Device *d = deviceMap(x, y);
  if (d) {
    removeDevice(d);
  }
}

void GameState::removeDevice(QPoint p) { removeDevice(p.x(), p.y()); }

void GameState::changeDevice(device_id_t id) {
  DeviceFactory *nf = getDeviceFactory(id);
  if (nf == deviceFactory) {
    deviceFactory = nullptr;
    emit deviceChangeEvent(DEV_NONE);
  } else {
    deviceFactory = nf;
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

QList<PortHint> GameState::getPortHint(QPoint base, rotate_t rotate, const QList<QPoint> &blocks)
{
  QList<PortHint> hints;
  for (auto p: blocks) {
    std::array<Port *, 4> ports = {};
    // cpp grammar check
    assert(ports[0] == nullptr);
    assert(ports[1] == nullptr);
    assert(ports[2] == nullptr);
    assert(ports[3] == nullptr);

    QPoint realp = mapToMap(p, base, rotate);

    for (int d = 0; d < 4; d ++) {
      // out-of-shape check
      QPoint np = p + QPoint(dx[d], dy[d]);
      if (blocks.contains(np))
        continue;

      rotate_t r = rotate_t((rotate + d)%4);
      ports[d] = otherPort(realp, r);
    }

    hints.push_back(PortHint(ports));
  }

  return hints;
}

void GameState::navieInitMap(int w, int h)
{
  this->w = w; this->h = h;

  groundMap_.resize(w);
  for (auto &col: groundMap_) {
    col.resize(h);
  }

  deviceMap_.resize(w);
  for (auto &col: deviceMap_) {
    col.resize(h);
  }

  portMap_.resize(w);
  for (auto &col: portMap_) {
    col.resize(h);
  }

  QRandomGenerator gen;

  for (int i = 0; i < w; i ++) {
    for (int j = 0; j < h; j ++) {
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

void GameState::timerEvent(QTimerEvent *) { scene->advance(); }

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
    if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_S) {
      QFile saveslot("save.bin");
      saveslot.open(QIODevice::WriteOnly);
      QDataStream savestream(&saveslot);
      save(savestream);
      saveslot.close();
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
      Device *device = deviceFactory->createDevice(selector->path(), hints, ground);
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


Scene::Scene(int w, int h, GameState& game, QObject *parent)
  : QGraphicsScene(parent), w(w), h(h), game(game)
{

}

void Scene::drawBackground(QPainter *painter, const QRectF &rect)
{
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
        if (auto f = game.groundMap(x/L, y/L)) {
          painter->setBrush(f->color());
        }
      }
      painter->drawRect(x, y, L, L);
      painter->restore();
    }
  }

  painter->restore();
}

#include "device.h"
#include <set>

Device::Device(int speed, const QList<QPoint> &blocks) : blocks_(blocks) {
  assert(!blocks.empty());
  setSpeed(speed);
}

void Device::save(QDataStream &out)
{
  qDebug() << "Saveing base:" << speed << frameCount << blocks_;
  out << speed << frameCount << blocks_;
}

const QList<QPoint> &Device::blocks() const { return blocks_; }

void Device::advance(int phase) {
  if (frameCount >= speed) {
    next();
    frameCount = 0;
  } else {
    frameCount++;
  }
}

void Device::setSpeed(int speed) {
  // sanity check, bound: [1/FPS, 10) s
  assert(speed >= 1 && speed < 10 * FPS);
  this->speed = speed;
}

Device::Device(QDataStream &in)
{
  in >> speed >> frameCount >> blocks_;
  setSpeed(speed);
  assert(blocks_.size() >= 1);
}

QRectF Device::boundingRect() const {
  int xmin, xmax, ymin, ymax;
  xmin = xmax = blocks_.first().x();
  ymin = ymax = blocks_.first().y();
  for (const auto &p : blocks_) {
    xmin = std::min(xmin, p.x());
    xmax = std::max(xmax, p.x());
    ymin = std::min(ymin, p.y());
    ymax = std::max(ymax, p.y());
  }
  return QRectF(xmin * L - L / 2, ymin * L - L / 2, (xmax - xmin + 1) * L,
                (ymax - ymin + 1) * L);
}

QPainterPath Device::shape() const {
  QPainterPath path;
  path.setFillRule(Qt::WindingFill);

  for (const auto &p : blocks_) {
    int x = p.x(), y = p.y();
    path.addRect(QRectF(x * L - L / 2, y * L - L / 2, L, L));
  }

  return path;
}

void Device::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget) {
  qWarning() << "default device image is painted.";
  painter->save();
  for (auto &p : blocks_) {
    int x = p.x(), y = p.y();
    painter->drawRect(x * L - L / 2, y * L - L / 2, L, L);
    painter->drawLine(x * L - L / 2, y * L - L / 2, x * L + L / 2,
                      y * L + L / 2);
    painter->drawLine(x * L + L / 2, y * L - L / 2, x * L - L / 2,
                      y * L + L / 2);
  }
  painter->restore();
}

Miner::Miner(ItemFactory *factory, int speed)
  : Device(speed), factory(factory) {}

void Miner::save(QDataStream &out)
{
  Device::save(out);
}

void Miner::restore(ItemFactory *f)
{
  factory = f;
}

void Miner::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  painter->setBrush(QBrush(Qt::green));
  static const QPoint points[3] = {
      {-L / 2, -L / 2}, {-L / 2, L / 2}, {L / 3, 0}};
  painter->drawPolygon(points, 3);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Miner::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  ret.push_back({static_cast<Port *>(&out), {QPoint(0, 0), R0}});
  return ret;
}

Miner::Miner(QDataStream &in)
  : Device(in)
{

}

void Miner::next() {
  if (!factory)
    return;
  Item *item = factory->createItem();
  if (out.send(item) == false) {
    delete item;
  }
}

Belt::Belt(const QList<QPoint> &blocks, rotate_t inDirection,
           rotate_t outDirection, int speed)
    : Device(speed, blocks), inDirection(inDirection), outDirection(outDirection) {
  buffer.resize(blocks.size() - 1, nullptr);
  direction.resize(blocks.size());
  turn.resize(blocks.size());

  direction.back() = outDirection;
  for (int i = blocks.size() - 2; i >= 0; i --) {
    QPoint p = blocks[i], np = blocks[i+1];
    for (int d = 0; d < 4; d ++) {
      if (p + QPoint(dx[d], dy[d]) == np) {
        direction[i] = rotate_t(d);
        break;
      }
    }
  }

  if (inDirection == direction.front()) {
    turn.front() = PASS_THROUGH;
  } else if (rotateL(inDirection) == direction.front()) {
    turn.front() = TURN_LEFT;
  } else if (rotateR(inDirection) == direction.front()) {
    turn.front() = TURN_RIGHT;
  } else {
    assert(false);
  }

  for (int i = 1; i < blocks.size(); i ++) {
    QPoint pp = blocks[i-1], p = blocks[i];
    rotate_t inD;
    for (int d = 0; d < 4; d ++) {
      if (pp + dp[d] == p) {
        inD = rotate_t(d);
        break;
      }
    }

    if (inD == direction[i]) {
      turn[i] = PASS_THROUGH;
    } else if (rotateL(inD) == direction[i]) {
      turn[i] = TURN_LEFT;
    } else if (rotateR(inD) == direction[i]) {
      turn[i] = TURN_RIGHT;
    } else {
      assert(false);
    }
  }
}

void Belt::save(QDataStream &out)
{
  Device::save(out);
  out << inDirection << outDirection;

  int n = blocks().size();
  out << n;

  for (int i = 0; i < n; i ++) {
    out << direction[i];
  }
  for (int i = 0; i < n; i ++) {
    out << turn[i];
  }
}

void Belt::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget) {
  painter->save();
  static const QPoint points[4] = {
      {-L / 3, -L / 3},
      {-L / 4, 0},
      {-L / 3, L / 3},
      {L / 4, 0},
  };
  for (int i = 0; i < blocks().size(); i++) {
    painter->save();
    int x = blocks()[i].x(), y = blocks()[i].y();
    painter->translate(x * L, y * L);
    painter->rotate(-direction[i]*90);
    painter->save();
    painter->setBrush(QBrush(Qt::gray));
    painter->drawPolygon(points, 4);
    painter->restore();
    if (i == blocks().size() - 1) {
      if (out.getBuffer()) {
        out.getBuffer()->paint(painter);
      }
    } else {
      if (buffer[i]) {
        buffer[i]->paint(painter);
      }
    }

    painter->restore();
  }
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Belt::ports() {
  return {{&in, {blocks().front(), rotate_t((inDirection + 2) % 4)}},
    {&out, {blocks().back(), outDirection}}};
}

Belt::Belt(QDataStream &in)
  : Device(in)
{
  in >> inDirection >> outDirection;

  int n;
  in >> n;
  assert(n > 0);
  direction.resize(n);
  turn.resize(n);
  for (auto &x: direction) {
    in >> x;
  }
  for (auto &x: turn) {
    in >> x;
  }
  buffer.resize(n - 1);
}

void Belt::next() {
  if (!buffer.empty()) {
    if (buffer.back() && out.ready()) {
      out.send(buffer.back());
      buffer.back() = nullptr;
    }
    for (int i = buffer.size() - 2; i >= 0; i --) {
      if (buffer[i+1] == nullptr) {
        buffer[i+1] = buffer[i];
        buffer[i] = nullptr;
      }
    }
    if (buffer.front() == nullptr && in.ready()) {
      buffer.front() = in.receive();
    }
  } else {
    if (out.ready() && in.ready()) {
      out.send(in.receive());
    }
  }
  update();
}

Trash::Trash(int speed) : Device(speed) {}

void Trash::save(QDataStream &out)
{
  Device::save(out);
}

void Trash::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  painter->drawRect(-L / 2, -L / 2, L, L);
  painter->drawText(QPoint(-L / 2, 0), "Trash");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Trash::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  for (auto &e : in) {
    ret.push_back({&e.first, e.second});
  }
  return ret;
}

Trash::Trash(QDataStream &in)
  : Device(in)
{

}

void Trash::next() {
  for (auto &e : in) {
    const Item *item = e.first.receive();
    if (item) {
      qDebug() << "Trash received item!";
      delete item;
    }
  }
}

Center::Center(int speed)
    : Device(speed, []() {
        QList<QPoint> ret;
        for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++) {
            ret.push_back(QPoint(i, j));
          }
        }
        return ret;
      }()) {}

void Center::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget) {
  painter->save();
  painter->translate(-L / 2, -L / 2);
  painter->drawRect(boundingRect());
  painter->drawText(boundingRect(), "Center");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Center::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  for (auto &p : in) {
    ret.push_back({&p.first, p.second});
  }
  return ret;
}

void Center::next() {
  for (auto &e : in) {
    const Item *item = e.first.receive();
    if (item) {
      delete item;
    }
  }
}

DeviceFactory::DeviceFactory(int speed) : speed_(speed) {}

int DeviceFactory::speed() { return speed_; }

void DeviceFactory::setSpeed(int speed) { speed_ = speed; }

MinerFactory::MinerFactory(int speed) : DeviceFactory(speed) {}

Miner *MinerFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Miner(itemFactory, speed());
}

BeltFactory::BeltFactory(int speed) : DeviceFactory(speed) {}

bool operator<(const QPoint &a, const QPoint &b) {
  if (a.x() != b.x())
    return a.x() < b.x();
  return a.y() < b.y();
}

Belt *BeltFactory::createDevice(const QList<QPoint> &blocks,
                                const QList<PortHint> &hints,
                                ItemFactory *itemFactory) {
  assert(blocks.size() >= 1);
  assert(hints.size() == blocks.size());
  // no overlap or blank check
  std::set<QPoint> points;
  if (blocks.size() > 1) {
    QPoint p = blocks.front();
    points.insert(p);
    for (int i = 1; i < blocks.size(); i++) {
      QPoint np = blocks[i];
      bool flag = false;
      for (int k = 0; k < 4; k++) {
        if (np == p + QPoint(dx[k], dy[k])) {
          flag = true;
          break;
        }
      }
      if (!flag)
        return nullptr;
      if (points.find(np) != points.end())
        return nullptr;
      points.insert(np);
      p = np;
    }
  }

  rotate_t inDirection = R0, outDirection = R0;

  if (blocks.size() > 1) {
    for (int d = 0; d < 4; d++) {
      Port *p = hints.front()[rotate_t(d)];
      if (!p)
        continue;
      if (dynamic_cast<OutputPort *>(p)) {
        inDirection = rotate_t((d + 2) % 4);
        break;
      }
    }

    for (int d = 0; d < 4; d++) {
      Port *p = hints.back()[rotate_t(d)];
      if (!p)
        continue;
      if (dynamic_cast<InputPort *>(p)) {
        outDirection = rotate_t(d);
        break;
      }
    }

    for (int d = 0; d < 4; d ++) {
      const QPoint &a = blocks[0], &b = blocks[1];
      if (a + dp[d] == b) {
        if (inDirection == (d + 2) % 4) {
          inDirection = rotate_t(d);
        }
        break;
      }
    }

    for (int d = 0; d < 4; d ++) {
      const QPoint &a = blocks[blocks.size() - 2], &b = blocks.back();
      if (a + dp[d] == b) {
        if (outDirection == (d + 2) % 4) {
          outDirection = rotate_t(d);
        }
        break;
      }
    }
  }

  return new Belt(blocks, inDirection, outDirection);
}

TrashFactory::TrashFactory(int speed) : DeviceFactory(speed) {}

Trash *TrashFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Trash(speed());
}

static DeviceFactory *globalDeviceFactories[] = {new MinerFactory(),
                                                 new BeltFactory(),
                                                 new CutterFactory(),
                                                 new MixerFactory(),
                                                 new RotatorFactory(),
                                                 new TrashFactory(),
                                                 nullptr};

DeviceFactory *getDeviceFactory(device_id_t id) {
  assert(id < DEV_NONE);
  if (globalDeviceFactories[id] == nullptr) {
    qWarning() << "unsupported device id" << id;
  }
  return globalDeviceFactories[id];
}

Cutter::Cutter(int speed) : Device(speed, {{0, 0}, {0, 1}}), stall(false) {}

void Cutter::save(QDataStream &out)
{
  Device::save(out);
  out << stall;
}

void Cutter::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget) {
  painter->save();
  painter->drawRect(-L / 2, -L / 2, L, 2 * L);
  painter->drawText(QPoint(-L / 2, 0), "Cutter");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Cutter::ports() {
  return {
      {&in, {{0, 0}, R180}},
      {&outU, {{0, 0}, R0}},
      {&outL, {{0, 1}, R0}},
  };
}

Cutter::Cutter(QDataStream &in)
  : Device(in)
{
  in >> stall;
}

void Cutter::next() {
  if (stall)
    return;
  if (outU.ready() && outL.ready()) {
    const Item *item = in.receive();
    if (!item)
      return;
    const Mine *mine = dynamic_cast<const Mine *>(item);
    if (!mine) {
      stall = true;
      return;
    }
    outU.send(mine->cutUpper());
    outL.send(mine->cutLower());
    delete mine;
  }
}

CutterFactory::CutterFactory(int speed) : DeviceFactory(speed) {}

Cutter *CutterFactory::createDevice(const QList<QPoint> &blocks,
                                    const QList<PortHint> &hints,
                                    ItemFactory *itemFactory) {
  return new Cutter(speed());
}

Rotator::Rotator(int speed) : Device(speed) {}

void Rotator::save(QDataStream &out)
{
  Device::save(out);
}

void Rotator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget) {
  painter->save();
  painter->drawRect(-L / 2, -L / 2, L, L);
  painter->drawText(QPoint(-L / 2, 0), "Rotator");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Rotator::ports() {
  return {
      {&in, {{0, 0}, R180}},
      {&out, {{0, 0}, R0}},
  };
}

Rotator::Rotator(QDataStream &in)
  : Device(in)
{
}

void Rotator::next() {
  if (out.ready()) {
    auto item = in.receive();
    if (!item)
      return;
    auto mine = dynamic_cast<const Mine *>(item);
    if (mine) {
      out.send(mine->rotateR());
      delete mine;
    } else {
      out.send(item);
    }
  }
}

RotatorFactory::RotatorFactory(int speed) : DeviceFactory(speed) {}

Rotator *RotatorFactory::createDevice(const QList<QPoint> &blocks,
                                      const QList<PortHint> &hints,
                                      ItemFactory *itemFactory) {
  return new Rotator(speed());
}

Mixer::Mixer(int speed) : Device(speed, {{0, 0}, {1, 0}}), stall(false) {}

void Mixer::save(QDataStream &out)
{
  Device::save(out);
  out << stall;
}

void Mixer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  painter->drawRect(-L / 2, -L / 2, 2 * L, L);
  painter->drawText(QPoint(-L / 2, 0), "Mixer");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Mixer::ports() {
  return {
      {&inMine, {{0, 0}, R180}},
      {&inTrait, {{0, 0}, R90}},
      {&out, {{1, 0}, R0}},
  };
}

Mixer::Mixer(QDataStream &in)
  : Device(in)
{
  in >> stall;
}

void Mixer::next() {
  if (!(inMine.ready() && inTrait.ready() && out.ready()))
    return;
  const Item *itemMine = inMine.receive();
  const Item *itemTrait = inTrait.receive();
  const Mine *mine = dynamic_cast<const Mine *>(itemMine);
  const TraitMine *trait = dynamic_cast<const TraitMine *>(itemTrait);
  if (!mine || !trait) {
    stall = true;
    delete itemMine;
    delete itemTrait;
    return;
  }

  out.send(mine->setTrait(trait->getTrait()));
  delete itemMine;
  delete itemTrait;
}

MixerFactory::MixerFactory(int speed) : DeviceFactory(speed) {}

Mixer *MixerFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Mixer(speed());
}

// serialize
void saveDevice(QDataStream &out, Device *dev) {
  qDebug() << "saveDevice";
  if (!dev) {
    out << QChar('N');
    return;
  } else if (dynamic_cast<Miner *>(dev)) {
    out << QChar('M');
  } else if (dynamic_cast<Belt *>(dev)) {
    out << QChar('B');
  } else if (dynamic_cast<Cutter *>(dev)) {
    out << QChar('C');
  } else if (dynamic_cast<Mixer *>(dev)) {
    out << QChar('X');
  } else if (dynamic_cast<Rotator *>(dev)) {
    out << QChar('R');
  } else if (dynamic_cast<Trash *>(dev)) {
    out << QChar('T');
  } else {
    assert(false);
  }
  // its GameState's responsibility to handle Center
  dev->save(out);
}

Device *loadDevice(QDataStream &in) {
  qDebug() << "loadDevice";
  QChar id;
  in >> id;
  qDebug() << "id is " << id << (int)id.toLatin1();
  if (id == 'N') {
    return nullptr;
  } else if (id == 'M') {
    return new Miner(in);
  } else if (id == 'B') {
    return new Belt(in);
  } else if (id == 'C') {
    return new Cutter(in);
  } else if (id == 'X') {
    return new Mixer(in);
  } else if (id == 'T') {
    return new Trash(in);
  } else if (id == 'R') {
    return new Rotator(in);
  } else {
    assert(false);
  }
  // its GameState's responsibility to handle Center
}

void restoreDevice(Device *dev, ItemFactory *f)
{
  if (auto miner = dynamic_cast<Miner *>(dev)) {
    miner->restore(f);
  }
}

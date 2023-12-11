#include "device.h"
#include <set>

Device::Device(int speed, const QList<QPoint> &blocks) : blocks_(blocks) {
  assert(!blocks.empty());
  setSpeed(speed);
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
  // sanity check, bound: [1, 10) s
  assert(speed >= 1 && speed < 10 * FPS);
  this->speed = speed;
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
  return QRectF(xmin * L - L / 2, ymin * L - L / 2, (xmax-xmin+1)*L,
                (ymax-ymin+1)*L);
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

void Miner::next() {
  if (!factory)
    return;
  Item *item = factory->createItem();
  if (out.send(item) == false) {
    delete item;
  }
}

Belt::Belt(int speed, const QList<QPoint> &blocks) : Device(speed, blocks) {
  buffer.resize(blocks.size(), nullptr);
}

void Belt::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget) {
  painter->save();
  painter->setBrush(QBrush(Qt::gray));
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
    painter->save();
    painter->drawPolygon(points, 4);
    painter->restore();
    if (buffer[i]) {
      buffer[i]->paint(painter);
    }
    painter->restore();
  }
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Belt::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  ret.append({
      {static_cast<Port *>(&in), {blocks().front(), R180}},
      {static_cast<Port *>(&out), {blocks().back(), R0}},
  });
  return ret;
}

void Belt::next() {
  if (buffer.back()) {
    if (out.send(buffer.back())) {
      buffer.back() = nullptr;
    }
  }
  for (int i = buffer.size() - 2; i >= 0; i--) {
    if (buffer[i + 1] == nullptr) {
      buffer[i + 1] = buffer[i];
      buffer[i] = nullptr;
    }
  }
  if (buffer[0] == nullptr) {
    buffer[0] = in.receive();
  }
  update();
}

Trash::Trash(int speed) : Device(speed) {}

void Trash::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  painter->drawRect(-L / 2, -L / 2, L, L);
  painter->drawText(QPoint(-L/2, 0), "Trash");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Trash::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  for (auto &e : in) {
    ret.push_back({&e.first, e.second});
  }
  return ret;
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

MinerFactory::MinerFactory(int speed)
    : DeviceFactory(speed) {
}

Miner *MinerFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) {
  return new Miner(itemFactory, speed());
}

BeltFactory::BeltFactory(int speed) : DeviceFactory(speed) {}

bool operator<(const QPoint& a, const QPoint& b) {
  if (a.x() != b.x()) return a.x() < b.x();
  return a.y() < b.y();
}

Belt *BeltFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) {
  assert(blocks.size() >= 1);
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
  return new Belt(speed(), blocks);
}

TrashFactory::TrashFactory(int speed) : DeviceFactory(speed) {}

Trash *TrashFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory) {
  return new Trash(speed());
}

static DeviceFactory *globalDeviceFactories[] = {
  new MinerFactory(),
  new BeltFactory(),
  new CutterFactory(),
  new MixerFactory(),
  new RotatorFactory(),
  new TrashFactory(),
  nullptr
};

DeviceFactory *getDeviceFactory(device_id_t id) {
  assert(id < DEV_NONE);
  if (globalDeviceFactories[id] == nullptr) {
    qWarning() << "unsupported device id" << id;
  }
  return globalDeviceFactories[id];
}

Cutter::Cutter(int speed)
  : Device(speed, { {0, 0}, {0, 1} }), stall(false)
{

}

void Cutter::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->save();
  painter->drawRect(-L/2, -L/2, L, 2*L);
  painter->drawText(QPoint(-L/2,0), "Cutter");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t> > > Cutter::ports()
{
  return {
    { &in, { {0, 0}, R180 }},
    { &outU, {{0, 0}, R0 }},
    { &outL, {{0, 1}, R0 }},
  };
}

void Cutter::next()
{
  if (stall) return;
  if (outU.ready() && outL.ready()) {
    const Item *item = in.receive();
    if (!item) return;
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

CutterFactory::CutterFactory(int speed)
  : DeviceFactory(speed)
{

}

Cutter *CutterFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory)
{
  return new Cutter(speed());
}

Rotator::Rotator(int speed)
  : Device(speed)
{
}

void Rotator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->save();
  painter->drawRect(-L/2, -L/2, L, L);
  painter->drawText(QPoint(-L/2, 0), "Rotator");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t> > > Rotator::ports()
{
  return {
    { &in, {{0, 0}, R180}},
    { &out, {{0, 0}, R0 }},
  };
}

void Rotator::next()
{
  if (out.ready()) {
    auto item = in.receive();
    if (!item) return;
    auto mine = dynamic_cast<const Mine *>(item);
    if (mine) {
      out.send(mine->rotateR());
      delete mine;
    } else {
      out.send(item);
    }
  }
}

RotatorFactory::RotatorFactory(int speed)
  : DeviceFactory(speed)
{

}

Rotator *RotatorFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory)
{
  return new Rotator(speed());
}

Mixer::Mixer(int speed)
  : Device(speed, {{0, 0}, {1, 0}}), stall(false)
{

}

void Mixer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->save();
  painter->drawRect(-L/2, -L/2, 2*L, L);
  painter->drawText(QPoint(-L/2, 0), "Mixer");
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t> > > Mixer::ports()
{
  return {
    { &inMine, {{0, 0}, R180}},
    { &inTrait, {{0, 0}, R90}},
    { &out, {{0, 1}, R0}},
  };
}

void Mixer::next()
{
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

MixerFactory::MixerFactory(int speed)
  : DeviceFactory(speed)
{

}

Mixer *MixerFactory::createDevice(const QList<QPoint> &blocks, ItemFactory *itemFactory)
{
  return new Mixer(speed());
}

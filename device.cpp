#include "device.h"
#include <set>
#include <string>

Device::Device(const QList<QPoint> &blocks) : blocks_(blocks) {
  assert(!blocks.empty());
}

void Device::save(QDataStream &out) { out << frameCount << blocks_; }

const QList<QPoint> &Device::blocks() const { return blocks_; }

void Device::advance(int phase) {
  int period;
  qreal realSpeed = speed() * ratio();
  if (realSpeed <= 0) {
    return;
  }
  period = FPS / realSpeed;
  if (frameCount >= period) {
    next();
    frameCount = 0;
  } else {
    frameCount++;
  }
}

Device::Device(QDataStream &in) {
  in >> frameCount >> blocks_;
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

Miner::Miner(ItemFactory *factory) : Device(), factory(factory) {}

qreal Miner::ratio_ = 1;

void Miner::save(QDataStream &out) { Device::save(out); }

void Miner::restore(ItemFactory *f) { factory = f; }

void Miner::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  static const QImage image(":/device/miner");
  painter->drawImage(QRect(-L / 2, -L / 2, L, L), image);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Miner::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  ret.push_back({static_cast<Port *>(&out), {QPoint(0, 0), R0}});
  return ret;
}

Miner::Miner(QDataStream &in) : Device(in) {}

void Miner::next() {
  if (!factory)
    return;
  Item *item = factory->createItem();
  if (out.send(item) == false) {
    delete item;
  }
}

Belt::Belt(const QList<QPoint> &blocks, rotate_t inDirection,
           rotate_t outDirection)
    : Device(blocks), inDirection(inDirection), outDirection(outDirection) {
  length = blocks.size();
  buffer.resize(length - 1, nullptr);
  direction.resize(length);
  turn.resize(length);

  direction.back() = outDirection;
  for (int i = length - 2; i >= 0; i--) {
    QPoint p = blocks[i], np = blocks[i + 1];
    for (int d = 0; d < 4; d++) {
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

  for (int i = 1; i < length; i++) {
    QPoint pp = blocks[i - 1], p = blocks[i];
    rotate_t inD;
    for (int d = 0; d < 4; d++) {
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

qreal Belt::ratio_ = 1;

void Belt::save(QDataStream &out) {
  Device::save(out);
  out << inDirection << outDirection;

  out << length;

  for (int i = 0; i < length; i++) {
    out << direction[i];
  }
  for (int i = 0; i < length; i++) {
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
    painter->rotate(-direction[i] * 90);
    static const QImage imageL(":/device/belt_left"),
        imageP(":/device/belt_pass"), imageR(":/device/belt_right");
    painter->drawImage(QRect(-L / 2, -L / 2, L, L),
                       (turn[i] == TURN_LEFT)    ? imageL
                       : (turn[i] == TURN_RIGHT) ? imageR
                                                 : imageP);
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

Belt::Belt(QDataStream &in) : Device(in) {
  in >> inDirection >> outDirection;

  in >> length;
  assert(length > 0);
  direction.resize(length);
  turn.resize(length);
  for (auto &x : direction) {
    in >> x;
  }
  for (auto &x : turn) {
    in >> x;
  }
  buffer.resize(length - 1);
}

void Belt::next() {
  if (!buffer.empty()) {
    if (buffer.back() && out.ready()) {
      out.send(buffer.back());
      buffer.back() = nullptr;
    }
    for (int i = buffer.size() - 2; i >= 0; i--) {
      if (buffer[i + 1] == nullptr) {
        buffer[i + 1] = buffer[i];
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

qreal Belt::speed() { return BELT_SPEED; }

qreal Belt::ratio() { return ratio_; }

Trash::Trash() : Device() {}

qreal Trash::ratio_;

void Trash::save(QDataStream &out) { Device::save(out); }

void Trash::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  static const QImage image(":/device/trash");
  painter->drawImage(boundingRect(), image);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Trash::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  for (auto &e : in) {
    ret.push_back({&e.first, e.second});
  }
  return ret;
}

Trash::Trash(QDataStream &in) : Device(in) {}

void Trash::next() {
  for (auto &e : in) {
    const Item *item = e.first.receive();
    if (item) {
      qDebug() << "Trash received item!";
      delete item;
    }
  }
}

qreal Trash::speed()
{
  return TRASH_SPEED;
}

qreal Trash::ratio()
{
  return ratio_;
}

Center::Center(int size)
    : Device([=]() {
        QList<QPoint> ret;
        for (int i = 0; i < size; i++) {
          for (int j = 0; j < size; j++) {
            ret.push_back(QPoint(i, j));
          }
        }
        return ret;
      }()),
      size(size) {
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{size - 1, i}, R0}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{0, i}, R180}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{i, 0}, R90}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{i, size - 1}, R270}});
  }
}

Center::~Center() {
  for (auto &[device, desc] : in) {
    delete device;
  }
}

void Center::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget) {
  painter->save();
  static const QImage image(":/device/center");
  painter->drawImage(boundingRect(), image);
  if (icon)
    painter->drawPicture(0, 2 * L, *icon);
  using std::to_string;
  painter->drawText(L / 4, 0, L / 2, L * 2 / 3, Qt::AlignCenter,
                    to_string(problemSet).c_str());
  painter->drawText(2 * L, 0, to_string(task).c_str());
  painter->drawText(
      0, 0, 4 * L, 4 * L, Qt::AlignCenter,
      (to_string(received) + " / " + to_string(required)).c_str());
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Center::ports() {
  QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> ret;
  for (auto &[dev, desc] : in) {
    ret.push_back({dev, desc});
  }
  return ret;
}

Center::Center(QDataStream &sin) : Device(sin) {
  sin >> size;
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{size - 1, i}, R0}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{0, i}, R180}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{i, 0}, R90}});
  }
  for (int i = 0; i < size; i++) {
    in.push_back({new InputPort(), {{i, size - 1}, R270}});
  }
}

void Center::next() {
  for (auto &e : in) {
    const Item *item = e.first->receive();
    if (item) {
      qDebug() << "Center received item!";
      emit receiveItem(item);
      delete item;
    }
  }
}

qreal Center::speed()
{
  return CENTER_SPEED;
}

qreal Center::ratio()
{
  return ratio_;
}

DeviceFactory::DeviceFactory() {}

MinerFactory::MinerFactory() {}

Miner *MinerFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Miner(itemFactory);
}

BeltFactory::BeltFactory() {}

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

    for (int d = 0; d < 4; d++) {
      const QPoint &a = blocks[0], &b = blocks[1];
      if (a + dp[d] == b) {
        if (inDirection == (d + 2) % 4) {
          inDirection = rotate_t(d);
        }
        break;
      }
    }

    for (int d = 0; d < 4; d++) {
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

TrashFactory::TrashFactory() {}

Trash *TrashFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Trash();
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

Cutter::Cutter() : Device({{0, 0}, {0, 1}}), stall(false) {}

qreal Cutter::ratio_ = 1;

void Cutter::save(QDataStream &out) {
  Device::save(out);
  out << stall;
}

void Cutter::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget) {
  painter->save();
  static const QImage image(":/device/cutter");
  painter->drawImage(boundingRect(), image);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Cutter::ports() {
  return {
      {&in, {{0, 0}, R180}},
      {&outU, {{0, 0}, R0}},
      {&outL, {{0, 1}, R0}},
  };
}

Cutter::Cutter(QDataStream &in) : Device(in) { in >> stall; }

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

qreal Cutter::speed()
{
  return CUTTER_SPEED;
}

qreal Cutter::ratio()
{
  return ratio_;
}

CutterFactory::CutterFactory() {}

Cutter *CutterFactory::createDevice(const QList<QPoint> &blocks,
                                    const QList<PortHint> &hints,
                                    ItemFactory *itemFactory) {
  return new Cutter();
}

Rotator::Rotator() {}

qreal Rotator::ratio_ = 1;

void Rotator::save(QDataStream &out) { Device::save(out); }

void Rotator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget) {
  painter->save();
  static const QImage image(":/device/rotater");
  painter->drawImage(boundingRect(), image);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Rotator::ports() {
  return {
      {&in, {{0, 0}, R180}},
      {&out, {{0, 0}, R0}},
  };
}

Rotator::Rotator(QDataStream &in) : Device(in) {}

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

qreal Rotator::speed() { return ROTATOR_SPEED; }

qreal Rotator::ratio() { return ratio_; }

RotatorFactory::RotatorFactory() {}

Rotator *RotatorFactory::createDevice(const QList<QPoint> &blocks,
                                      const QList<PortHint> &hints,
                                      ItemFactory *itemFactory) {
  return new Rotator();
}

Mixer::Mixer() : Device({{0, 0}, {1, 0}}), stall(false) {}

qreal Mixer::ratio_ = 1;

void Mixer::save(QDataStream &out) {
  Device::save(out);
  out << stall;
}

void Mixer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  static const QImage image(":/device/mixer");
  painter->drawImage(QRect(-L / 2, -L / 2, 2 * L, L), image);
  painter->restore();
}

const QList<std::pair<Port *, std::pair<QPoint, rotate_t>>> Mixer::ports() {
  return {
      {&inMine, {{0, 0}, R180}},
      {&inTrait, {{0, 0}, R90}},
      {&out, {{1, 0}, R0}},
  };
}

Mixer::Mixer(QDataStream &in) : Device(in) { in >> stall; }

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

qreal Mixer::speed() { return MIXER_SPEED; }

qreal Mixer::ratio() { return ratio_; }

MixerFactory::MixerFactory() {}

Mixer *MixerFactory::createDevice(const QList<QPoint> &blocks,
                                  const QList<PortHint> &hints,
                                  ItemFactory *itemFactory) {
  return new Mixer();
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
  } else if (dynamic_cast<Center *>(dev)) {
    out << QChar('A');
  } else {
    assert(false);
  }
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
  } else if (id == 'A') {
    return new Center(in);
  } else {
    assert(false);
  }
}

void resetDeviceRatio() {
  Miner::ratio_ = 1;
  Belt::ratio_ = 1;
  Cutter::ratio_ = 1;
  Rotator::ratio_ = 1;
  Cutter::ratio_ = 1;
  Trash::ratio_ = 1;

}
void setDeviceRatio(device_id_t id, qreal ratio) {
  assert(0.5 <= ratio && ratio <= 4);
  switch (id) {
  case MINER:
    Miner::ratio_ = ratio;
    break;
  case BELT:
    Belt::ratio_ = ratio;
    break;
  case CUTTER:
    Cutter::ratio_ = ratio;
    break;
  case ROTATOR:
    Rotator::ratio_ = ratio;
    break;
  case MIXER:
    Mixer::ratio_ = ratio;
    break;
  case TRASH:
    Trash::ratio_ = ratio;
    break;
  default:
    break;
  }
}

void restoreDevice(Device *dev, ItemFactory *f) {
  if (auto miner = dynamic_cast<Miner *>(dev)) {
    miner->restore(f);
  }
}

void Center::save(QDataStream &out) {
  Device::save(out);
  out << size;
}

void Center::updateGoal(int problemSet, int task, int received, int required,
                        const QPicture *icon) {
  this->problemSet = problemSet;
  this->task = task;
  this->received = received;
  this->required = required;
  this->icon = icon;
  update();
}

qreal Miner::speed() { return MINER_SPEED; }

qreal Miner::ratio() { return ratio_; }

void saveDeviceRatio(QDataStream &out) {
  out << Miner::ratio_ << Belt::ratio_ << Cutter::ratio_ << Rotator::ratio_ << Mixer::ratio_ << Trash::ratio_;
}

void loadDeviceRatio(QDataStream &in) {
  in >> Miner::ratio_ >> Belt::ratio_ >> Cutter::ratio_ >> Rotator::ratio_ >> Mixer::ratio_ >> Trash::ratio_;
}

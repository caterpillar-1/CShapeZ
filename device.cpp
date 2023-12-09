#include "device.h"
#include "config.h"

Device::Device(GameState &game, const QPoint &base, rotate_t rotate, const QList<QPoint> &blocks)
  : game(game), base_(base), rotate_(rotate), blocks(blocks) {}

Device::Device(GameState &game, const QPoint &base, rotate_t rotate)
  : game(game), base_(base), rotate_(rotate)
{
  blocks.clear();
  blocks.append(QPoint(0, 0));
}

Device::~Device() {}

void Device::install() {
  for (int i = 0; i < blocks.size(); i++) {
    int x, y;
    rotate_t o;
    convertToMap(i, R0, x, y, o);
    Q_UNUSED(o);
    game.setDevice(x, y, this);
  }
  connectPorts();
}

void Device::remove() {
  for (int i = 0; i < blocks.size(); i++) {
    int x, y;
    rotate_t o;
    convertToMap(i, R0, x, y, o);
    Q_UNUSED(o);
    game.setDevice(x, y, nullptr);
  }
  disconnectPorts();
}

const QList<QPoint> &Device::getBlocks() { return blocks; }

const QPoint &Device::getBase()
{
  return base_;
}

const rotate_t Device::getRotate() { return rotate_; }

void Device::connectPortAt(Port *port, int i, rotate_t d) {
  assert(port);
  qDebug() << "connectPortAt:" << port << i << d;
  int x, y;
  rotate_t o;
  convertToMap(i, d, x, y, o);
  qDebug() << "The dest block is:" << x << y;
  Port *otherPort = game.getOtherPort(x, y, o);
  qDebug() << "The other port is:" << otherPort;
  if (!otherPort) {
    return;
  }
  otherPort->connect(port);
  port->connect(otherPort);
}

void Device::disconnectPortAt(Port *port, int i, rotate_t d)
{
  assert(port);
  int x, y;
  rotate_t o;
  convertToMap(i, d, x, y, o);
  Port *otherPort = game.getOtherPort(x, y, o);
  if (!otherPort) {
    return;
  }
  otherPort->disconnect(true);
  port->disconnect(true);
}

const QPoint Device::coordinate(int i) {
  int x, y;
  rotate_t o;
  convertToMap(i, R0, x, y, o);
  Q_UNUSED(o);
  return QPoint(x * TILE_W, y * TILE_H);
}

QRectF Device::boundingRect() const {
  assert(!blocks.empty());
  assert(blocks.first() == QPoint(0, 0));
  int xmin, xmax, ymin, ymax;
  xmin = xmax = blocks.first().x();
  ymin = ymax = blocks.first().y();
  for (const auto &p : blocks) {
    xmin = std::min(xmin, p.x());
    xmax = std::max(xmax, p.x());
    ymin = std::min(ymin, p.y());
    ymax = std::max(ymax, p.y());
  }
  xmax += 1;
  ymax += 1;
  return QRectF(xmin * TILE_W, ymin * TILE_H, xmax * TILE_W, ymax * TILE_H);
}

QPainterPath Device::shape() const {
  QPainterPath path;
  path.setFillRule(Qt::WindingFill);

  for (const auto &p : blocks) {
    path.addRect(QRectF(p.x() * TILE_W, p.y() * TILE_H, TILE_W, TILE_H));
  }

  return path;
}

void Device::convertToMap(int i, rotate_t d, int &x, int &y, rotate_t &o) {
  assert(0 <= i && i < blocks.size());
  const QPoint &pos = blocks[i];
  switch (d) {
  case R0:
    x = base_.x() + pos.x();
    y = base_.y() + pos.y();
    break;
  case R90:
    x = base_.x() + pos.y();
    y = base_.y() - pos.x();
    break;
  case R180:
    x = base_.x() - pos.x();
    y = base_.y() - pos.y();
    break;
  case R270:
    x = base_.x() - pos.y();
    y = base_.y() + pos.x();
    break;
  default:
    break;
  }
  o = rotate_t((d + rotate_) % 4);
}

Miner::Miner(GameState &game, const QPoint &base, rotate_t rotate)
  : Device(game, base, rotate)
{

}

void Miner::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  painter->drawRect(0, 0, TILE_W, TILE_H);
  QBrush brush(Qt::green);
  painter->setBrush(brush);
  painter->translate(TILE_W / 2, TILE_H / 2);
  painter->rotate(getRotate() * 90);
  const QPointF path[] = {{-5, -5}, {-5, 5}, {3, 0}};
  painter->drawPolygon(path, 3);
  painter->restore();
}

void Miner::connectPorts() { connectPortAt(&out, 0, R0); }

void Miner::disconnectPorts() { out.disconnect(false); }

Belt::Belt(GameState &game, QPoint base, rotate_t rotate, const QList<QPoint> &blocks)
  : Device(game, base, rotate, blocks)
{
  buffer.resize(blocks.size());
}

void Belt::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget) {}

void Belt::connectPorts() {
  for (int d = R0; d < 4; d++) {
    connectPortAt(&in, 0, rotate_t(d));
  }

  for (int d = R0; d < 4; d++) {
    connectPortAt(&out, getBlocks().size() - 1, rotate_t(d));
  }
}

void Belt::disconnectPorts() {
  for (int d = R0; d < 4; d++) {
    disconnectPortAt(&in, 0, rotate_t(d));
  }

  for (int d = R0; d < 4; d++) {
    disconnectPortAt(&out, getBlocks().size() - 1, rotate_t(d));
  }
}

void Belt::advance(int phase) {
  assert(buffer.size() == getBlocks().size());

  if (phaseCount >= BELT_SPEED) {
    Item *item;
    if (out.ready()) {
      if ((item = buffer.back()) != nullptr) {
        out.send(item);
        buffer.back() = nullptr;
      }
    }
    for (int i = buffer.size() - 2; i >= 0; i--) {
      if (buffer[i + 1] == nullptr) {
        buffer[i + 1] = buffer[i];
        if (buffer[i + 1] != nullptr) {
          buffer[i + 1]->setPos(coordinate(i + 1));
        }
        buffer[i] = nullptr;
      }
    }
    if (buffer.front() == nullptr && (item = in.receive()) != nullptr) {
      buffer.front() = item;
      item->show();
      item->setPos(coordinate(0));
    }
  } else {
    phaseCount++;
  }
}

void Miner::advance(int phase) {
//  qDebug() << "Miner::advance";
  if (phaseCount >= MINER_SPEED) {
    if (out.ready()) {
      Item *item = game.getItem(getBase().x(), getBase().y());
      item->setPos(coordinate(0));
      if (item) {
        out.send(item);
        phaseCount = 0;
      }
    }
  } else {
    phaseCount++;
  }
}

MinerFactory::MinerFactory(GameState &game)
  : DeviceFactory(game)
{

}

Miner *MinerFactory::createDevice(QPoint base, rotate_t rotate, const QList<QPoint> &blocks)
{
  return new Miner(getGame(), base, rotate);
}

DeviceFactory::DeviceFactory(GameState &game)
  : game_(game)
{

}

DeviceFactory::~DeviceFactory()
{

}

GameState &DeviceFactory::getGame()
{
  return game_;
}

BeltFactory::BeltFactory(GameState &game)
  : DeviceFactory(game)
{

}

Belt *BeltFactory::createDevice(QPoint base, rotate_t rotate, const QList<QPoint> &blocks)
{
  return new Belt(getGame(), base, rotate, blocks);
}

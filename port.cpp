#include "port.h"

InputPort::InputPort() : otherPort(nullptr) {}

InputPort::~InputPort() {
  if (otherPort) {
    otherPort->disconnect();
  }
}

bool InputPort::ready() const
{
  return otherPort && otherPort->valid();
}

const Item *InputPort::receive() {
  if (otherPort && otherPort->valid()) {
    return otherPort->transmit();
  } else {
    return nullptr;
  }
}

void InputPort::connect(Port *o) {
  if (o == otherPort)
    return;
  if (otherPort) {
    otherPort->disconnect();
  }
  otherPort = dynamic_cast<OutputPort *>(o);
}

void InputPort::disconnect() { otherPort = nullptr; }

OutputPort::OutputPort() : otherPort(nullptr), buffer(nullptr) {}

OutputPort::~OutputPort() {
  if (buffer) {
    delete buffer;
  }
  if (otherPort) {
    otherPort->disconnect();
  }
}

bool OutputPort::send(const Item *item) {
  if (!item) {
    return false;
  }
  if (buffer) {
    return false;
  }
  buffer = item;
  return true;
}

bool OutputPort::ready() const
{
  return buffer == nullptr;
}

const Item *OutputPort::getBuffer()
{
  return buffer;
}

bool OutputPort::valid() const
{
  return buffer != nullptr;
}

const Item *OutputPort::transmit() {
  const Item *ret = buffer;
  buffer = nullptr;
  return ret;
}

void OutputPort::connect(Port *o) {
  if (otherPort) {
    otherPort->disconnect();
  }
  otherPort = dynamic_cast<InputPort *>(o);
}

void OutputPort::disconnect() { otherPort = nullptr; }

Port::~Port() {}

PortHint::PortHint(const std::array<Port *, 4> &otherPorts)
  : otherPorts(otherPorts)
{

}

PortHint::PortHint(const PortHint &o)
  : otherPorts(o.otherPorts)
{

}

const PortHint &PortHint::operator=(const PortHint &o)
{
  otherPorts = o.otherPorts;
  return *this;
}

Port *PortHint::operator[](rotate_t direction) const
{
  return otherPorts[direction];
}

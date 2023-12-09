#include "port.h"

InputPort::InputPort() : otherPort(nullptr) {}

InputPort::~InputPort() {
  if (otherPort) {
    otherPort->disconnect(true);
  }
}

Item *InputPort::receive() {
  if (otherPort) {
    return otherPort->transmit();
  } else {
    return nullptr;
  }
}

void InputPort::connect(Port *o) {
  if (otherPort) {
    disconnect(false);
  }
  otherPort = dynamic_cast<OutputPort *>(o);
}

void InputPort::disconnect(bool passive) {
  if (passive) {
    otherPort = nullptr;
  } else {
    if (otherPort) {
      otherPort->disconnect(true);
    }
    otherPort = nullptr;
  }
}

OutputPort::OutputPort() : otherPort(nullptr), buffer(nullptr) {}

OutputPort::~OutputPort() {
  if (buffer) {
    delete buffer;
  }
  if (otherPort) {
    otherPort->disconnect(true);
  }
}

void OutputPort::send(Item *item) {
  assert(item);
  assert(buffer == nullptr);
  if (buffer) {
    return;
  } else {
    buffer = item;
    return;
  }
}

bool OutputPort::ready() {
  return buffer == nullptr;
}

Item *OutputPort::transmit() {
  Item *ret = buffer;
  buffer = nullptr;
  return ret;
}

void OutputPort::connect(Port *o) {
  if (otherPort) {
    otherPort->disconnect(true);
  }
  otherPort = dynamic_cast<InputPort *>(o);
}

void OutputPort::disconnect(bool passive)
{
  if (passive) {
    otherPort = nullptr;
  } else {
    otherPort->disconnect(true);
    otherPort = nullptr;
  }
}

Port::~Port()
{

}

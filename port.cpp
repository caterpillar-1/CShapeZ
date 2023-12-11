#include "port.h"

InputPort::InputPort() : otherPort(nullptr) {}

InputPort::~InputPort() {
  if (otherPort) {
    otherPort->disconnect();
  }
}

const Item *InputPort::receive() {
  if (otherPort) {
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
  assert(item);
  if (buffer) {
    return false;
  }
  buffer = item;
  return true;
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

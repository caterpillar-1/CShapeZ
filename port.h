#ifndef PORT_H
#define PORT_H

#include "item.h"

class Port {
public:
  virtual void connect(Port *otherPort) = 0;
  virtual void disconnect() = 0;
  virtual ~Port();
};

class InputPort;
class OutputPort;

class InputPort : public Port {
public:
  explicit InputPort();
  ~InputPort();
  // interface for device
  bool ready();
  const Item *receive();

  // Port interface
  void connect(Port *otherPort) override;
  void disconnect() override;

private:
  OutputPort *otherPort;
};

class OutputPort : public Port {
public:
  explicit OutputPort();
  ~OutputPort();
  // interface for device
  bool send(const Item *item);
  bool ready();
  const Item *getBuffer();
  // interface for otherPort
  bool valid();
  const Item *transmit();

  // Port interface
  void connect(Port *otherPort) override;
  void disconnect() override;

private:
  const Item *buffer;
  InputPort *otherPort;
};

#endif // PORT_H

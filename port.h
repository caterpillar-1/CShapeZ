#ifndef PORT_H
#define PORT_H

#include "item.h"

class Port
{
public:
  virtual void connect(Port *otherPort) = 0;
  virtual void disconnect(bool passive) = 0;
  virtual ~Port();
};

class InputPort;
class OutputPort;

class InputPort: public Port {
public:
  InputPort();
  ~InputPort();
  Item *receive();
  void connect(Port *otherPort) override;
  void disconnect(bool passive) override;
private:
  OutputPort *otherPort;
};

class OutputPort: public Port {
public:
  OutputPort();
  ~OutputPort();
  // interface for device
  void send(Item *item);
  bool ready();
  // interface for otherPort
  Item *transmit();
  void connect(Port *otherPort) override;
  void disconnect(bool passive) override;
private:
  Item *buffer;
  InputPort *otherPort;
};

#endif // PORT_H

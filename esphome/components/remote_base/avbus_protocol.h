#pragma once

#include "remote_base.h"

namespace esphome {
namespace remote_base {

struct AvBusData {
  uint8_t address;
  uint8_t command;

  bool operator==(const AvBusData &rhs) const { return address == rhs.address && command == rhs.command; }
};

class AvBusProtocol : public RemoteProtocol<AvBusData> {
 public:
  void encode(RemoteTransmitData *dst, const AvBusData &data) override;
  optional<AvBusData> decode(RemoteReceiveData src) override;
  void dump(const AvBusData &data) override;
};

DECLARE_REMOTE_PROTOCOL(AvBus)

template<typename... Ts> class AvBusAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t, address)
  TEMPLATABLE_VALUE(uint8_t, command)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    AvBusData data{};
    data.address = this->address_.value(x...);
    data.command = this->command_.value(x...);
    AvBusProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome

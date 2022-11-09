#include "avbus_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.nec";

static const uint32_t HEADER_US = 10000;
static const uint32_t BIT_TOTAL_US = 1000;
static const uint32_t BIT_ONE_US = 300;
static const uint32_t BIT_ONE_SPACE_US=BIT_TOTAL_US - BIT_ONE_US;
static const uint32_t BIT_ZERO_US = 700;
static const uint32_t BIT_ZERO_SPACE_US=BIT_TOTAL_US - BIT_ZERO_US;
static const uint32_t FOOTER_US = 282000;

void AvBusProtocol::encode(RemoteTransmitData *dst, const AvBusData &data) {
  dst->reserve(68);
  dst->set_carrier_frequency(0);

  dst->space(HEADER_US);

  uint8_t combinedData = (data.address << 6) | (data.command & 0x00111111);

  for (uint8_t mask = (1<<7); mask > 0; mask >>= 1) {
    if (combinedData & mask) {
      dst->item(BIT_ONE_US, BIT_ONE_SPACE_US);
    } else {
      dst->item(BIT_ZERO_US, BIT_ZERO_SPACE_US);
    }
  }

  dst->space(FOOTER_US);
}

optional<AvBusData> AvBusProtocol::decode(RemoteReceiveData src) {
  uint8_t parsedData = 0;
  for (uint8_t mask = (1 << 7); mask > 0; mask >>= 1) {
    if (src.expect_pulse_with_gap(BIT_ONE_US, BIT_ONE_SPACE_US)) {
      parsedData |= mask;
    } else if (src.expect_pulse_with_gap(BIT_ZERO_US, BIT_ZERO_SPACE_US)) {
      parsedData &= ~mask;
    } else {
      return {};
    }
  }

  AvBusData data{
    .address = (0b11000000 & parsedData) >> 6,
    .command = (0b00111111 & parsedData),
  };

  return data;
}
void AvBusProtocol::dump(const AvBusData &data) {
  ESP_LOGD(TAG, "Received AvBus: address=%1$01d, command=0x%2$02X, commandDecimal=%2$d", data.address, data.command);
}

}  // namespace remote_base
}  // namespace esphome

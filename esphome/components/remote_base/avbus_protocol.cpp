#include "avbus_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.avbus";

static const uint32_t HEADER_US = 10000;
static const uint32_t BIT_TOTAL_US = 1000;
static const uint32_t BIT_ONE_US = 300;
static const uint32_t BIT_ONE_SPACE_US = BIT_TOTAL_US - BIT_ONE_US;
static const uint32_t BIT_ZERO_US = 700;
static const uint32_t BIT_ZERO_SPACE_US = BIT_TOTAL_US - BIT_ZERO_US;
static const uint32_t FOOTER_US = 282000;

void AvBusProtocol::encode(RemoteTransmitData *dst, const AvBusData &data) {
  dst->reserve(1 + 8 * 2 + 1);
  dst->set_carrier_frequency(0);

  dst->mark(HEADER_US);

  uint8_t combinedData = (data.address << 5) | (data.command & 0b00011111);

  for (uint8_t mask = (1 << 7); mask > 0; mask >>= 1) {
    if (combinedData & mask) {
      dst->space(BIT_ONE_US);
      dst->mark(BIT_ONE_SPACE_US);
    } else {
      dst->space(BIT_ZERO_US);
      dst->mark(BIT_ZERO_SPACE_US);
    }
  }

  dst->mark(FOOTER_US);
}

optional<AvBusData> AvBusProtocol::decode(RemoteReceiveData src) {
  if (!src.expect_space(HEADER_US)) {
    return {};
  }

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

  uint8_t address = ((uint8_t) 0b11100000 & parsedData) >> 5;
  uint8_t command = ((uint8_t) 0b00011111 & parsedData);

  AvBusData data{
      .address = address,
      .command = command,
  };

  return data;
}

void AvBusProtocol::dump(const AvBusData &data) {
  ESP_LOGD(TAG, "Received AvBus: address=%d, command=0x%02X(%d)", data.address, data.command, data.command);
}

}  // namespace remote_base
}  // namespace esphome

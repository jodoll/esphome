#include "avbus_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.nec";

static const uint32_t HEADER_US = 10000;
static const uint32_t BIT_TOTAL_US = 1000;
static const uint32_t BIT_ONE_US = 300;
static const uint32_t BIT_ONE_SPACE_US = BIT_TOTAL_US - BIT_ONE_US;
static const uint32_t BIT_ZERO_US = 700;
static const uint32_t BIT_ZERO_SPACE_US = BIT_TOTAL_US - BIT_ZERO_US;
static const uint32_t FOOTER_US = 282000;

void AvBusProtocol::encode(RemoteTransmitData *dst, const AvBusData &data) {
  dst->reserve(1+8*2+1);
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
    ESP_LOGD(TAG, "Command did not start with AvBusHeader");
    ESP_LOGD(TAG, "Starts with space %d", src.peek_space_at_least(1));
    ESP_LOGD(TAG, "Length of first item: %d", src.peek());
    return {};
  }
  ESP_LOGD(TAG, "Continues with space %d", src.peek_space_at_least(1));
  ESP_LOGD(TAG, "Length of next item: %d", src.peek());

  uint8_t parsedData = 0;
  for (uint8_t mask = (1 << 7); mask > 0; mask >>= 1) {
    const uint32_t extraMarkLength = mask == 1 ? FOOTER_US : 0;
    if (src.peek_space(BIT_ONE_US) && src.peek_mark(BIT_ONE_SPACE_US + extraMarkLength, 1)) {
      parsedData |= mask;
    } else if (src.peek_space(BIT_ZERO_US) && src.peek_mark(BIT_ZERO_SPACE_US + extraMarkLength, 1)) {
      parsedData &= ~mask;
    } else {
      ESP_LOGD(TAG, "Parsing AvBus command failed with mask %02X, got so far %02X", mask, parsedData);
      return {};
    }
    src.advance(2);
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
  ESP_LOGD(TAG, "Received AvBus: address=%1$d, command=%2$02X(%2$d", data.address, data.command);
}

}  // namespace remote_base
}  // namespace esphome

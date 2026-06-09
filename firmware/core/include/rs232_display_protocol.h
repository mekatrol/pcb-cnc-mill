#ifndef RS232_DISPLAY_PROTOCOL_H
#define RS232_DISPLAY_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

enum
{
  RS232_DISPLAY_PROTOCOL_START_BYTE = 0xA5,
  RS232_DISPLAY_PROTOCOL_HEARTBEAT_FRAME_SIZE = 5,
};

typedef enum
{
  RS232_DISPLAY_PROTOCOL_MESSAGE_NONE,
  RS232_DISPLAY_PROTOCOL_MESSAGE_HEARTBEAT,
  RS232_DISPLAY_PROTOCOL_MESSAGE_CRC_ERROR,
  RS232_DISPLAY_PROTOCOL_MESSAGE_FRAME_ERROR,
} rs232_display_protocol_message_t;

typedef enum
{
  RS232_DISPLAY_PROTOCOL_RECEIVE_START,
  RS232_DISPLAY_PROTOCOL_RECEIVE_TYPE,
  RS232_DISPLAY_PROTOCOL_RECEIVE_LENGTH,
  RS232_DISPLAY_PROTOCOL_RECEIVE_PAYLOAD,
  RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_HIGH,
  RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_LOW,
} rs232_display_protocol_receive_state_t;

typedef struct
{
  rs232_display_protocol_receive_state_t receive_state;
  uint8_t message_type;
  uint8_t payload_length;
  uint8_t payload_index;
  uint16_t calculated_crc;
  uint16_t received_crc;
} rs232_display_protocol_parser_t;

void rs232_display_protocol_initialize_parser(rs232_display_protocol_parser_t *parser);
uint16_t rs232_display_protocol_crc16_update(uint16_t crc, uint8_t value);
uint8_t rs232_display_protocol_build_heartbeat_frame(uint8_t *frame, uint8_t frame_capacity);
rs232_display_protocol_message_t rs232_display_protocol_receive_byte(
    rs232_display_protocol_parser_t *parser,
    uint8_t value);

#endif // RS232_DISPLAY_PROTOCOL_H

#include "rs232_display_protocol.h"

enum
{
  RS232_DISPLAY_PROTOCOL_MESSAGE_TYPE_HEARTBEAT = 0x01,
  RS232_DISPLAY_PROTOCOL_CRC16_INITIAL = 0xFFFF,
  RS232_DISPLAY_PROTOCOL_CRC16_POLYNOMIAL = 0x1021,
};

void rs232_display_protocol_initialize_parser(rs232_display_protocol_parser_t *parser)
{
  parser->receive_state = RS232_DISPLAY_PROTOCOL_RECEIVE_START;
  parser->message_type = 0;
  parser->payload_length = 0;
  parser->payload_index = 0;
  parser->calculated_crc = RS232_DISPLAY_PROTOCOL_CRC16_INITIAL;
  parser->received_crc = 0;
}

uint16_t rs232_display_protocol_crc16_update(uint16_t crc, uint8_t value)
{
  crc ^= (uint16_t)value << 8;

  for (uint8_t bit = 0; bit < 8; bit++)
  {
    if ((crc & 0x8000u) != 0)
    {
      crc = (uint16_t)((crc << 1) ^ RS232_DISPLAY_PROTOCOL_CRC16_POLYNOMIAL);
    }
    else
    {
      crc <<= 1;
    }
  }

  return crc;
}

uint8_t rs232_display_protocol_build_heartbeat_frame(uint8_t *frame, uint8_t frame_capacity)
{
  if (frame_capacity < RS232_DISPLAY_PROTOCOL_HEARTBEAT_FRAME_SIZE)
  {
    return 0;
  }

  frame[0] = RS232_DISPLAY_PROTOCOL_START_BYTE;
  frame[1] = RS232_DISPLAY_PROTOCOL_MESSAGE_TYPE_HEARTBEAT;
  frame[2] = 0;
  uint16_t crc = rs232_display_protocol_crc16_update(RS232_DISPLAY_PROTOCOL_CRC16_INITIAL, frame[1]);
  crc = rs232_display_protocol_crc16_update(crc, frame[2]);
  frame[3] = (uint8_t)(crc >> 8);
  frame[4] = (uint8_t)crc;

  return RS232_DISPLAY_PROTOCOL_HEARTBEAT_FRAME_SIZE;
}

rs232_display_protocol_message_t rs232_display_protocol_receive_byte(
    rs232_display_protocol_parser_t *parser,
    uint8_t value)
{
  switch (parser->receive_state)
  {
  case RS232_DISPLAY_PROTOCOL_RECEIVE_START:
    if (value == RS232_DISPLAY_PROTOCOL_START_BYTE)
    {
      parser->receive_state = RS232_DISPLAY_PROTOCOL_RECEIVE_TYPE;
      parser->message_type = 0;
      parser->payload_length = 0;
      parser->payload_index = 0;
      parser->calculated_crc = RS232_DISPLAY_PROTOCOL_CRC16_INITIAL;
      parser->received_crc = 0;
    }
    return RS232_DISPLAY_PROTOCOL_MESSAGE_NONE;

  case RS232_DISPLAY_PROTOCOL_RECEIVE_TYPE:
    parser->message_type = value;
    parser->calculated_crc = rs232_display_protocol_crc16_update(parser->calculated_crc, value);
    parser->receive_state = RS232_DISPLAY_PROTOCOL_RECEIVE_LENGTH;
    return RS232_DISPLAY_PROTOCOL_MESSAGE_NONE;

  case RS232_DISPLAY_PROTOCOL_RECEIVE_LENGTH:
    parser->payload_length = value;
    parser->payload_index = 0;
    parser->calculated_crc = rs232_display_protocol_crc16_update(parser->calculated_crc, value);
    parser->receive_state = parser->payload_length == 0 ? RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_HIGH
                                                        : RS232_DISPLAY_PROTOCOL_RECEIVE_PAYLOAD;
    return RS232_DISPLAY_PROTOCOL_MESSAGE_NONE;

  case RS232_DISPLAY_PROTOCOL_RECEIVE_PAYLOAD:
    parser->calculated_crc = rs232_display_protocol_crc16_update(parser->calculated_crc, value);
    parser->payload_index++;
    if (parser->payload_index >= parser->payload_length)
    {
      parser->receive_state = RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_HIGH;
    }
    return RS232_DISPLAY_PROTOCOL_MESSAGE_NONE;

  case RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_HIGH:
    parser->received_crc = (uint16_t)value << 8;
    parser->receive_state = RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_LOW;
    return RS232_DISPLAY_PROTOCOL_MESSAGE_NONE;

  case RS232_DISPLAY_PROTOCOL_RECEIVE_CRC_LOW:
  {
    parser->received_crc |= value;
    const bool crc_matches = parser->received_crc == parser->calculated_crc;
    const uint8_t message_type = parser->message_type;

    rs232_display_protocol_initialize_parser(parser);
    if (!crc_matches)
    {
      return RS232_DISPLAY_PROTOCOL_MESSAGE_CRC_ERROR;
    }
    if (message_type == RS232_DISPLAY_PROTOCOL_MESSAGE_TYPE_HEARTBEAT)
    {
      return RS232_DISPLAY_PROTOCOL_MESSAGE_HEARTBEAT;
    }
    return RS232_DISPLAY_PROTOCOL_MESSAGE_FRAME_ERROR;
  }
  }

  rs232_display_protocol_initialize_parser(parser);
  return RS232_DISPLAY_PROTOCOL_MESSAGE_FRAME_ERROR;
}

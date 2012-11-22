/* This file is part of Equine Monitor
 *
 * Equine Monitor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Equine Monitor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Equine Monitor.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Konke Radlow <koradlow@gmail.com>
 */

#ifndef XBEE_IF
#define XBEE_IF

#include "gbee.h"
#include "messagetypes.h"
#include <string>
#include <inttypes.h>

#define XBEE_MSG_LENGTH 84
#define XBEE_ADDR_CACHE_SIZE 2

#define MSG_HEADER_LENGTH 4
/* define position of values in the header */
#define MSG_TYPE 0x00
#define MSG_PART 0x01
#define MSG_PART_CNT 0x02
#define MSG_PAYLOAD_LENGTH 0x03


enum xbee_baud_rate {
	B1200 = 0,
	B2400,
	B4800,
	B9600,
	B19200,
	B38400,
	B57600,
	B115200
};

using namespace std;

class XBee_Message;

class XBee_Address {
public:
	XBee_Address();
	XBee_Address(const string &node, uint16_t addr16, uint32_t addr64h, uint32_t addr64l);
	XBee_Address(const GBeeRxPacket *rx);
	XBee_Address(const string &node, const uint8_t *payload);

	string node;
	uint16_t addr16;
	uint32_t addr64h;
	uint32_t addr64l;
};

class XBee_Config {
public:
	XBee_Config(const string &port, const string &node, bool mode, const uint8_t *pan, uint32_t timeout,
		enum xbee_baud_rate baud, uint8_t max_unicast_hops);

	const string serial_port;
	const string node;
	const bool coordinator_mode;
	uint8_t pan_id[8];
	const uint32_t timeout;
	const enum xbee_baud_rate baud;
	const uint8_t max_unicast_hops;
};

class XBee_At_Command {
public:
	XBee_At_Command(const string &command, const uint8_t *cmd_data, uint8_t length);
	XBee_At_Command(const string &command, const string &cmd_data);
	XBee_At_Command(const string &command);
	XBee_At_Command(const XBee_At_Command &cmd);
	XBee_At_Command& operator=(const XBee_At_Command &cmd);
	~XBee_At_Command();

	void set_data(const uint8_t *data, uint8_t length, uint8_t status);
	void append_data(const uint8_t *data, uint8_t length, uint8_t status);

	string at_command;
	uint8_t *data;
	uint8_t length;
	uint8_t status;
private:

};

class XBee {
public:
	XBee(XBee_Config& config);
	virtual ~XBee();
	uint8_t xbee_init();
	uint8_t xbee_status();
	uint8_t xbee_send_at_command(XBee_At_Command& cmd);
	uint8_t xbee_send_to_coordinator(XBee_Message& msg);
	uint8_t xbee_send_to_node(XBee_Message& msg, const string &node);
	XBee_Message* xbee_receive_message();
	const XBee_Address* xbee_get_address(const string &node);
	int xbee_bytes_available();
	void xbee_test_msg();
private:
	XBee(const XBee&);
	XBee& operator=(const XBee&);
	uint8_t xbee_send(XBee_Message& msg, const XBee_Address *addr);
	uint8_t xbee_send_ackn(const XBee_Address *addr);
	uint8_t xbee_receive_acknowledge();
	uint8_t xbee_configure_device();
	uint8_t* at_cmd_str(const string at_cmd_str);

	XBee_Config config;
	XBee_Address *address_cache[XBEE_ADDR_CACHE_SIZE];
	uint8_t address_cache_size;
	GBee *gbee_handle;
};

class XBee_Message {
friend class XBee;
public:
	XBee_Message(enum MessageType type, const uint8_t *payload, uint16_t length);
	XBee_Message(const uint8_t *message);
	XBee_Message();
	XBee_Message(const XBee_Message& msg);
	XBee_Message& operator=(const XBee_Message &msg);
	~XBee_Message();
	uint8_t* get_payload(uint16_t *length);
	enum MessageType get_type();
	bool is_complete();
private:
	bool append_msg(const XBee_Message &msg);
	bool append_msg(const uint8_t *data);
	uint8_t* get_msg(uint16_t part);
	uint16_t get_msg_len(uint16_t part);
	uint8_t* allocate_msg_buffer(uint16_t payload_length);

	uint8_t *message_buffer;
	uint8_t *payload;
	enum MessageType type;
	uint16_t payload_len;
	uint8_t message_part;
	uint16_t message_part_cnt;
	bool message_complete;
};


#endif

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

#include "xbee_if.h"
#include "gbee.h"
#include "gbee-util.h"
#include <string.h>
#include <unistd.h>

/** XBee_Address Class implementation */
/* default constructor of XBee_Address, creating an empty object */
XBee_Address::XBee_Address() :
	node(""),
	addr16(0),
	addr64h(0),
	addr64l(0)
{}

/* constructor of XBee_Address */
XBee_Address::XBee_Address(const string &node, uint16_t addr16, uint32_t addr64h, uint32_t addr64l) :
	node(""),
	addr16(addr16),
	addr64h(addr64h),
	addr64l(addr64l)
{}

XBee_Address::XBee_Address(const GBeeRxPacket *rx) :
	node("")
{
	addr16 = 	GBEE_USHORT(rx->srcAddr16);
	addr64h = 	GBEE_ULONG(rx->srcAddr64h);
	addr64l = 	GBEE_ULONG(rx->srcAddr64l);
}

/* constructor that decodes the data returned as an reply to the AT "DN"
 * command by an XBee device */
XBee_Address::XBee_Address(const string &node, const uint8_t *payload) :
	node(node),
	addr16(0),
	addr64h(0),
	addr64l(0)
{
	/* the response to a Destination Node discovery command is a message
	 * with a length of 10 bytes, where the first 2 bytes define the
	 * 16-bit address, bytes 2-5 the high part of the 64-bit address
	 * and bytes 6-9 the low part of the 64-bit address.
	 * The addresses are encoded as hex characters transmitted in big-endian */
	addr16 = (uint8_t)payload[0] << 8 | (uint8_t)payload[1];
	for (int i = 0; i <= 3; i++) {
		addr64h |= (uint8_t)payload[i+2] << (3-i)*8;
		addr64l |= (uint8_t)payload[i+6] << (3-i)*8;
	}
}

/** XBee_Config Class implementation */
/* constructor of the XBee_config class, which is used to provide access
 * to configuration options. It is a raw data container at the moment */
 /* TODO: find a good way to set baud rate for xbees */
XBee_Config::XBee_Config(const string &port, const string &node, bool mode,
			const uint8_t *pan, uint32_t timeout,
			enum xbee_baud_rate baud, uint8_t max_unicast_hops):
		serial_port(port),
		node(node),
		coordinator_mode(mode),
		timeout(timeout),
		baud(baud),
		max_unicast_hops(max_unicast_hops)
{
	memcpy(pan_id, pan, 8);
}

/** XBee_At_Command Class implementation */
/* constructs a XBee_At_Command object, by copying the given values into
 * an internal memory space */
XBee_At_Command::XBee_At_Command(const string &command, const uint8_t *cmd_data, uint8_t cmd_length) :
		at_command(command),
		length(cmd_length),
		status(0x00)
{
	data = new uint8_t[cmd_length];
	memcpy(data, cmd_data, cmd_length);
}

/* constructs a XBee_At_Command object, by translating the command string into
 * a byte array and copying it an internal memory space */
XBee_At_Command::XBee_At_Command(const string &command, const string &cmd_data) :
		at_command(command),
		length(cmd_data.length()),
		status(0x00)
{
	data = new uint8_t[length];
	memcpy(data, cmd_data.c_str(), length);
}

/* constructs a XBee_At_Command object that is empty except for the actual
 * AT command, and can be used to request values */
XBee_At_Command::XBee_At_Command(const string &command) :
		at_command(command),
		data(NULL),
		length(0),
		status(0x00)
{}

/* copy constructor, performs a deep copy */
XBee_At_Command::XBee_At_Command(const XBee_At_Command &cmd) :
		at_command(cmd.at_command),
		length(cmd.length),
		status(cmd.status)
{
	data = new uint8_t[length];
	memcpy(data, cmd.data, length);
}

/* assignment operator, performs deep copy for pointer members */
XBee_At_Command& XBee_At_Command::operator=(const XBee_At_Command &cmd) {
	at_command = cmd.at_command;
	length = cmd.length;
	status = cmd.status;

	/* free locally allocated memory, and copy memory content from cmd.data
	 * address into new allocated memory space */
	if (data)
		delete[] data;
	data = new uint8_t[length];
	memcpy(data, cmd.data, length);

	return *this;
}

XBee_At_Command::~XBee_At_Command() {
	if (data)
		delete[] data;
}

/* frees the memory space allocated for the data, and copies the given
 * data into the object by allocating new memory space */
void XBee_At_Command::set_data(const uint8_t *cmd_data, uint8_t cmd_length, uint8_t cmd_status) {
	if (data)
		delete[] data;
	length = cmd_length;
	data = new uint8_t[cmd_length];
	memcpy(data, cmd_data, cmd_length);
	status = cmd_status;
}

/* appends the data chunk to the existing memory space in the object.
 * This is used for AT commands with a multi frame reply */
void XBee_At_Command::append_data(const uint8_t *new_data, uint8_t cmd_length, uint8_t cmd_status) {
	uint8_t *old_data = data;

	status = cmd_status;
	/* allocate new memory, big enough to contain the existing data and
	 * the additional new data, and copy the old data to the new memory space */
	data = new uint8_t[length + cmd_length];
	memcpy(data, old_data, length);

	/* append the new_data to the existing data, and update the length
	 * of the data field */
	memcpy(&data[length], new_data, cmd_length);
	length += cmd_length;

	/* free the old memory space */
	if (old_data)
		delete[] old_data;
}

/** XBee_Message Class implementation */
/* constructor for a XBee message - used to create messages for transmission */
// TODO: Make message part in Header 2 bytes long
XBee_Message::XBee_Message(enum xbee_msg_type type, const uint8_t *msg_payload, uint16_t msg_length):
		type(type),
		payload_len(msg_length),
		message_part(1),	/* message part numbers start with 1 */
		message_complete(true)	/* messages created by this constructor
					 * are complete at construction time */
{
	/* calculate the number of parts required to transmit this message */
	message_part_cnt = payload_len / (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH) + 1;
	if (message_part_cnt > 255)
		printf("Error: Message size > 20kB not supported\n");
	/* allocate memory to copy the payload into the object */
	payload = new uint8_t[payload_len];
	memcpy(payload, msg_payload, payload_len);
	/* allocate memory for the message buffer */
	message_buffer = allocate_msg_buffer(payload_len);
}

/* constructor for XBee_messages - used to deserialize objects after reception */
XBee_Message::XBee_Message(const uint8_t *message):
		message_buffer(NULL),	/* this message type will not use the buffer */
		type(static_cast<xbee_msg_type>(message[MSG_TYPE])),
		payload_len(message[MSG_PAYLOAD_LENGTH]),
		message_part(message[MSG_PART]),
		message_part_cnt(message[MSG_PART_CNT])
{
	/* allocate memory to copy the payload into the object */
	payload = new uint8_t[payload_len];
	memcpy(payload, &message[MSG_HEADER_LENGTH], payload_len);

	/* determine if the message is complete, or just a part of a longer
	 * message */
	if (message_part_cnt == 1)
		message_complete = true;
	else
		message_complete = false;
}

/* constructor for a empty XBee message - used to create messages for appending data */
XBee_Message::XBee_Message():
	message_buffer(NULL),
	payload(NULL),
	payload_len(0),
	message_part(0),
	message_part_cnt(0),
	message_complete(false)
{}

/* copy constructor, performs a deep copy */
XBee_Message::XBee_Message(const XBee_Message& msg) :
	message_buffer(NULL),
	type(msg.type),
	payload_len(msg.payload_len),
	message_part(msg.message_part),
	message_part_cnt(msg.message_part_cnt),
	message_complete(msg.message_complete)
{
	/* allocate memory space for the payload and copy the data from msg */
	payload = new uint8_t[payload_len];
	memcpy(payload, msg.payload, payload_len);
	/* allocate memory for the message buffer, if there was memory allocated
	 * in the source object */
	if (msg.message_buffer)
		message_buffer = allocate_msg_buffer(payload_len);
}

/* assignment operator, performs deep copy for pointer members */
XBee_Message& XBee_Message::operator=(const XBee_Message& msg) {
	payload_len = msg.payload_len;
	message_part = msg.message_part;
	message_part_cnt = msg.message_part_cnt;
	message_complete = msg.message_complete;

	/* take care of pointer members */
	/* if memory was allocated in the object, free the memory */
	if (payload)
		delete[] payload;
	if (message_buffer)
		delete[] message_buffer;

	/* allocate memory space for the payload and copy the data from msg */
	payload = new uint8_t[payload_len];
	memcpy(payload, msg.payload, payload_len);
	/* allocate memory for the message buffer, if there was memory allocated
	 * in the source object */
	if (msg.message_buffer)
		message_buffer = allocate_msg_buffer(payload_len);

	return *this;
}

XBee_Message::~XBee_Message() {
	if (payload)
		delete[] payload;
	if (message_buffer)
		delete[] message_buffer;
}

uint8_t* XBee_Message::get_payload(uint16_t *length) {
	*length = payload_len;
	return payload;
}

enum xbee_msg_type XBee_Message::get_type() {
	return type;
}

bool XBee_Message::is_complete() {
	return message_complete;
}

/* reconstructs messages that consist of multiple parts, returns true if
 * the message was successfully appended and false, if the operation failed
 * due to failed validity check */
bool XBee_Message::append_msg(const XBee_Message &msg) {
	uint8_t *new_payload;
	uint16_t new_payload_len;

	/* check if it's possible to append the given message */
	if (msg.message_part != message_part+1)
		return false;

	/* if it's the first part of a message, copy the total part count */
	if (msg.message_part == 1)
		message_part_cnt = msg.message_part_cnt;

	/* given message passed validity check -> allocate memory */
	new_payload_len = payload_len + msg.payload_len;
	new_payload = new uint8_t[new_payload_len];

	/* copy the existing payload into the new memory space */
	memcpy(new_payload, payload, payload_len);
	/* append the new payload to the memory */
	memcpy(&new_payload[payload_len], msg.payload, msg.payload_len);

	/* update internal variables to match new data */
	if (payload)
		delete[] payload;
	payload = new_payload;
	payload_len += msg.payload_len;
	message_part += 1;
	type = msg.type;

	/* determine if the message is complete */
	if (message_part == message_part_cnt) {
		printf("Complete message received \n");
		message_complete = true;
	}

	return true;
}

bool XBee_Message::append_msg(const uint8_t *data) {
	XBee_Message tmp_msg(data);
	return append_msg(tmp_msg);
}

/* returns a pointer to a message buffer that includes a header and a payload.
 * The message_buffer is constructed on the fly into a preallocated and fixed
 * memory space.
 * The content of the memory space is overwritten each time this function is called */
uint8_t* XBee_Message::get_msg(uint16_t part = 1) {
	uint16_t length = payload_len;
	uint16_t overhead_len;
	uint16_t offset = 0;

	/* check if memory was allocated for the message_buffer. Because
	 * we're working on a machine with limited memory, there is a case
	 * when no memory is allocated for the buffer during object instantiation
	 * (de-serializing object from byte stream) because these objects are only used
	 * to put received message back together */
	if (!message_buffer)
		return NULL;

	if (message_part_cnt > 1) {
		/* calculate the length of the payload in last message part */
		overhead_len = length - (message_part_cnt - 1) * (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH);
		/* payload length depends on the part number of the message ->
		 * last message part is an exception */
		length = (part == message_part_cnt)? overhead_len : (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH);
		/* offset in the payload data based on message part */
		offset = (part - 1) * (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH);
	}
	/* create the header of the message */
	message_buffer[MSG_TYPE] = static_cast<uint8_t>(type);
	message_buffer[MSG_PART] = part;
	message_buffer[MSG_PART_CNT] = message_part_cnt;
	message_buffer[MSG_PAYLOAD_LENGTH] = length;
	/* copy payload into message body */
	memcpy(&message_buffer[MSG_HEADER_LENGTH], &payload[offset], length);

	return message_buffer;
}

/* returns the length of the requested part of the message (including header) */
uint16_t XBee_Message::get_msg_len(uint16_t part = 1) {
	/* check if memory was allocated for the message_buffer, and see
	 * get_msg function for explanation */
	if (!message_buffer)
		return 0;

	/* message consists of one part? */
	if (message_part_cnt == 1)
		return (MSG_HEADER_LENGTH + payload_len);

	/* message consists of multiple parts, part in the middle requested.
	 * Parts in the middle always have the maximal possible message length
	 * to make best use of bandwidth */
	if (message_part_cnt != part)
		return XBEE_MSG_LENGTH;

	/* message consists of multiple parts, last part requested */
	uint16_t transmitted_len = (message_part_cnt - 1) * (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH);
	return MSG_HEADER_LENGTH + payload_len - transmitted_len;
}

/* allocates memory in for the message buffer in a XBee_Message object.
 * The size of the memory depends on the the fact if the payload will fit
 * into one transmission or has to be split up */
uint8_t* XBee_Message::allocate_msg_buffer(uint16_t payload_len) {
	uint8_t *message_buffer;
	uint16_t msg_part_cnt;

	/* calculate the number of parts required to transmit this message */
	msg_part_cnt = payload_len / (XBEE_MSG_LENGTH - MSG_HEADER_LENGTH) + 1;
	/* allocate memory for the message buffer */
	if (msg_part_cnt > 1) {
		/* message has to be split into multiple parts, but each
		 * single part will not be larger thatn the maximal msg lengh */
		message_buffer = new uint8_t[XBEE_MSG_LENGTH];
	} else {
		/* message fits into one transmission */
		message_buffer = new uint8_t[payload_len + MSG_HEADER_LENGTH];
	}

	return message_buffer;
}

/** XBee Class implementation */
XBee::XBee(XBee_Config& config) :
	config(config),
	address_cache_size(0)
{}

XBee::~XBee() {
	gbeeDestroy(gbee_handle);
	for (int i = 0; i < address_cache_size; i++)
		delete address_cache[i];
}

/* the init function initializes the internally used libgbee library by creating
 * a handle for the xbee device */
uint8_t XBee::xbee_init() {
	gbee_handle = gbeeCreate(config.serial_port.c_str());
	if (!gbee_handle) {
		printf("Error creating handle for XBee device\n");
		exit(-1);
	}

	/* TODO: check if device is operating in API mode: the functions provided by
	 * libgbee (gbeeGetMode, gbeeSetMode) cannot be used, because they rely
	 * on the AT mode of the devices which is not working with the current
	 * Firmware version */
	return xbee_configure_device();
}

/* the configure device function sets the basic parameters for the XBee modules,
 * according to the values found in the XBee_Config object.
 * It will read the register values from the device and compare them with the
 * desired values, updating them if a mismatch is detected. If a register was
 * updated the changes are written into the nonvolatile memory of the device */
 // TODO: Configure Sleep Mode for Coordinator / End Devices
uint8_t XBee::xbee_configure_device() {
	uint8_t error_code;
	bool register_updated = false;

	printf("Validating device configuration \n");

	/* check the 64bit PAN ID */
	XBee_At_Command cmd("ID");
	error_code = xbee_send_at_command(cmd);
	if (error_code != GBEE_NO_ERROR)
		return error_code;
	if (memcmp(cmd.data, config.pan_id, 8)) {
		printf("Setting PAN ID\n");
		XBee_At_Command cmd_pan("ID", config.pan_id, 8);
		xbee_send_at_command(cmd_pan);
		register_updated = true;
	}

	/* check the Node Identifier */
	cmd = XBee_At_Command("NI");
	error_code = xbee_send_at_command(cmd);
	if (error_code != GBEE_NO_ERROR)
		return error_code;
	if (memcmp(cmd.data, config.node.c_str(), config.node.length())) {
		printf("Setting Node Identifier\n");
		XBee_At_Command cmd_ni("NI", config.node);
		xbee_send_at_command(cmd_ni);
		register_updated = true;
	}

	/* check the Maximum Unicast Hops value */
	cmd = XBee_At_Command("NH");
	error_code = xbee_send_at_command(cmd);
	if (error_code != GBEE_NO_ERROR)
		return error_code;
	/* NH returns 1 byte, with a range of 0x00 - 0xFF. Value defines the
	 * unicast timeout: 50*NH + 100ms */
	if (cmd.data[0] != config.max_unicast_hops) {
		printf("Setting Unicast Hops from %02x to %02x\n",cmd.data[0], config.max_unicast_hops);
		XBee_At_Command cmd_nh("NH", &config.max_unicast_hops, 1);
		xbee_send_at_command(cmd_nh);
		register_updated = true;
	}

	/* check the Baud Rate */
	cmd = XBee_At_Command("BD");
	error_code = xbee_send_at_command(cmd);
	if (error_code != GBEE_NO_ERROR)
		return error_code;
	/* BD returns 4 bytes, this program only supports predefined baud rates,
	 * which have a range from 0-7 and are found in the last byte */
	if (cmd.data[3] != (uint8_t)config.baud) {
		printf("Setting Baud Rate from %02x to %02x\n",cmd.data[0], (uint8_t)config.baud);
		XBee_At_Command cmd_bd("BD", (const uint8_t*)&config.baud, 1);
		xbee_send_at_command(cmd_bd);
		register_updated = true;
	}

	if (register_updated) {
		/* write the changes to the internal memory of the xbee module */
		cmd = XBee_At_Command("WR");
		error_code = xbee_send_at_command(cmd);
		if (error_code != GBEE_NO_ERROR)
			return error_code;

		/* apply the queued changes */
		cmd = XBee_At_Command("AC");
		error_code = xbee_send_at_command(cmd);
		if (error_code != GBEE_NO_ERROR)
			return error_code;
	}
	return GBEE_NO_ERROR;
}

/* xbee_status requests, decodes and prints the current status of the XBee module */
uint8_t XBee::xbee_status() {
	GBeeFrameData *frame;
	GBeeError error_code;
	uint16_t length;
	uint32_t timeout = config.timeout;
	uint8_t frame_id = 3;	/* used to identify response frame */
	uint8_t status = 0xFE;	/* Unknown Status */

	/* query the current network status and print the response in cleartext */
	error_code = gbeeSendAtCommand(gbee_handle, frame_id, at_cmd_str("AI"), NULL, 0);
	if (error_code != GBEE_NO_ERROR) {
		printf("Error requesting XBee status: %s\n", gbeeUtilCodeToString(error_code));
		return status;
	}
	/* wait for the response to the command */
	frame = new GBeeFrameData;
	error_code = gbeeReceive(gbee_handle, frame, &length, &timeout);
	if (error_code != GBEE_NO_ERROR)
		return error_code;
	if (frame->ident == GBEE_AT_COMMAND_RESPONSE) {
		GBeeAtCommandResponse *at_frame = (GBeeAtCommandResponse*) frame;
		status = at_frame->value[0];
		printf("Status: %s\n", gbeeUtilStatusCodeToString(status));
	}
	delete frame;

	return status;
}

/* sends out the requested AT command, receives & stores the register value
 * in the XBee_At_Command object */
uint8_t XBee::xbee_send_at_command(XBee_At_Command& cmd){
	GBeeFrameData *frame;
	GBeeError error_code;
	uint8_t response_cnt = 0;
	uint16_t length = 0;
	uint32_t timeout = config.timeout;
	static uint8_t frame_id;

	/* send the AT command & data to the device */
	frame_id = (frame_id % 255) + 1;	/* give each frame a unique ID */
	error_code = gbeeSendAtCommand(gbee_handle, frame_id, at_cmd_str(cmd.at_command), cmd.data, cmd.length);
	if (error_code != GBEE_NO_ERROR) {
		printf("Error sending XBee AT (%s) command : %s\n", cmd.at_command.c_str(),
		gbeeUtilCodeToString(error_code));
		return error_code;
	}
	/* try to receive the response and copy it into the XBee_At_Command object*/
	frame = new GBeeFrameData;
	while (1) {
		memset(frame, 0, sizeof(*frame));
		error_code = gbeeReceive(gbee_handle, frame, &length, &timeout);
		if (error_code != GBEE_NO_ERROR)
			break;

		/* check if the received frame is a AT Command Response frame */
		if (frame->ident == GBEE_AT_COMMAND_RESPONSE) {
			GBeeAtCommandResponse *at_frame = (GBeeAtCommandResponse*) frame;
			if (at_frame->frameId != frame_id) {
				printf("Problem: Frame IDs not matching\n (%i : %i)\n",
				frame_id, at_frame->frameId);
				error_code = GBEE_RESPONSE_ERROR;
				/* if the frameId is larger than expected nothing can be done */
				if (frame_id < at_frame->frameId)
					break;
				/* if it's smaller, wait for a second and try again */
                        	sleep(1);
				delete frame;
				continue;
			}
			/* copy the response payload into the XBee_At_Command object.
			 * This frame type has an overhead of 5 bytes that are counted
			 * as part of the length. */
			if (response_cnt++ < 1)
				cmd.set_data(at_frame->value, length - 5, at_frame->status);
			else
				cmd.append_data(at_frame->value, length - 5, at_frame->status);
			break;
		/* Modem Status frames can be transmitted at arbitrary times,
		 * as long as there's no message queue, we have to handle them here */
		} else if (frame->ident == GBEE_MODEM_STATUS) {
			GBeeModemStatus *status_frame = (GBeeModemStatus*) frame;
			printf("Received Modem status: %02x\n", status_frame->status);
		} else {
			printf("Received frame with ident %02x\n", frame->ident);
			break;
		}
	}

	/* in case there was an unexpected error, the function returned early
	 * the only way we get this far, is if everything worked*/
	delete frame;
	return error_code;
}

/* sends the data in the message object to the coordinator */
uint8_t XBee::xbee_send_to_coordinator(XBee_Message& msg) {
	/* coordinator can be addressed by setting the 64bit destination
	 * address to all zeros and the 16bit address to 0xFFFE */
	XBee_Address addr;
	addr.addr16 = 0xFFFE;
	return xbee_send(msg, &addr);
}

/* sends the data in the message object to a Network Node */
uint8_t XBee::xbee_send_to_node(XBee_Message& msg, const string &node) {
	const XBee_Address *addr = xbee_get_address(node);
	if (!addr)
		return GBEE_TIMEOUT_ERROR;	/* node couldn't be found in network */
	return xbee_send(msg, addr);
}

/* checks the buffer for (parts of) messages, puts together a complete message
 * from the parts */
XBee_Message* XBee::xbee_receive_message() {
	GBeeError error_code;
	GBeeFrameData *frame = new GBeeFrameData;
	XBee_Message *msg = new XBee_Message;
	XBee_Address src_addr;
	uint16_t length = 0;
	uint32_t timeout = config.timeout;

	/* try to receive a message, it might consist of several parts */
	uint8_t retry_cnt = 3;
	do {
		while (--retry_cnt > 0 && !msg->is_complete()) {
			memset(frame, 0, sizeof(*frame));
			error_code = gbeeReceive(gbee_handle, frame, &length, &timeout);
			if (error_code != GBEE_NO_ERROR) {
				printf("Error receiving message: length=%d, error= %s\n",
				length, gbeeUtilCodeToString(error_code));
				continue;
			}
			/* check if the received frame is a RxPacket frame */
			if (frame->ident == GBEE_RX_PACKET) {
				GBeeRxPacket *rx_frame = (GBeeRxPacket*) frame;
				/* try to append the data to the message
				 * if it fails, give up and assume an faulty transmission */
				if (!msg->append_msg(rx_frame->data))
					retry_cnt = 0;
				else
					retry_cnt = 3;
				break;
			}
			else {
				printf("Received unexpected message frame: ident=%02x\n",frame->ident);
			}
		}
	} while (!msg->is_complete() && retry_cnt > 0);

	delete frame;
	return msg;
}

/* returns a reference to an address object, that contains the current network
 * address of the node identified by the string */
const XBee_Address* XBee::xbee_get_address(const string &node) {
	uint8_t error_code;

	/* check for cached addresses */
	for (int i = 0; i < address_cache_size; i++) {
		if (address_cache[i]->node == node) {
			return address_cache[i];
		}
	}
	/* address not cached -> do a destination node lookup */
	XBee_At_Command cmd("DN", node);
	error_code = xbee_send_at_command(cmd);
	if (error_code != GBEE_NO_ERROR) {
		printf("Node discovery failed, error: %s\n", gbeeUtilCodeToString((gbeeError)error_code));
		return NULL;
	}
	/* decode the returned data and add the address to the cache */
	XBee_Address *new_address = new XBee_Address(node, cmd.data);
	address_cache[address_cache_size++] = new_address;

	return new_address;
}

/* checks the buffer of the serial device for available data, and returns the
 * number of pending bytes */
int XBee::xbee_bytes_available() {
  return 0;
}

uint8_t XBee::xbee_send(XBee_Message& msg, const XBee_Address *addr) {
	GBeeFrameData *frame = new GBeeFrameData;
	GBeeError error_code;
	const uint8_t bcast_radius = 0;	/* -> max hops for bcast transmission */
	const uint8_t options = 0x00;	/* 0x01 = Disable ACK, 0x20 - Enable APS
				 * encryption (if EE=1), 0x04 = Send packet
				 * with Broadcast Pan ID.
				 * All other bits must be set to 0. */
	uint8_t tx_status = 0xFF;	/* -> Unknown Tx Status */
	uint16_t length;
	uint32_t timeout = config.timeout;

	/* send the message, by splitting it up into parts that have the
	 * correct length for transmission over ZigBee */
	for (uint16_t i = 1; i <= msg.message_part_cnt; i++) {
		/* send out one part of the message, use the message part as frame id */
		error_code = gbeeSendTxRequest(gbee_handle, i, addr->addr64h, addr->addr64l,
		addr->addr16, bcast_radius, options, msg.get_msg(i), msg.get_msg_len(i));
		printf("Sending message part %u of %u with length %u\n",
			i, msg.message_part_cnt, msg.get_msg_len(i));
		if (error_code != GBEE_NO_ERROR) {
			printf("Error sending message part %u of %u: %s\n",
			i, msg.message_part_cnt, gbeeUtilCodeToString(error_code));
			tx_status = 0xFF;	/* -> Unknown Tx Status */
			break;
		}

		/* wait for the acknowledgement for the message frame */
		uint8_t retry_cnt = 3;
		while (--retry_cnt > 0) {
			memset(frame, 0, sizeof(*frame));
			error_code = gbeeReceive(gbee_handle, frame, &length, &timeout);
			if (error_code != GBEE_NO_ERROR) {
				printf("Error receiving transmission status, status message: error= %s\n",
				gbeeUtilCodeToString(error_code));
				tx_status = 0xFF;	/* -> Unknown Tx Status */
			/* check if the received frame is a TxStatus frame */
			} else if (frame->ident == GBEE_TX_STATUS_NEW) {
				GBeeTxStatusNew *tx_frame = (GBeeTxStatusNew*) frame;
				tx_status = tx_frame->deliveryStatus;
				if (tx_status != 0x00)	/* 0x00 = success */
					continue;
				break;
			}
		}
		/* reset timeout to initial value (it's modified by gbeeReceive fckt */
		timeout = config.timeout;
		if (retry_cnt == 0)
			break;
	}
	delete frame;
	return tx_status;
}

/* converts a string into a ASCII coded byte array - the length of
 * the byte array is fixed to a length of an AT command (2 chars) */
uint8_t* XBee::at_cmd_str(const string at_cmd_str) {
	static uint8_t at_cmd[2];
	memcpy(at_cmd, at_cmd_str.c_str(), 2);
	return &at_cmd[0];
}


#define DTA_SIZE 400
void XBee::xbee_test_msg() {

}

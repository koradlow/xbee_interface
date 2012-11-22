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
#include <gbee.h>
#include <gbee-util.h>
#include <array> 
#include <unistd.h>
#include <sys/time.h>

const char* hex_str(uint8_t *data, uint8_t length);
XBee_Message get_message(uint16_t size);
void speed_measurement(XBee* interface, uint16_t size, uint8_t iterations);

int main(int argc, char **argv) {
	uint8_t pan_id[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xAB, 0xBC, 0xCD};
	uint8_t unique_id = 2;
	uint8_t error_code;
	XBee_Config config("/dev/ttyUSB0", "denver", false, pan_id, 500, B115200, 1);
	
	XBee interface(config);
	error_code = interface.xbee_init();
	if (error_code != GBEE_NO_ERROR) {
		printf("Error: unable to configure device, code: %02x\n", error_code);
		return 0;
	}
	interface.xbee_status();
	speed_measurement(&interface, 300, 20);
	
	/*
	XBee_Message *rcv_msg = NULL;
	uint16_t length = 0;
	uint8_t *payload;
	while (true) {
		if (interface.xbee_bytes_available()) {
			rcv_msg = interface.xbee_receive_message();
			if ( rcv_msg->is_complete()) {
				rcv_msg->get_payload(&length);
				printf("%s msg received. Type: %u, length: %u\n",
				rcv_msg->is_complete() ? "complete" : "incomplete",
				rcv_msg->get_type(), length);
				payload = rcv_msg->get_payload(&length);
				printf("content: %s\n", hex_str(payload, length));
			}
			delete rcv_msg;
		}
	}
	*/
	/* print some information about the current network status */
	XBee_At_Command cmd("MY");
	interface.xbee_send_at_command(cmd);
	printf("%s: %s\n", cmd.at_command.c_str(), hex_str(cmd.data, cmd.length));
	cmd = XBee_At_Command("SH");
	interface.xbee_send_at_command(cmd);
	printf("%s: %s\n", cmd.at_command.c_str(), hex_str(cmd.data, cmd.length));
	cmd = XBee_At_Command("SL");
	interface.xbee_send_at_command(cmd);
	printf("%s: %s\n", cmd.at_command.c_str(), hex_str(cmd.data, cmd.length));
	
	return 0;
}

const char* hex_str(uint8_t *data, uint8_t length) {
	static char c_str[80];
	for (int i = 0; i < length; i++)
		sprintf(&c_str[i*3], "%02x ", data[i]);
	return c_str;
}

XBee_Message get_message(uint16_t size) {
	uint8_t *payload = new uint8_t[size];

	for (int i = 0; i < size; i++) {
		payload[i] = (uint8_t)i % 255;
	}
	XBee_Message test_msg(TEST, payload, size);
	delete[] payload;

	return test_msg; 
}

void speed_measurement(XBee* interface, uint16_t size, uint8_t iterations) {
	struct timeval start, end;
	long mtime, seconds, useconds;
	uint8_t error_code;  
	XBee_Message test_msg = get_message(size);

	gettimeofday(&start, NULL);
	for (int i = 0; i < iterations; i++) {
		test_msg = get_message(size);
		if ((error_code = interface->xbee_send_to_node(test_msg, "coordinator"))!= 0x00) {
			printf("Error transmitting: %u\n", error_code);
			break;
		}
		printf("Successfully transmitted msg %u with type %02x\n", i+1, (uint8_t)test_msg.get_type());
	}
	gettimeofday(&end, NULL);

	seconds  = end.tv_sec  - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	printf("Elapsed time: %ld milliseconds\n", mtime);
	printf("Data throughput: %.1f B/s\n", (iterations * size * 1000) / (float) mtime);
}


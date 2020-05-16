/* Art-Net protocol for rpi-rgb-led-matrix
 * https://github.com/hzeller/rpi-rgb-led-matrix
 * https://github.com/darknessii/rpi-matrix-artnet.git
 * Transcription from Python to C, by Louis Croisez.
 * License: MIT */

#include "led-matrix-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <math.h>

#define ARTNET_WIDTH 64
#define ARTNET_HEIGHT 32
#define HORIZ_NUMBER_PANELS 1
#define VERT_NUMBER_PANELS 1
#define UNIVERSE_LEN 170
#define UNIVERSE_COUNT 13

struct RGBLedMatrixOptions options;
struct RGBLedMatrix *matrix;
struct LedCanvas *offscreen_canvas;

volatile int terminating = 0; 
void terminate(int sign) { 
	printf("Exiting\n");
	led_matrix_delete(matrix);
	exit(0);
}

int main(int argc, char **argv)
{
	int width, height;
	int x, y, i;
	int x_start[UNIVERSE_COUNT];
	int y_start[UNIVERSE_COUNT];

	memset(&options, 0, sizeof(options));
	options.cols = ARTNET_WIDTH;
	options.rows = ARTNET_HEIGHT;
	options.chain_length = HORIZ_NUMBER_PANELS;
	options.parallel = VERT_NUMBER_PANELS;
	options.multiplexing = 0;
  	options.row_address_type = 0;
  	options.brightness = 100;
  	options.hardware_mapping = "regular";
  	options.inverse_colors = 0;
  	options.led_rgb_sequence = "RGB";
  	options.pwm_lsb_nanoseconds = 150;
  	options.show_refresh_rate = 0;
  	options.disable_hardware_pulsing = 1;
  	options.scan_mode = 0;
  	options.pwm_bits = 11;

	matrix = led_matrix_create_from_options(&options, &argc, &argv);
	if (matrix == NULL) { printf("Error creating matrix object\n"); return 1; }

	printf("Use --led-help command line switch to get all available options.\n");

	offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);
	led_canvas_get_size(offscreen_canvas, &width, &height);

	x_start[0] = 0; y_start[0] = 0;
	for (int u=1; u < UNIVERSE_COUNT; u++) {
		int rest = UNIVERSE_LEN - ARTNET_WIDTH + x_start[u-1];
		int restmod = trunc(rest / ARTNET_WIDTH);
		x_start[u] = rest - restmod * ARTNET_WIDTH;
		y_start[u] = y_start[u-1] + restmod + 1;
	}

	printf("Launching of Artnet to LED matrix adaptation layer\n");

	int sock, length, n;
	socklen_t fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buf[1024];
	int  lastSequence = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) { printf("Error opening socket\n"); return 1; }
	length = sizeof(server);
	bzero(&server, length);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(6454);
	if (bind(sock, (struct sockaddr *)&server, length) < 0) { printf("Error binding socket\n"); return 1; }
	fromlen = sizeof(struct sockaddr_in);
	
	printf("Listening on port UDP/6454\n");
	printf("Hit Ctrl-C for clean exit\n");
	
	signal(SIGINT, terminate);
	
	while (1) 
	{
		n = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&from, &fromlen);
		if ((n > 18) && (strncmp(buf, "Art-Net\x00", 8) == 0)) {
			int opcode = buf[8] + (buf[9]<<8);
			int protocolVersion = (buf[10]<<8) + buf[11];
			if ((opcode == 0x5000) && (protocolVersion >= 14)){
				char sequence = buf[12];
				//char physical = buf[13];
				//char net      = buf[15];
				//char sub_net  = (buf[14] & 0xF0) >> 4;
				int universe = (int)(buf[14] & 0x0F);
				int  rgb_length = (buf[16]<<8) + buf[17];

				//Sequence is complete, let us display it
				if (lastSequence != sequence) {
					offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
					lastSequence = sequence;
				}

				x = x_start[universe];
				y = y_start[universe];

				//printf("%d (%d.%d.%d.%d) len=%d x=%d y=%d\n", 
				//	sequence, physical, net, sub_net, universe, rgb_length, x, y);

				for (i=0; i < rgb_length/3; i++) {
					char R = buf[18 + 3*i];
					char G = buf[18 + 3*i + 1];
					char B = buf[18 + 3*i + 2];
					led_canvas_set_pixel(offscreen_canvas, x, y, R, G, B);
					if (x < width-1) {x++;} else { x=0; y++; }
				}
			}
		}
	}

	return 0;
}


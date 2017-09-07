/*  
 *  Simple http_server_client
 *  @author: Austin Payne
 */
SYSTEM_MODE(SEMI_AUTOMATIC); // don't connect to Spark cloud until Particle.connect()
SYSTEM_THREAD(ENABLED); // run spark procs and application procs in parrallel
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COORD_VALID(c) ((c) < 8)

TCPServer server = TCPServer(80);
TCPClient server_client;
TCPClient board_client;

void init_serial(void) {
    Serial.begin(9600);
    Serial1.begin(9600);
}

/*
 *  setup runs first (in semi-automatic or manual mode)
 */
void setup()
{
    init_serial();
    WiFi.on(); // needed when in semi-automatic mode
    WiFi.connect();
    waitFor(WiFi.ready, 10000);
    if (!WiFi.ready) {
        Serial.println("could not connect to wifi!\n");
        Serial.println("stored networks:");
        Serial.println("ssid\tsecurity\tcipher\t");
        WiFiAccessPoint ap[5];
        int found = WiFi.getCredentials(ap, 5);
        for (int i = 0; i < found; i++) {
            Serial.printf("%s\t%s\t%s\n"ap[i].ssid, ap[i].security, ap[i].cipher);
        }
    } else {
        Serial.println("system ready");
    }
    
    server.begin();
}

/*
 *  validates that move is of the form:
 *      [a-h][1-8][a-h][1-8]
 */
bool valid_move(uint8_t *move, size_t size) {
    if (size == 4) {
        unsigned char src_x, src_y, dst_x, dst_y;
        src_x = move[0]-'a';
	    src_y = move[1]-'1';
	    dst_x = move[2]-'a';
	    dst_y = move[3]-'1';
	    return (COORD_VALID(src_x) && COORD_VALID(src_y) && COORD_VALID(dst_x) && COORD_VALID(dst_y));
	    return false;
    }
    
    return false;
}

/*
 *  main loop
 */
void loop()
{
    // chess server has sent a move for the board, get it and forward to board
    if (server_client.connected()) {
        while (server_client.available()) {
            uint8_t temp_buffer[4];
            int n;
            n = server_client.read(temp_buffer, 4);
            if (n == 4) {
                if (valid_move(temp_buffer, n)) {
                    server.println("ok");
                    Serial1.printf("%s", temp_buffer);
	            } else {
	                server.println("err");
	            }
            }
        }
    } 
    // otherwise, check if board has sent a player move and forward to chess server
    else {
        uint8_t temp_buffer[4];
        int i = 0;
        while (Serial1.available()) {
            if (i < 4) {
                int temp = Serial1.read();
                temp_buffer[i] = temp;
                i++;
            } else if (valid_move(temp_buffer, i)) {
                if (board_client.connect({192, 168, 33, 14}, 80))
                    board_client.write(temp_buffer, 4);
            }
        }
        // check for a new connection from chess server
        server_client = server.available();
    }
}

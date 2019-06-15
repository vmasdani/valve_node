/* 
 *  WATER VALVE SYSTEM
 *
 *  This is a water valve IoT system designed to turn on and off a
 *  single valve with a solenoid valve. The connection is MQTT
 *  through Websockets.
 *  
 *  Local MQTT browser i.e. Mosquitto should be installed in this system.
 *
 *  There are 2 types of control, automatic and manual.
 *
 *  Automatic control:
 *  Uses schedule from database to check whether the current time matches the
 *  desired time in the database record.
 *  Checking is done every 30 seconds in the server. This code receives
 *  data through MQTT, parses the desired watering length, and then 
 *  finally executes watering control.
 *  Time is synchronized through internet, so the device must be connected 
 *  to internet.
 *  Scheduled control uses JSON for its formatting for parsing purposes.
 *  
 *  JSON format:
 *  { "length": (int) watering time }
 *
 *  Manual control:
 *  Manual control is done within MQTT connectivity with the following topics
 *  and messages:
 *  /control/ message: 1 (on)
 *  /control/ message: 0 (off)
 *
 *  Below is the pinout of this system:
 *  RELAY | GPIO6 (WiringPi GPIO7)
 *  
 *  External libraries used:
 *  - WiringOP-Zero by xpertsavenue
 *  - MQTT-C by LiamBindle
 *  - cJSON by DaveGamble
 *  - wPi_soft_lcd by electronicayciencia
 *
 *  Constants are defined in the config/constants.h.
 *  use getip() to get current IP in wlan0 interface.
 *
 * */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mqtt.h>
#include <time.h>
#include <getip.h>
#include <cJSON.h>
#include <wiringPi.h>
#include <soft_lcd.h>

#include "templates/posix_sockets.h"
#include "config/constants.h"

struct reconnect_state_t { // MQTT reconnection state struct. from MQTT-C lib
  const char *hostname;
  const char *port;
  const char *topic;
  const char *topic_sched;
  uint8_t *sendbuf;
  size_t sendbufsz;
  uint8_t *recvbuf;
  size_t recvbufsz;
};

// Function to trigger when MQTT broker fails to connect
void reconnect_client(struct mqtt_client* client, void **reconnect_state_vptr);
// Callback function when MQTT data is received
void publish_callback(void **unused, struct mqtt_response_publish *published);
// Backend function to run MQTT re-checking
void *client_refresher(void *client);
// Safely close TCP sockfd and cancels refresher daemon
void exit_example(int status, int sockfd, pthread_t *client_daemon);

char *getTime() { // Get current system time
  time_t mytime = time(NULL);
  char *time_str = ctime(&mytime);
  time_str[strlen(time_str) - 1] = '\0';
  return time_str;
}

char ipaddr[15]; 
lcd_t *lcd;

int main() {
  printf("[%s]", getTime());
  printf("\n===== WATER VALVE SYSTEM =====\n");
  printf("MQTT CONFIG\nHost: %s\nPort: %s\nControl topic: %s\nSched topic: %s\n", 
         _host, _port, _topic, _topic_sched);  

  // init lcd
  lcd = lcd_create(9, 8, 0x27, 2);

  if(lcd == NULL) {
    printf("Cannot set-up LCD.\n");
  }

  if(getip() != NULL) {
    printf("Detected IP address: %s\n", getip());
    strncpy(ipaddr, getip(), 15);
    lcd_print(lcd, ipaddr);
  }
  else {
    printf("No IP Address detected!\n");
    lcd_print(lcd, "NO IP");
  }
  lcd_pos(lcd, 1, 0);
  lcd_print(lcd, "OFF");

  // Initialize wiringPi and relay output pin
  wiringPiSetup();
  pinMode(_relay_pin, OUTPUT);
  digitalWrite(_relay_pin, _relay_off);

  // Reconnecting structure
  struct reconnect_state_t reconnect_state;
  reconnect_state.hostname = _host;
  reconnect_state.port = _port;
  reconnect_state.topic = _topic;
  reconnect_state.topic_sched = _topic_sched;
  uint8_t sendbuf[2048];
  uint8_t recvbuf[1024];
  reconnect_state.sendbuf = sendbuf;
  reconnect_state.sendbufsz = sizeof(sendbuf);
  reconnect_state.recvbuf = recvbuf;
  reconnect_state.recvbufsz = sizeof(recvbuf);

  // Setup MQTT client
  struct mqtt_client client;

  mqtt_init_reconnect(&client,
    reconnect_client, &reconnect_state,
    publish_callback
  );

  // start MQTT refresh daemon
  pthread_t client_daemon;
  if(pthread_create(&client_daemon, NULL, client_refresher, &client)) {
    fprintf(stderr, "Failed to start client daemon.\n");
    exit_example(EXIT_FAILURE, -1, NULL);
  }
  else {
    printf("Client daemon init success!\n");
  }

  printf("\nPress CTRL-D to exit\n\n");
  printf("\nPress ENTER to inject error\n\n");
  
  // while(1);
  while(fgetc(stdin) != EOF) {// Press Ctrl-D to exit
    printf("Injecting error: \"MQTT_ERROR_SOCKET_ERROR\"");
    client.error = MQTT_ERROR_SOCKET_ERROR;
  }

  printf("\nDisconnecting...\n");
  usleep(1000000);

  // Exit safely
  exit_example(EXIT_SUCCESS, client.socketfd, &client_daemon);
  lcd_destroy(lcd);

  return 0;
}

void reconnect_client(struct mqtt_client *client, void **reconnect_state_vptr) {
  struct reconnect_state_t *reconnect_state = *((struct reconnect_state_t**) reconnect_state_vptr);
  
  // if not initial reconnect call, close socketfd
  if(client->error != MQTT_ERROR_INITIAL_RECONNECT) {
    printf("reconnect client: called while client was in error state\"%s\"\n",
      mqtt_error_str(client->error)
    );
  }

  // Open new socket
  int sockfd = open_nb_socket(reconnect_state->hostname, reconnect_state->port);
  if(sockfd == -1) {
    perror("Failed to open socket: ");
    exit_example(EXIT_FAILURE, sockfd, NULL);
  }
  else {
    printf("Init socket success!\n");
  }

  // Reinitialize client
  mqtt_reinit(client, sockfd,
    reconnect_state->sendbuf, reconnect_state->sendbufsz,
    reconnect_state->recvbuf, reconnect_state->recvbufsz
  );

  // Send connection request to broker
  mqtt_connect(client, "subscribing_client", NULL, NULL, 0, NULL, NULL, 0, 400);

  // Subscribe to topic
  mqtt_subscribe(client, reconnect_state->topic, 0); // control topic
  mqtt_subscribe(client, reconnect_state->topic_sched, 0); // schedule topic
}

void exit_example(int status, int sockfd, pthread_t *client_daemon) {
  if(sockfd != -1) close(sockfd);
  if(client_daemon != NULL) pthread_cancel(*client_daemon);
  lcd_destroy(lcd);
  exit(status);
}

void publish_callback(void **unused, struct mqtt_response_publish *published) {
  // published->topic_name conversion to c-string
  char *topic_name = (char*) malloc(published->topic_name_size + 1);
  memcpy(topic_name, published->topic_name, published->topic_name_size);
  topic_name[published->topic_name_size] = '\0';

  // published->application_message conversion to c-string
  char *app_msg = (char*) malloc(published->application_message_size + 1);
  memcpy(app_msg, published->application_message, published->application_message_size);
  app_msg[published->application_message_size] = '\0';

  // Print the received message
  printf("[%s] Received msg('%s'): %s\n", getTime(), topic_name, app_msg);

  /* === TOPIC: control === */
  if(strcmp(topic_name, "control") == 0) {
    printf("Control topic detected!\n");
    
    // Check valve switch status
    if(strcmp(app_msg, "1") == 0) {
      printf("Turning on valve...\n");
      digitalWrite(_relay_pin, _relay_on);
    
      // lcd write
      lcd_clear(lcd);
      lcd_pos(lcd, 0, 0);
      strncpy(ipaddr, getip(), 15);
      lcd_print(lcd, ipaddr);
      lcd_pos(lcd, 1, 0);
      lcd_print(lcd, "ON");
    }
    else if(strcmp(app_msg, "0") == 0) {
      printf("Turning off valve...\n");
      digitalWrite(_relay_pin, _relay_off);
    
      // lcd write
      lcd_clear(lcd);
      lcd_pos(lcd, 0, 0);
      strncpy(ipaddr, getip(), 15);
      lcd_print(lcd, ipaddr);
      lcd_pos(lcd, 1, 0);
      lcd_print(lcd, "OFF");
    }
    else {
      printf("Message does not match any of the control status.\n");
    }
  }
  else if(strcmp(topic_name, "schedule") == 0) {
    printf("Schedule topic detected! %s\n", app_msg);
  
    const cJSON *watering_length = NULL;
    // Parse the JSON "length" item
    cJSON *json = cJSON_Parse(app_msg);
    
    if(json == NULL) {
      printf("Invalid JSON detected!\n"); 
    }
    else {
      watering_length = cJSON_GetObjectItemCaseSensitive(json, "length");
      if(!cJSON_IsNumber(watering_length)) {
        printf("Message is not a number!\n");
      }
      else {
        int seconds_to_water = watering_length->valueint;
        int counter;
        printf("Now watering for %d seconds...\n", seconds_to_water);      
       
        // lcd write
        lcd_clear(lcd);
        lcd_pos(lcd, 0, 0);
        strncpy(ipaddr, getip(), 15);
        lcd_print(lcd, ipaddr);
        lcd_pos(lcd, 1, 0);
        lcd_print(lcd, "ON");

        digitalWrite(_relay_pin, _relay_on);
        for(counter = 0; counter < seconds_to_water; counter++) {
          printf("%d seconds have passed\n", counter);
          sleep(1);
        }
        digitalWrite(_relay_pin, _relay_off);
        printf("Done watering! Closing valve...\n");
      
        // lcd write
        lcd_clear(lcd);
        lcd_pos(lcd, 0, 0);
        strncpy(ipaddr, getip(), 15);
        lcd_print(lcd, ipaddr);
        lcd_pos(lcd, 1, 0);
        lcd_print(lcd, "OFF");
      }
    }

    cJSON_Delete(json); // Cleanup JSON object
    sleep(2);
  }
  else {
    printf("Topic irrelevant.\n");
  }

  free(topic_name);
  free(app_msg);
}

void *client_refresher(void *client) {
  while(1) {
    mqtt_sync((struct mqtt_client*) client);
    usleep(100000U);
  }

  return NULL;
}

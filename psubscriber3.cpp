#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>
#include <wiringPi.h>

#define ADDRESS     "tcp://172.20.10.7:1883"
#define CLIENTID    "rpi3"
#define AUTHMETHOD  "Spandan"
#define AUTHTOKEN   "spd"
#define TOPIC       "ee513/TempPitch"
#define QOS         1
#define TIMEOUT     10000L
#define LED_PIN     17 // WiringPi pin number

volatile MQTTClient_deliveryToken delivered_token;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    delivered_token = dt;
}

int handle_mqtt_message(void *context, char *topic_name, int topic_len, MQTTClient_message *message) {

	char* payloadptr;

	printf("----------------------------");
    printf("\n");
	printf("Message arrived\n");
		printf("     topic: %s\n", topic_name);
		printf("   message: ");
		payloadptr = (char*) message->payload;
		for(int i=0; i<message->payloadlen; i++) {
		    putchar(*payloadptr++);
		}
		putchar('\n');

	double pitch_value;
    char *payload = (char *)message->payload;
    char *pitch_start = strstr(payload, "\"Pitch\":");

    if (pitch_start) {
        pitch_value = atof(pitch_start + 9);
        printf("Pitch: %f\n", pitch_value);
        printf("\n");

        if (pitch_value > 20.0) {
            digitalWrite(LED_PIN, HIGH);
        } else {
            digitalWrite(LED_PIN, LOW);
        }
    } else {
        printf("Failed to parse Pitch from JSON payload\n");
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic_name);
    return 1;
}

void conn_lost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("Cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc, ch;

    // Initialize WiringPi library and set LED pin as output
    wiringPiSetupGpio();
    pinMode(LED_PIN, OUTPUT);

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, conn_lost, handle_mqtt_message, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, TIMEOUT);
    MQTTClient_destroy(&client);
    return rc;
}

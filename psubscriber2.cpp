#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <wiringPi.h>

#define ADDRESS     "tcp://172.20.10.7:1883"
#define CLIENTID    "rpi3"
#define AUTHMETHOD  "Spandan"
#define AUTHTOKEN   "spd"
#define TOPIC       "ee513/TempPitch"
#define QOS         1
#define TIMEOUT     10000L
#define LED_PIN     17 // WiringPi pin number

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	int i;
	char* payloadptr;
	double pitchValue;
	digitalWrite(LED_PIN, LOW); // Inital LOW
	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	printf("   message: ");
	payloadptr = (char*) message->payload;
	for(i=0; i<message->payloadlen; i++) {
	    putchar(*payloadptr++);
	}
	putchar('\n');


	char* payload = (char*)message->payload;
	char* pitchStart = strstr(payload, "\"Pitch\":");
	if (pitchStart) {
	      pitchValue = atof(pitchStart + 9);
	      printf("Pitch: %f\n", pitchValue);
	    } else {
	        printf("Failed to parse Pitch from JSON payload\n");
	    }

	if(pitchValue > 20.0){
		digitalWrite(LED_PIN, HIGH);
	} else {
		        digitalWrite(LED_PIN, LOW);
		    }
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;

}


void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    // Initialize WiringPi library and set LED pin as output
    wiringPiSetupGpio();
    pinMode(LED_PIN, OUTPUT);

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

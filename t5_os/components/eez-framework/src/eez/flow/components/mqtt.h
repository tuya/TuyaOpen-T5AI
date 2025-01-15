/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef __cplusplus
extern "C" {
#endif

//
// These functions must be implemented by the MQTT library adapter.
// Define EEZ_MQTT_ADAPTER if implementation is available.
//

#define MQTT_ERROR_OK 0
#define MQTT_ERROR_OTHER 1
#define MQTT_ERROR_NOT_IMPLEMENTED 2

int eez_mqtt_init(const char *protocol, const char *host, int port, const char *username, const char *password, void **handle);
int eez_mqtt_deinit(void *handle);
int eez_mqtt_connect(void *handle);
int eez_mqtt_disconnect(void *handle);
int eez_mqtt_subscribe(void *handle, const char *topic);
int eez_mqtt_unsubscribe(void *handle, const char *topic);
int eez_mqtt_publish(void *handle, const char *topic, const char *payload);

//
// The following function is implemented inside EEZ Framework and should be called by the MQTT library adapter
//

typedef enum {
    EEZ_MQTT_EVENT_CONNECT = 0, // eventData is null
    EEZ_MQTT_EVENT_RECONNECT = 1, // eventData is null
    EEZ_MQTT_EVENT_CLOSE = 2, // eventData is null
    EEZ_MQTT_EVENT_DISCONNECT = 3, // eventData is null
    EEZ_MQTT_EVENT_OFFLINE = 4, // eventData is null
    EEZ_MQTT_EVENT_END = 5, // eventData is null
    EEZ_MQTT_EVENT_ERROR = 6, // eventData is char *, i.e. error message
    EEZ_MQTT_EVENT_MESSAGE = 7 // eventData is EEZ_MQTT_MessageEvent *
} EEZ_MQTT_Event;

typedef struct {
    const char *topic;
    const char *payload;
} EEZ_MQTT_MessageEvent;

void eez_mqtt_on_event_callback(void *handle, EEZ_MQTT_Event event, void *eventData);

#ifdef __cplusplus
}
#endif

#ifndef SERVICE_H_
#define SERVICE_H_

#include <PubSubClient.h>

namespace Iotsec
{

    static PubSubClient *client = nullptr;
    void setMqttClient(PubSubClient &mqttClient)
    {
        client = &mqttClient;
    }

} // namespace Iotsec

#endif

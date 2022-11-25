//
// Created by tar on 21/11/22.
//

#ifndef JACKTRIP_TEENSY_JACKCONNECTOR_H
#define JACKTRIP_TEENSY_JACKCONNECTOR_H

#include <cstdio>
#include <JuceHeader.h>
#include <jack/jack.h>

class JackConnector {
public:

    JackConnector();

    virtual ~JackConnector();

    /**
     * Connect this app to all available jacktrip clients.
     */
    void connect();

    StringArray getJackTripClients();


    String getClientName();

private:
    static const uint NUM_MODULES{8}, IP_ADDRESS_BYTES{15};
    const char *WFS_OUT_PORT_FORMAT{"^" JUCE_JACK_CLIENT_NAME ":out_%d$"};
    const char *JACKTRIP_IN_PORT_FORMAT{"^_{2}f{4}_([0-9]{1,3}\.?){4}:send_%d$"};
    const char *CONNECTOR_CLIENT_NAME{"wfs_connector"};
    jack_client_t *client{nullptr};
    const char **outputDevices{nullptr};

    void openClient();
};


#endif //JACKTRIP_TEENSY_JACKCONNECTOR_H

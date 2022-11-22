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
private:
    const char *OUT_PORT_PATTERN{"^" JUCE_JACK_CLIENT_NAME ":out_%d$"};
    const char *IN_PORT_PATTERN{"^_{2}f{4}_([0-9]{1,3}\.?){4}:send_%d$"};
    const char *CLIENT_NAME{"wfs_connector"};
    jack_client_t *client;
};


#endif //JACKTRIP_TEENSY_JACKCONNECTOR_H

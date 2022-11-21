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

    void connect();
private:
    const char *OUT_PORT_PATTERN{"^alsa-jack\.jackP\.[0-9]{4,5}\.[0-9]:out_%03d$"};
    const char *IN_PORT_PATTERN{"^_{2}f{4}_([0-9]{1,3}\.?){4}:send_%d$"};
    const char *CLIENT_NAME{"wfs_control"};
    jack_client_t *client;
};


#endif //JACKTRIP_TEENSY_JACKCONNECTOR_H

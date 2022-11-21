//
// Created by tar on 21/11/22.
//

#include "JackConnector.h"

JackConnector::JackConnector() {
    jack_status_t status;
    client = jack_client_open(CLIENT_NAME, JackNoStartServer, &status);
    if (client == nullptr) {
        fprintf(stderr, "jack_client_open() failed, "
                        "status = 0x%2.0x\n", status);
    }
}

JackConnector::~JackConnector() {
    jack_client_close(client);
}

void JackConnector::connect() {
    const char **inPorts, **outPorts;

    auto i{0};
    do {
        char outPattern[50], inPattern[50];
        sprintf(outPattern, OUT_PORT_PATTERN, i);
        sprintf(inPattern, IN_PORT_PATTERN, i + 1);
        outPorts = jack_get_ports(client, outPattern, nullptr, JackPortIsOutput);
        inPorts = jack_get_ports(client, inPattern, nullptr, JackPortIsInput);
        for (int ch = 0; inPorts && inPorts[ch] != nullptr; ch++) {
//            fprintf(stdout, "Connecting %s to %s\n", outputPorts[0], inputPorts[ch]);
            jack_connect(client, outPorts[0], inPorts[ch]);
        }
        jack_free(outPorts);
        jack_free(inPorts);
        ++i;
    } while (outPorts);
}

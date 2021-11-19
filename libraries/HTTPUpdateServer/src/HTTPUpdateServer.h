#ifndef __HTTP_UPDATE_SERVER_H
#define __HTTP_UPDATE_SERVER_H

#include <SPIFFS.h>
#include <StreamString.h>
#include <Update.h>
#include <WebServer.h>
#include "Esp.h"

class HTTPUpdateServer
{
public:
    HTTPUpdateServer(bool serial_debug = false);

    void setup(WebServer *server);
    void setup2(WebServer *server);

    void setup(WebServer *server, const String &path);

    void setup(WebServer *server, const String &username, const String &password);

    void setup(WebServer *server, const String &path, const String &username, const String &password);

    void updateCredentials(const String &username, const String &password);

protected:
    void _setUpdaterError();

private:
    bool _serial_output;
    WebServer *_server;
    String _username;
    String _password;
    bool _authenticated;
    String _updaterError;
};

#endif
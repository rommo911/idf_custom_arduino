#include "HTTPUpdateServer.h"

static const char serverIndex[] PROGMEM =
    R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update Firmware'>
     </form>
     <form method='POST' action='' enctype='multipart/form-data'>
         FileSystem:<br>
         <input type='file' accept='.bin,.bin.gz,.image' name='filesystem'>
         <input type='submit' value='Update FileSystem'>
     </form>
     </body>
     </html>)";
static const char successResponse[] PROGMEM =
    "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...";

static const char loginIndex[] PROGMEM =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */

static const char serverIndex2[] PROGMEM =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

HTTPUpdateServer::HTTPUpdateServer(bool serial_debug)
{
    _serial_output = serial_debug;
    _server = NULL;
    _username = emptyString;
    _password = emptyString;
    _authenticated = false;
}

void HTTPUpdateServer::setup(WebServer *server)
{
    setup(server, emptyString, emptyString);
}
void HTTPUpdateServer::setup2(WebServer *server)
{
    _server = server;
    _server->on("/", HTTP_GET, [&]()
                { _server->send_P(200, PSTR("text/html"), loginIndex); });
    _server->on("/serverIndex", HTTP_GET, [&]()
                { _server->send_P(200, PSTR("text/html"), serverIndex2); });
    /*handling uploading firmware file */
    _server->on(
        "/update", HTTP_POST, [&]()
        {
    _server->send(200, PSTR("text/plain"), Update.hasError() ? String(F("FAIL")) : String(F("OK")) ) ;
      ESP_LOGW("UPDATE", "done"); 
      delay(10); },

        [&]()
        {
            ESP_LOGE("UPDATE", "Update request handling");
            HTTPUpload &upload = _server->upload();
            if (upload.status == UPLOAD_FILE_START)
            {
                ESP_LOGE("UPDATE", "Update UPLOAD_FILE_START");
                Serial.printf("Update: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                { // start with max available size
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                /* flashing firmware to ESP*/
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        });
}

void HTTPUpdateServer::setup(WebServer *server, const String &path)
{
    setup(server, path, emptyString, emptyString);
}

void HTTPUpdateServer::setup(WebServer *server, const String &username, const String &password)
{
    setup(server, "/update", username, password);
}

void HTTPUpdateServer::setup(WebServer *server, const String &path, const String &username, const String &password)
{
    _server = server;
    _username = username;
    _password = password;

    // handler for the /update form page
    _server->on(path.c_str(), HTTP_GET, [&]()
                {
                            if (_username != emptyString && _password != emptyString && !_server->authenticate(_username.c_str(), _password.c_str()))
                        { return _server->requestAuthentication();}
                        _server->send_P(200, PSTR("text/html"), serverIndex2); });

    // handler for the /update form POST (once file upload finishes)
    _server->on(
        path.c_str(), HTTP_POST, [&]()
        {
            if (!_authenticated)
              {  return _server->requestAuthentication();}
            if (Update.hasError())
            {
                _server->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
            }
            else {
                _server->client().setNoDelay(true);
                _server->send_P(200, PSTR("text/html"), successResponse);
                delay(100);
                _server->client().stop();
                ESP.restart();
            } },
        [&]()
        {
            // handler for the file upload, get's the sketch bytes, and writes
            // them through the Update object
            HTTPUpload &upload = _server->upload();

            if (upload.status == UPLOAD_FILE_START)
            {
                _updaterError.clear();

                _authenticated = (_username == emptyString || _password == emptyString || _server->authenticate(_username.c_str(), _password.c_str()));
                if (!_authenticated)
                {
                    ESP_LOGE("UPDATE", "Unauthenticated Update");
                    return;
                }

                ESP_LOGE("UPDATE", "Update: %s", upload.filename.c_str());
                if (upload.name == "filesystem")
                {
                    if (!Update.begin(SPIFFS.totalBytes(), U_SPIFFS))
                    { // start with max available size
                    }
                }
                else
                {
                    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                    if (!Update.begin(maxSketchSpace, U_FLASH))
                    { // start with max available size
                        _setUpdaterError();
                    }
                }
            }
            else if (_authenticated && upload.status == UPLOAD_FILE_WRITE && !_updaterError.length())
            {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    _setUpdaterError();
                }
            }
            else if (_authenticated && upload.status == UPLOAD_FILE_END && !_updaterError.length())
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    ESP_LOGI("UPDATE", "Update Success: %u\nRebooting...", upload.totalSize);
                }
                else
                {
                    _setUpdaterError();
                }
            }
            else if (_authenticated && upload.status == UPLOAD_FILE_ABORTED)
            {
                Update.end();
                ESP_LOGE("UPDATE", "Update was aborted");
            }
        });
    ESP_LOGI("httpuploader", " setr OK");
}

void HTTPUpdateServer::updateCredentials(const String &username, const String &password)
{
    _username = username;
    _password = password;
}

void HTTPUpdateServer::_setUpdaterError()
{
    StreamString str;
    Update.printError(str);
    _updaterError = str.c_str();
}

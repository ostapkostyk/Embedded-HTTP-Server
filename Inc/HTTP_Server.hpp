/**
  ******************************************************************************
  * @file    HTTP_Server.cpp
  * @author  Ostap Kostyk
  * @brief   Implementation of the HTTP Server to be used with ESP8266 class
  *
  ******************************************************************************
  * Copyright (C) 2018  Ostap Kostyk
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version provided that the redistributions
  * of source code must retain the above copyright notice.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  * Author contact information: Ostap Kostyk, email: ostap.kostyk@gmail.com
  ******************************************************************************
 */

#ifndef HTTP_SERVER_HPP_
#define HTTP_SERVER_HPP_

#include "Timer.h"
#include "ESP8266.hpp"
#include <string.h>

extern "C" {
#include "common.h"
}
#include "HTTP_content.h"

/* Application should render dynamic fields of the page and return true if success, otherwise false
 * Arguments: PageIndex is index of page in HTTPServerContent[] array, pHostName pointer to host name (text string) if received, otherwise zero (e.g. HTPP 1.0 protocol)*/
extern bool HTTP_RenderPage(int PageIndex, char *pHostName, bool **pProcessSemaphore);

namespace OKO_HTTP_SERVER
{
using namespace mTimer;
using namespace OKO_ESP8266;

#include "ESP8266.hpp"

#define HTTP_CLIENT_REQUEST_STRING_SIZE     700     //  should be long enough to receive HTTP header with query string with method "put". If only "get" method is intented to be used then the size could be much smaller to receive only part of HTTP header with query string and host name, e.g. 200-300
#define HTTP_CLIENT_HOST_NAME_SIZE          50
#define HTTP_SERVER_SOCKETS_MAX             ESP8266_SOCKETS_MAX

class HTTP_Server
{
public:
    /* Constructor */
    HTTP_Server(ESP* pESP);

    enum class ResponseStatusCode : int {Continue=100, OK=200, NotModified=304, BadRequest=400, AuthenticationRequired=401, Forbidden=403, NotFound=404, MethodNotAllowed=405, RequestURItooLarge=414, InternalServerError=500, MethodNotImplemented=501, ServiceUnavailable=503};

    /* Main Handler - must be called regularly (e.g. in main loop) */
    void Handle();

private:
    //TODO: change return type from int to ResponseStatusCode. Save page index into Process, return pure ResponseStatusCode
    ResponseStatusCode ParseHTTPRequest(char *ReqStr, size_t Len, uint8_t SocketID); //  parse request and search for the requested page name and arguments in content
    char* ParseQueryString(char *ReqStr, size_t len, HTTP_Server::ResponseStatusCode *response);

    ESP* pESP;  //  pointer to ESP8266 modem used for communication
    int NumOfPages;
    const int MaxNumOfPages = 100;                  //  Maximum number of pages in server's content
    Timer BaseTimer{Timer::Down, _100ms_, true};    //  down counting timer with dummy delay and enabled
    const int SocketConnectionTimeOut = 30;         //  Timeout for connected socket as number of BaseTimer ticks. If socket is connected and no data come within this timeout time then the socket will be closed
    const int MessageSendTimeout = 5;               //  Timeout for data send and close socket as number of BaseTimer ticks.
    const int ApplicationResponseTimeout = 100;     //  Application can request to wait while some process finishes before sending the page. This timeout is to limit time for application

    //enum class ParserStatusCodes : int { PageNotFound = -1, BadRequest = -2 };  // TODO: replace this with ResponseStatusCode type

    struct process
    {
       /* Constructor */
       process();

       char RequestString[HTTP_CLIENT_REQUEST_STRING_SIZE];
       char HostName[HTTP_CLIENT_HOST_NAME_SIZE+1];
       int RequestedPageIndex;
       bool HostNameFound;
       bool HostNameTooLong;
       int STEP;
       int TimeCounter;
       bool TimeoutFlag;
       bool *pSemaphore;
       int SendIndex;
    };

    process Process[HTTP_SERVER_SOCKETS_MAX];
};

}

#endif /* HTTP_SERVER_HPP_ */

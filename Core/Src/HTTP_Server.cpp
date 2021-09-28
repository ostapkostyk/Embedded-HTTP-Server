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

#include "HTTP_Server.hpp"

/*  This version of server handles one connection at a time to save RAM   */

const char HTTP_ServerResponseOKShort[] = "HTTP/1.1 200 OK\r\n\r\n";
const char HTTP_ServerResponseOKLong[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n";

const char HTTP_ServerResponseNoContent[] = "HTTP/1.1 204 No Content\r\n\r\n";
const char HTTP_ServerResponseInternalServerError[] = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
const char HTTP_ServerResponseBadRequest[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
const char HTTP_ServerResponseNotFound[] = "HTTP/1.1 404 Not Found\r\n\r\n";
const char HTTP_ServerResponseURITooLarge[] = "HTTP/1.1 414 Request URI too large\r\n\r\n";

using namespace OKO_ESP8266;
using namespace OKO_HTTP_SERVER;

HTTP_Server::HTTP_Server(ESP* pESP)
{
    this->pESP = pESP;

    for(int i=0; i<MaxNumOfPages; i++)
    {
        if(HTTPServerContent[i].pPage == 0)
        {
            NumOfPages = i;
            break;
        }
        else
        {
            HTTPServerContent[i].Type = HTTP_PageType::Static;
            for(int j=0; j<HTTPServerContent[i].PageParts; j++)
            {
                if(HTTPServerContent[i].pPage[j].Size == 0)     //  page contain at least one field that should be generated dynamically by application
                {
                    HTTPServerContent[i].Type = HTTP_PageType::Dynamic;
                    break;
                }
            }
        }
    }
}

HTTP_Server::process::process()
{
    STEP = 0;
    TimeCounter = 0;
    RequestedPageIndex = -1;    //  -1 should be out of possible indexes range
    TimeoutFlag = false;
    HostNameFound = false;
    HostNameTooLong = false;
    pSemaphore = 0;
    SendIndex = 0;
}

void HTTP_Server::Handle()
{
uint16_t DataLen;
ResponseStatusCode Response;
bool ret;
bool CloseSocketAfterSending;
uint8_t *pSendData = 0;
size_t len;
STATUS Status;
int PageIndex;

    if(BaseTimer.Elapsed())
    {
        BaseTimer.Reset();    //  TODO: Timer should be changed to restart automatically then this line to be removed and timer initialization to be changed in constructor
        for(uint8_t i=0; i < HTTP_SERVER_SOCKETS_MAX; i++)
        {
            if(Process[i].TimeCounter)
            {
                Process[i].TimeCounter--;
                if(Process[i].TimeCounter == 0)
                {
                    Process[i].TimeoutFlag = true;
                }
            }
        }
    }

    for(uint8_t i=0; i < HTTP_SERVER_SOCKETS_MAX; i++)
    {
        if(Process[i].TimeoutFlag)
        {
            if(pESP->GetSocketState(i) != ESP::eSocketState::Closed)
            {
                pESP->CloseSocket(i);
                Process[i].STEP = 200;  //  Close connected but inactive socket
            }
            Process[i].TimeoutFlag = false;
        }

        switch(Process[i].STEP)
        {
        case 0:     // assign buffer for incoming stream and unlock receiving
            if(SUCCESS == pESP->ListenSocket(i, (unsigned char*)Process[i].RequestString, sizeof(Process[i].RequestString)))
            {
                Process[i].TimeCounter = SocketConnectionTimeOut;
                Process[i].TimeoutFlag = false;
                Process[i].STEP = 1;
            }
            break;

        case 1:     //  check socket state, set timeouts
            if(pESP->GetSocketState(i) == ESP::eSocketState::Connected)
            {
                Process[i].STEP = 2;
            }

            Process[i].TimeCounter = SocketConnectionTimeOut;
            Process[i].TimeoutFlag = false;
            break;

        case 2:     // wait for new data
            // check socket state, set timeouts
            if(pESP->GetSocketState(i) != ESP::eSocketState::Connected)
            {
                if(pESP->GetSocketState(i) == ESP::eSocketState::Closed)
                {
                    Process[i].STEP = 0;
                    break;
                }

                pESP->CloseSocket(i);
                Process[i].STEP = 200;
                break;
            }

            // receive data
            DataLen = pESP->SocketRecv(i);
            if(DataLen == (uint16_t)-1)    //  Incoming Message is longer than available buffer and therefore has been cut
            {
                DataLen = sizeof(Process[i].RequestString);
                Process[i].RequestString[HTTP_CLIENT_REQUEST_STRING_SIZE-1] = 0;    //  terminate string to use sscanf() safe later on
            }
            else if(DataLen)
            {
                Process[i].RequestString[DataLen] = 0;    //  terminate string to use sscanf() safe later on
            }

            if(DataLen)     //  parse message
            {
                Process[i].TimeCounter = SocketConnectionTimeOut;   //  prolong timeout time

                Response = ParseHTTPRequest(Process[i].RequestString, HTTP_CLIENT_REQUEST_STRING_SIZE, i);

                pSendData = 0;
                switch(Response)
                {
                case ResponseStatusCode::OK:
                    if(HTTPServerContent[Process[i].RequestedPageIndex].Type == HTTP_PageType::Dynamic)
                    {
                        // Generate dynamic parts of the page by application
                        Process[i].pSemaphore = 0;  //  optional semaphore from application
                        if(Process[i].HostNameFound)
                        {
                            ret = HTTP_RenderPage(Process[i].RequestedPageIndex, Process[i].HostName, &(Process[i].pSemaphore));
                        }
                        else
                        {
                            ret = HTTP_RenderPage(Process[i].RequestedPageIndex, 0, &(Process[i].pSemaphore));
                        }

                        if(ret) //  application rendered page successfully, send it in next step (maybe by several pieces)
                        {
                            if(Process[i].pSemaphore == 0)  //  no need to wait for application
                            {
                                Process[i].STEP = 3;    //  Next step - send page
                            }
                            else                            //  wait for application to render the page
                            {
                                Process[i].TimeCounter = ApplicationResponseTimeout;    // set timeout for rendering
                                Process[i].STEP = 4;    //  Next step - send page
                            }
                            Process[i].SendIndex = 0;
                            break;
                        }
                        else    //  application was not able to render page, return error code
                        {
                            pSendData = (uint8_t*)HTTP_ServerResponseInternalServerError;
                            break;
                        }

                    }
                    else    //  static page
                    {
                        Process[i].SendIndex = 0;
                        Process[i].STEP = 3;    //  Next step - send page
                    }
                    break;

                case ResponseStatusCode::BadRequest:
                    pSendData = (uint8_t*)HTTP_ServerResponseBadRequest;
                    break;

                case ResponseStatusCode::InternalServerError:
                /* Following responses can be implemented separately. Here is not implemented to save resources */
                case ResponseStatusCode::Continue:
                case ResponseStatusCode::Forbidden:
                case ResponseStatusCode::MethodNotAllowed:
                case ResponseStatusCode::NotModified:
                case ResponseStatusCode::ServiceUnavailable:
                case ResponseStatusCode::AuthenticationRequired:
                    pSendData = (uint8_t*)HTTP_ServerResponseInternalServerError;
                    break;

                case ResponseStatusCode::RequestURItooLarge:
                    pSendData = (uint8_t*)HTTP_ServerResponseURITooLarge;
                    break;

                case ResponseStatusCode::NotFound:
                    pSendData = (uint8_t*)HTTP_ServerResponseNotFound;
                    break;

                default:
                    pSendData = (uint8_t*)HTTP_ServerResponseInternalServerError;
                }

                if(pSendData)
                {
                    if(SUCCESS == pESP->SocketSendClose(i, pSendData, (uint16_t)strlen((const char*)pSendData)))
                    {
                        //  wait for socket close
                        Process[i].TimeCounter = MessageSendTimeout;    // set socket close timeout
                        Process[i].STEP = 100;
                        break;
                    }
                    else
                    {
                        pESP->CloseSocket(i);
                        Process[i].STEP = 200;
                        break;
                    }
                }
                break;
            }

            break;

        case 3: //  SEND PAGE
            if(pESP->GetSocketState(i) != ESP::eSocketState::Connected ||
                   pESP->GetDataSendStatus(i) == ESP::eSocketSendDataStatus::SendFail)
            {
                Process[i].STEP = 200;
                break;
            }

            PageIndex = Process[i].RequestedPageIndex;

            if(Process[i].SendIndex < HTTPServerContent[PageIndex].PageParts)
            {
                if(Process[i].SendIndex+1 == HTTPServerContent[PageIndex].PageParts) { CloseSocketAfterSending = true; }    // last part of page
                else                                                         { CloseSocketAfterSending = false; }

                if(HTTPServerContent[PageIndex].pPage[Process[i].SendIndex].Size == 0)  //  dynamic part of page, size is not known, need to calculate
                {
                    len = strlen(HTTPServerContent[PageIndex].pPage[Process[i].SendIndex].pContent);
                    if(len)
                    {
                        pSendData = (uint8_t*)HTTPServerContent[PageIndex].pPage[Process[i].SendIndex].pContent;
                    }
                    else    //  empty block, send next block
                    {
                        // do nothing
                    }
                }
                else
                {
                    pSendData = (uint8_t*)HTTPServerContent[PageIndex].pPage[Process[i].SendIndex].pContent;
                    len = HTTPServerContent[PageIndex].pPage[Process[i].SendIndex].Size;
                }

                if(len)     //  there are data to send
                {
                    if(CloseSocketAfterSending)
                    {
                        Status = pESP->SocketSendClose(i, pSendData, (uint16_t)len);    //  this will also change STEP after data are sent out
                        if(Status == SUCCESS) debug_print("SRV: SendAndClose, len=%u\n", len);
                    }
                    else
                    {
                        Status = pESP->SocketSend(i, pSendData, (uint16_t)len);
                        if(Status == SUCCESS) debug_print("SRV: Send, len=%u\n", len);
                    }

                    if(Status == SUCCESS)   //  Next part of page
                    {
                        Process[i].SendIndex++;
                        Process[i].TimeCounter = SocketConnectionTimeOut;   //  prolong timeout time
                        break;
                    }
                    else
                    {
                        //  previous block is sending, wait for next cycle
                        break;
                    }
                }
            }

            break;

        case 4:     //  check if application is ready with page rendering
            if(*(Process[i].pSemaphore) == true)  //  application is ready with page rendering
            {
                Process[i].STEP = 3;    //  send out rendered page
            }
            break;

        case 100:   //  wait timeout and then close socket if not already closed
            if(pESP->GetSocketState(i) == ESP::eSocketState::Closed )
            {
                Process[i].TimeCounter = 0;
                Process[i].STEP = 0;
            }

            if(Process[i].TimeoutFlag)
            {
                if(SUCCESS == pESP->CloseSocket(i))
                {
                    Process[i].STEP = 200;
                }
            }

            break;

        case 200:
            if(pESP->GetSocketState(i) == ESP::eSocketState::Closed )
            {
                Process[i].STEP = 0;
            }
            else if(pESP->GetSocketState(i) == ESP::eSocketState::Connected ||     //  Socket has been connected again after closing, between steps
                    pESP->GetSocketState(i) == ESP::eSocketState::Error)
            {
                Process[i].STEP = 201;  //  Close socket because HTTP Server was not ready when it opened (should be very exceptional case)
            }
            break;

        case 201:   //  Socket timeout, close socket
            if(pESP->GetSocketState(i) != ESP::eSocketState::Closing )
            {
                if(SUCCESS == pESP->CloseSocket(i))
                {
                    Process[i].STEP = 200;
                }
                else
                {
                    Process[i].STEP = 0;
                }
            }
            break;

        default:
            Process[i].STEP = 0;
            break;
        }
    }
}


HTTP_Server::ResponseStatusCode HTTP_Server::ParseHTTPRequest(char *ReqStr, size_t len, uint8_t SocketID)
{
int pos = 0;
char c;
bool PageFound = false;
int num;
bool exit = false;
int HTTPVersion[2] = {0,0};
char *p;
enum class cMethod {NotImplemented = 0, Get, Post};
cMethod Method = cMethod::NotImplemented;
char *ReqStrStart = ReqStr;
char *ReqStrEnd = &ReqStr[len-1];
ResponseStatusCode response;

    switch(ReqStr[0])
    {
    case 'G':   // GET method
        if(0 == strncmp(ReqStr, "GET /", 5))
        {
            if(0 == strncmp(ReqStr, "GET /", 5))
            {
                Method = cMethod::Get;
                ReqStr += 5;
            }
            /* Here other methods can be implemented */
            else
            {
                return ResponseStatusCode::MethodNotImplemented;
            }
            break;
        }
        /* Here other methods can be implemented */
        else
        {
            return ResponseStatusCode::MethodNotImplemented;
        }
        break;

    case 'P':      //  "POST" or "PUT" methods
        if(0 == strncmp(ReqStr, "POST /", 6))
        {
            Method = cMethod::Post;
            ReqStr += 6;
        }
        /* Here other methods can be implemented */
        else
        {
            return ResponseStatusCode::MethodNotImplemented;
        }
        break;

    /* Here other methods can be implemented */

    default:
        return ResponseStatusCode::MethodNotImplemented;
        break;

    }

    if(Method == cMethod::Get || Method == cMethod::Post)
    {

        if(*ReqStr == ' ')  //  "GET / " or "POST / "
        {
            if(Method == cMethod::Get)  //  Home page requested
            {
                c = ' ';
                Process[SocketID].RequestedPageIndex = 0;
            }
            else if(Method == cMethod::Post)
            {
                return ResponseStatusCode::BadRequest;
            }
            else
            {
                return ResponseStatusCode::MethodNotImplemented;
            }
        }
        else
        {
            /* search for page name */
            sscanf(ReqStr, "%*" xstr(HTTP_CLIENT_REQUEST_STRING_SIZE) "[--9a-zA-Z_]%n", &pos);

            if(pos)
            {
                c = ReqStr[pos];    //  save next symbol for further analysis
                ReqStr[pos] = 0;    //  terminate string
                for(int i=0; i<NumOfPages; i++)     //  search in content if page exist on the server
                {
                    if(0 == strcmp(ReqStr, HTTPServerContent[i].pPageName))
                    {
                        Process[SocketID].RequestedPageIndex = i;
                        PageFound = true;
                        break;
                    }
                }
                if(PageFound == false) { return ResponseStatusCode::NotFound; }    //  requested page not found
                ReqStr += pos;
            }
            else    //  requested page not found
            {
                return ResponseStatusCode::NotFound;
            }
        }
    }

    if(Method == cMethod::Get)
    {
        if(c == ' ')    //  No arguments in this request
        {
            ReqStr++;  //  pointer on HTTP/X.Y
        }
        else if(c == '?')    //  There is/are argument(s)
        {
            ReqStr++;
            /* Parse Query String */
            if(ReqStrEnd - ReqStrStart > 0)
            {
                ReqStr = ParseQueryString(ReqStr, ReqStrEnd - ReqStr, &response);
                if(ResponseStatusCode::OK != response)
                {
                    return response;
                }
            }
            else
            {
                return ResponseStatusCode::InternalServerError;
            }
        }
        else    //  error, wrong format
        {
            PageFound = false;
            return ResponseStatusCode::BadRequest;
        }
    }
    else if(Method == cMethod::Post)
    {
        if(c == ' ')    //  No arguments in this request
        {
            ReqStr++;  //  pointer on HTTP/X.Y
        }
        else
        {
            return ResponseStatusCode::BadRequest;
        }
    }

    /* ======   parse HTTP/X.Y, parse "Host:" if HTTP ver 1.1 ======= */
    num = sscanf(ReqStr, " HTTP/%d.%d%n", &HTTPVersion[0], &HTTPVersion[1], &pos);
    if(2 == num && HTTPVersion[0] == 1)
    {
        if(HTTPVersion[1] == 0)     //  HTTP version 1.0, nothing to parse more in this version, return
        {
            Process[SocketID].HostNameFound = false;
        }
        else if(HTTPVersion[1] == 1)     //  HTTP version 1.1, Host name to be parsed
        {
            //  Host name is parsed in code below
        }
        else
        {
            return ResponseStatusCode::BadRequest;
        }
        ReqStr += pos;
    }
    else
    {
        return ResponseStatusCode::BadRequest;
    }

    /* ====== parse arguments in head ======= */
    sscanf(ReqStr, " %n", &pos);    //  delete white symbols in front, if any
    ReqStr+=pos;

    exit = false;
    while(exit == false)
    {
        p = strstr(ReqStr, "\r\n");           //  find end of current line
        if(p != 0 && (p < ReqStrEnd))         //  \r\n found inside received string
        {
            if(0 == strncmp(p, "\r\n\r\n", 4))
            {
                exit = true;    //  end of header found
            }
            //  find "Host:" string and set pos shift. Actual name reading is made separately to limit reading to buffer size and detect if name has been cut
            num = sscanf(ReqStr, " Host: %c%n ", &c, &pos);
            if(num == 1)
            {
                num = sscanf(&ReqStr[pos-1], "%" xstr(HTTP_CLIENT_HOST_NAME_SIZE) "s%n ", Process[SocketID].HostName, &pos);
                if(num)
                {
                    Process[SocketID].HostNameFound = true;
                    if(pos >= HTTP_CLIENT_HOST_NAME_SIZE)
                    {
                        Process[SocketID].HostName[HTTP_CLIENT_HOST_NAME_SIZE] = 0;     //  terminate string if Host name was too long
                        Process[SocketID].HostNameTooLong = true;
                    }
                    else
                    {
                        Process[SocketID].HostNameTooLong = false;
                    }
                }
                else
                {
                    return ResponseStatusCode::BadRequest;
                }
            }

            /* ===  Other arguments parsing can be implemented here */

            /* =====================================================*/

            ReqStr += p-ReqStr + 2;  //  2 is \r\n
        }
        else
        {
            exit = true;
            continue;
        }
    }

    if(Method == cMethod::Post)
    {
        /*====  Parse Query String ====*/
        ReqStr = ParseQueryString(ReqStr, ReqStrEnd - ReqStr, &response);
        return response;
    }

    return ResponseStatusCode::OK;
}

char* HTTP_Server::ParseQueryString(char *ReqStr, size_t len, HTTP_Server::ResponseStatusCode *response)
{
unsigned int pos = 0;
char c;
int num, IntValue;
#ifdef HTTP_SERV_SUPPORT_FLOATING_POINT_VARS
float FloatValue;
#endif
bool exit = false;
HTTPVariable* Variable = 0;

    sscanf(ReqStr, " %n", &pos);    //  delete white symbols in front, if any
    ReqStr+=pos;

    while(exit == false)
    {
        pos = 0;
        sscanf(ReqStr, " %*[^& \r\n=]%n", &pos);
        if(pos && pos<len)
        {
            if(ReqStr[pos] == '=')
            {
                ReqStr[pos] = 0;   // terminate string to search for variable name in the content
                Variable = HTTPVariable::FindVariable(ReqStr);
                if(Variable)        //  Variable found
                {
                    ReqStr += pos+1;
                    pos = 0;
                    sscanf(ReqStr, "%*[^& \r\n=]%n", &pos);
                    if(pos && pos<len)
                    {
                        c = ReqStr[pos];
                        ReqStr[pos] = 0;   //  terminate string
                        switch(Variable->Type)
                        {
                        case HTTPVariable::HTTPVarType::Text:
                            if(Variable->GetMaxTextSize() >= (size_t)(&ReqStr[pos] - ReqStr))   //  check if there is enough space to hold received string
                            {
                                num = sscanf(ReqStr, "%[^& \r\n=]s", Variable->pText);
                                if(num == 1)
                                {
                                    Variable->NewValue = true;
                                    HTTPVariable::HTTPVariableReceivedFlag = true;
                                }
                            }
                            break;

                        case HTTPVariable::HTTPVarType::Integer:
                            num = sscanf(ReqStr, "%d", &IntValue);
                            if(num == 1)
                            {
                                Variable->SetValueInteger(IntValue);
                                Variable->NewValue = true;
                                HTTPVariable::HTTPVariableReceivedFlag = true;
                            }
                            break;

                        case HTTPVariable::HTTPVarType::Float:
#ifdef HTTP_SERV_SUPPORT_FLOATING_POINT_VARS
                            num = sscanf(ReqStr, "%f", &FloatValue);
                            if(num == 1)
                            {
                                Variable->SetValueFloat(FloatValue);
                                Variable->NewValue = true;
                                HTTPVariable::HTTPVariableReceivedFlag = true;
                            }
#endif
                            break;
                        }
                        ReqStr += pos + 1;
                        if(c != '&')
                        {
                            exit = true;
                        }
                    }
                    else
                    {
                        *response = ResponseStatusCode::BadRequest;
                        return ReqStr;
                    }
                }
            }
            else
            {
                *response = ResponseStatusCode::BadRequest;
                return ReqStr;
            }
        }
        else
        {
            exit = true;
        }
    }

    *response = ResponseStatusCode::OK;
    return ReqStr;
}

/*****************************************************************************************************************************
 *                                  VARIABLES
 *****************************************************************************************************************************/

HTTPVariable* HTTPVariable::pFirst = 0;
bool HTTPVariable::HTTPVariableReceivedFlag = false;

HTTPVariable::HTTPVariable(const char *Name, HTTPVarType Type)
{
HTTPVariable* pVariable;

    Valid = false;

    if(Type == HTTPVariable::HTTPVarType::Text) { return; } //  too few arguments for text type of variable

    if(strlen(Name) == 0) { return; }   //  Name is not correct

    this->pName = Name;
    this->Type = Type;
    pText = 0;
    TextSize = 0;
    NewValue = false;
    ValueInteger = 0;
    ValueFloat = 0;

    pVariable = HTTPVariable::pFirst;

    if(pVariable == 0)     //  Creating first instance of the HTTPVariable class
    {
        HTTPVariable::pFirst = this;
    }
    else
    {
        while(pVariable->Next != 0)
        {
            pVariable = pVariable->Next;
        }
        pVariable->Next = this;
        Prev = pVariable;
    }

    Valid = true;
}


HTTPVariable::HTTPVariable(const char *Name, HTTPVarType Type, size_t TextSize)
{
HTTPVariable* pVariable;

    Valid = false;

    if(Type != HTTPVariable::HTTPVarType::Text) { return; } //  too few arguments for text type of variable

    if(strlen(Name) == 0) { return; }   //  Name is not correct

    if(TextSize == 0) { return; }   //  text size is not correct

    this->pName = Name;
    this->Type = Type;
    this->TextSize = 0;

    pText = (char*)malloc(TextSize);

    if(pText == 0) { return; }  //  can't allocate memory, variable cannot be created
    this->TextSize = TextSize;
    memset(pText, 0, TextSize);

    NewValue = false;
    ValueInteger = 0;
    ValueFloat = 0;

    pVariable = HTTPVariable::pFirst;

    if(pVariable == 0)     //  Creating first instance of the HTTPVariable class
    {
        HTTPVariable::pFirst = this;
    }
    else
    {
        while(pVariable->Next != 0)
        {
            pVariable = pVariable->Next;
        }
        pVariable->Next = this;
        Prev = pVariable;
    }

    Valid = true;
}

bool HTTPVariable::NewValueReceived(void)
{
    bool flag = NewValue;

    if(Valid == false) { return false; }

    NewValue = false;
    return flag;
}

char* HTTPVariable::GetText(void)
{
    if(Valid == false) { return 0; }

    if(Type == HTTPVariable::HTTPVarType::Text) { return pText; }

    return 0;
}

HTTPVariable* HTTPVariable::FindVariable(char* pName)
{
HTTPVariable* pVar = 0;

    if(HTTPVariable::pFirst == 0)   //  No timers exist
    {
        return 0;
    }

    pVar = HTTPVariable::pFirst;

    while(1)
    {
        if(0 == strcmp(pName, pVar->pName))
        {
            return pVar;    //  Variable has been found, return reference to the object
        }

        if(pVar->Next == 0) { return 0; }   //  all timers done

        pVar = pVar->Next;
    }
}




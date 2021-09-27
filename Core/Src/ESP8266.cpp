/**
  ******************************************************************************
  * @file    ESP8266.cpp
  * @author  Ostap Kostyk
  * @brief   ESP8266 class
  *          This class handles communication with WiFi modem ESP8266 using
  *          AT-commands
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

#include "ESP8266.hpp"

using namespace OKO_ESP8266;

static const U8 AT[] =                  "AT\r\n";
static const U8 ATE0[] =                "ATE0\r\n";    //  Echo Off
static const U8 AT_RST[] =              "AT+RST\r\n";
//static const U8 AT_CIOBAUD[] =          "AT+CIOBAUD?\r\n";
static const U8 AT_CWMODE_CUR_STA[] =   "AT+CWMODE_CUR=1\r\n";
static const U8 AT_CWMODE_CUR_AP[] =    "AT+CWMODE_CUR=2\r\n";
static const U8 AT_CWMODE_CUR_BOTH[] =  "AT+CWMODE_CUR=3\r\n";
static const U8 AT_CIPMUX_SINGLE[] =    "AT+CIPMUX=0\r\n";
static const U8 AT_CIPMUX_MULTIPLE[] =  "AT+CIPMUX=1\r\n";
static const U8 AT_CWSAP_CUR_REQ[] =    "AT+CWSAP_CUR?\r\n";
static const U8 AT_CWLAP_REQ[] =        "AT+CWLAP\r\n";
static const U8 AT_CIFSR[] =            "AT+CIFSR\r\n";


ESP::module::module()
{
ModeActual = eModuleMode::Undefined;
ModeRequest = eModuleMode::Undefined;
ConnectionTypeActual = eModuleConnectionType::Undefined;
ConnectionTypeRequest = eModuleConnectionType::Undefined;
BaudRateIndex = 0;
AP_NamePostfix = 0;
}

ESP::AccessPointCredentials::AccessPointCredentials()
{
    pName = Name;
    pPassword = Password;

    IP = 0;
    NetMask = 0;
    Gateway = 0;
    MAC = 0;

    NewIP = 0;
    NewNetMask = 0;
    NewGateway = 0;

    Channel = 0;
    ecn = ESP::eECNType::Open;
}

bool ESP::AccessPointCredentials::CheckNameAndPassword(void)
{
    if( strlen(pName)     > 0                   &&
        strlen(pName)     < ESP8266_AP_NAME_LEN &&
        strlen(pPassword) < ESP8266_AP_PWD_LEN      )
    {
        return true;
    }

    return false;
}

ESP::AccessPoint::AccessPoint()
{
    State = ESP::eAccessPointState::NotStarted;
    NewName = false;
    ChangeIPRequest = false;
}

ESP::Station::Station()
{
    StationState = ESP::eStationConnectionState::NotConnected;
}

ESP::server::server()
{
    State = eServerState::Undefined;
    StartRequest = false;
    Port = 0;
}

ESP::io::io()
{
pRxBuffer              = RxBuffer;
pCommandString         = CommandString;
pReceivedParameterStr  = ReceivedParameterStr;
pReceivedParameterStr2 = ReceivedParameterStr2;
pReceivedParameter     = ReceivedParameter;

pCurrentSocketData = 0;
ReceivedCommand = eAT::NO_COMMAND_RECEIVED;

RxBuffCounter = 0;
RxIgnoreCounter = 0;
CurrentSocketDataLeft = 0;
RxSocketId = 0;
ReceiveError = false;
DoEmptyRxStream = false;
ListenToTxData = false;
ReceivingDataStream = false;
RxOverflowEvent = false;
RxOverflowFlag = false;
}

ESP::socket::socket()
{
    State = eSocketState::Closed;
    ErrorFlag = eSocketErrorFlag::NoError;
    Type = eSocketType::UDP;
    Address = 0;
    Port = 0;
    DataRx = 0;
    RxBuffSize = 0;
    RxDataLen = 0;
    RxLock = false;
    DataTx = 0;
    TxDataLen = 0;
    TxPacketLen = 0;
    TxLock = false;
    TxState = eSocketSendDataStatus::Idle;
    DataCutFlag = false;
    CloseAfterSending = false;
}

void ESP::io::ClearReceivingErrors(void)
{
    if(ReceiveError) { DoEmptyRxStream = true; }
}

void ESP::io::ClearRxStream(void)
{
    DoEmptyRxStream = true;
}

ESP::ESP(uint8_t HuartNumber)
{
    this->HuartNumber = HuartNumber;
    CurrentState = &smStartModule;
    LastState = 0;
    SMStateChanged = true;
    STEP = 0;
    FirstTime = true;
    HuartConfigured = false;

    DebugFlag_RxStreamToStdOut = false;
    NewCommandSemaphore = false;

    for(int i = 0; i < ESP8266_SOCKETS_MAX; i++)
    {
        pSocket[i] = &Socket[i];
    }

}

void ESP::Process()
{
    if(FirstTime)
    {
        if(SUCCESS == ESP_HuartInit(HuartNumber))
        {
            HuartConfigured = true;
        }
        FirstTime = false;
    }

    /* do not run state machine if module is disabled by IO-pin, it will not respond anyway
     * TODO: take care that machine will stop when there is no data exchange/analysis by RxHandler() */
    if(ModuleToggleFlag == eModuleToggle::Disable) { return; }

    if(HuartConfigured == false) { return; }    //  do not run if HUART cannot be configured (communication will not run)

    if(ESP_HuartRxOverflow(HuartNumber))
    {
        if(IO.RxOverflowFlag == false)
        {
            RxOverflowTimer.Reset();
            IO.RxOverflowFlag = true;
            IO.RxOverflowEvent = true;
        }
    }

    if(IO.RxOverflowFlag)
    {
        if(RxOverflowTimer.Elapsed())
        {
            RxOverflowTimer.Stop(); //  to save CPU time
            IO.DoEmptyRxStream = true;
            IO.RxOverflowFlag = false;
        }
    }

    if(CurrentState != LastState)
    {
        LastState = CurrentState;
        SMStateChanged = true;
    }
    else
    {
        SMStateChanged = false;
    }

    if(IO.RxOverflowFlag == false)
    {
        CurrentState->Process(this);    //  call actual State Machine function
        NewCommandSemaphore = false;
    }

    RxHandler();
}

/******************************************************
 *  Re-initialize variables in case of Module Reset
 ******************************************************/
void ESP::ModuleReInit(void)
{
    CurrentState = &smStartModule;
    LastState = 0;
    SMStateChanged = true;

    DebugFlag_RxStreamToStdOut = false;
    NewCommandSemaphore = false;

    pIO = new (pIO) io;
    pModule = new (pModule) module;
    pLocalAP = new (pLocalAP) AccessPoint;
    pRemoteAP = new (pRemoteAP) Station;
    pServer = new (pServer) server;

    for(int i = 0; i < ESP8266_SOCKETS_MAX; i++)
    {
        pSocket[i] = new (pSocket[i]) socket;
    }
}

bool ESP::isCommandReceived(eAT cmd)
{
    if(cmd == IO.ReceivedCommand)
    {
        IO.ReceivedCommand = eAT::NO_COMMAND_RECEIVED;
        return true;
    }

    return false;
}

void ESP::StoreCommand(eAT cmd)
{
    IO.ReceivedCommand = cmd;
    NewCommandSemaphore = true;
    esp_debug_print("AT Command:%d\n", (int)cmd);
}

void ESP::ClearLastCommand(void)
{
    IO.ReceivedCommand = eAT::NO_COMMAND_RECEIVED;
}

bool ESP::isModuleReady(void)
{
    return Module.ModuleReady;
}

/*********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************
 *                      Public Methods
 *********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************/

void ESP::ModuleToggle(eModuleToggle toggle)
{
    ModuleToggleFlag = toggle;
    switch(ModuleToggleFlag)
    {
    default:
    case ESP::eModuleToggle::Disable:
        ESP_Disable(HuartNumber);
        break;

    case ESP::eModuleToggle::Enable:
        ESP_Enable(HuartNumber);
        break;

    }
}

STATUS ESP::SwitchToStationMode(char* AP_Name, char* AP_PWD)
{
    if((strlen(AP_Name) < RemoteAP.NameSize) && (strlen(AP_PWD) < RemoteAP.PasswordSize))
    {
        strncpy(RemoteAP.pName, AP_Name, RemoteAP.NameSize);
        strncpy(RemoteAP.pPassword, AP_PWD, RemoteAP.PasswordSize);
        Module.ModeRequest = eModuleMode::Station;
        return SUCCESS;
    }
    else
    {
        strcpy(RemoteAP.Name, "");
        strcpy(RemoteAP.Password, "");
        return ERROR;
    }
}

STATUS ESP::SwitchToAccessPointMode(char* AP_Name, char* AP_PWD, uint16_t ch, eECNType ecn)
{
    if((strlen(AP_Name) < LocalAP.NameSize) && (strlen(AP_PWD) < LocalAP.PasswordSize))
    {
        strncpy(LocalAP.pName, AP_Name, LocalAP.NameSize);
        strncpy(LocalAP.pPassword, AP_PWD, LocalAP.PasswordSize);
        Module.ModeRequest = eModuleMode::AccessPoint;
    }
    else
    {
        strcpy(LocalAP.pName, "");
        strcpy(LocalAP.pPassword, "");
        return ERROR;
    }
    if(ch > 0 && ch <= ESP8266_AP_CH_NUM)
    {
        LocalAP.Channel = ch;
    }
    else
    {
        LocalAP.Channel = 1;
    }

    LocalAP.ecn = ecn;

    return SUCCESS;
}

/* future feature */
STATUS ESP::SwitchToBothStaAndAPMode()
{
    return ERROR;
}

ESP::eModuleMode ESP::GetCurrentModuleMode()
{
    return Module.ModeActual;
}

bool ESP::SetModuleConnectionType(eModuleConnectionType Type)
{
    Module.ConnectionTypeRequest = Type;
    if(Module.ConnectionTypeRequest == Module.ConnectionTypeActual)
    {
      return true;
    }

    return false;
}

STATUS ESP::OpenSocket(uint8_t &SocketID, eSocketType Type)
{
    U8 i;

    for(i=0; i<SocketsNum; i++)
    {
        if(Socket[i].State == eSocketState::Closed)
        {
            Socket[i].State = eSocketState::Open;
            Socket[i].Address = 0;
            Socket[i].Port = 0;
            Socket[i].Type = Type;
            Socket[i].DataRx = 0;
            Socket[i].DataTx = 0;
            Socket[i].RxBuffSize = 0;
            Socket[i].RxDataLen = 0;
            SocketID = i;
            return SUCCESS;
        }
    }
    return ERROR;
}

STATUS ESP::ConnectSocket(uint8_t SocketID, char * Address, unsigned int Port)
{
    if(SocketID >= SocketsNum || Address == 0 || Port == 0 )
    {
        return ERROR;
    }

    if(Socket[SocketID].State == eSocketState::Open)
    {
        Socket[SocketID].State = eSocketState::ConnectRequested;
        Socket[SocketID].Address = Address;
        Socket[SocketID].Port = Port;
        Socket[SocketID].DataRx = 0;
        Socket[SocketID].DataTx = 0;
        Socket[SocketID].RxBuffSize = 0;
        Socket[SocketID].RxDataLen = 0;
        return SUCCESS;
    }

    return ERROR;
}

STATUS ESP::CloseSocket(uint8_t SocketID)
{
    if(SocketID >= SocketsNum)
    {
        return ERROR;
    }

    if(Socket[SocketID].State == eSocketState::Open)    //  Socket opened but not connected, so no input data is expected
    {
        Socket[SocketID].State = eSocketState::Closed;
        Socket[SocketID].Address = 0;
        Socket[SocketID].Port = 0;
        Socket[SocketID].DataRx = 0;
        Socket[SocketID].DataTx = 0;
        Socket[SocketID].RxBuffSize = 0;
        Socket[SocketID].RxDataLen = 0;
        return SUCCESS;
    }

    if(Socket[SocketID].State != eSocketState::Closed)
    {
        Socket[SocketID].State = eSocketState::CloseRequested;
        Socket[SocketID].Address = 0;
        Socket[SocketID].Port = 0;
        Socket[SocketID].DataRx = 0;
        Socket[SocketID].DataTx = 0;
        Socket[SocketID].RxBuffSize = 0;
        Socket[SocketID].RxDataLen = 0;
        return SUCCESS;
    }

    return SUCCESS;
}

ESP::eStationConnectionState ESP::StationConnectionState()
{
    return RemoteAP.StationState;
}

ESP::eAccessPointState ESP::LocalAccessPointState()
{
    return LocalAP.State;
}

uint8_t ESP::AccessPointNameChanged()
{
    return LocalAP.NewName;
}

ESP::eSocketState ESP::GetSocketState(uint8_t SocketID)
{
    if(SocketID >= SocketsNum)  //  socket id is out of range
        return eSocketState::Error;

    return Socket[SocketID].State;
}

ESP::eSocketSendDataStatus ESP::GetDataSendStatus(uint8_t SocketID)
{
    if(SocketID >= SocketsNum)  //  socket id is out of range
        return eSocketSendDataStatus::SendFail;

    return Socket[SocketID].TxState;
}

STATUS ESP::ListenSocket(uint8_t SocketID, uint8_t * RxBuffer, uint16_t BufferSize)
{
    if((SocketID >= SocketsNum) || (0 == RxBuffer) )
    {
        return ERROR;
    }

    Socket[SocketID].DataRx = RxBuffer;
    Socket[SocketID].RxBuffSize = BufferSize;
    Socket[SocketID].RxLock = false;

    return SUCCESS;
}

uint16_t ESP::SocketRecv(uint8_t SocketID)
{
    if(SocketID >= SocketsNum)  //  socket id is out of range
        return 0;

    if((Socket[SocketID].RxLock)   &&
        IO.RxIgnoreCounter == 0    &&      //  too long message ended
       (Socket[SocketID].ErrorFlag == eSocketErrorFlag::NoError))
    {
        if(Socket[SocketID].DataCutFlag)
        {
            return -1;
        }
        return Socket[SocketID].RxDataLen;
    }

    return 0;
}

STATUS ESP::SocketSend(uint8_t SocketID,  uint8_t* Data, uint16_t DataLen)
{
    if(SocketID >= SocketsNum)
    {
        return ERROR;
    }

    Socket[SocketID].CloseAfterSending = false;

    if((Socket[SocketID].State == eSocketState::Connected) &&
       (Socket[SocketID].TxLock == false))
    {
        Socket[SocketID].DataTx = Data;
        Socket[SocketID].TxDataLen = DataLen;
        Socket[SocketID].TxLock = true;       //  this must be cleared when data successfully transmitted
        return SUCCESS;
    }

    return ERROR;
}

STATUS ESP::SocketSendClose(uint8_t SocketID,  uint8_t* Data, uint16_t DataLen)
{
    if(SUCCESS == SocketSend(SocketID, Data, DataLen))
    {
        Socket[SocketID].CloseAfterSending = true;
        return SUCCESS;
    }

    return ERROR;
}

void ESP::StartServer(unsigned int Port)
{
    Server.Port = Port;
    Server.StartRequest = true;
}

void ESP::StopServer()
{
    Server.StartRequest = false;
}

ESP::eServerState ESP::GetServerState()
{
  return Server.State;
}

bool ESP::SetAccessPointIP(uint32_t ip, uint32_t gw, uint32_t mask)
{
    if(ip != LocalAP.IP         ||
       gw != LocalAP.Gateway    ||
       mask != LocalAP.NetMask   )
    {
        LocalAP.NewIP = ip;
        LocalAP.NewGateway = gw;
        LocalAP.NewNetMask = mask;
        LocalAP.ChangeIPRequest = true;
        return 1;
    }
    return 0;
}


/*********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************
 *                      Internal State Machine States
 *********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************/

void ESP::StateModuleReset::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
        esp_debug_print("ESP: Module Reset\n");
        pESP->Module.ModuleReady = false;
        pESP->STEP = 0;
    }

    pESP->IO.ClearReceivingErrors();     //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it

    switch(pESP->STEP)
    {
    case 0:
        ESP_ActivateResetPin(pESP->HuartNumber);
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_RST, sizeof(AT_RST)-1))    //  Try to reset by communication (this is needed if HW reset by IO-pin is not implemented)
        {
            pESP->StateTimer.Set(_200ms_);  //  time to send command over UART
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            pESP->StateTimer.Set(_1sec_);   //  retry after 1sec (if UART is overloaded then this time should be enough to empty it)
            pESP->StateTimer.Reset();
            pESP->STEP = 2;
        }
        break;

    case 1:
        if(pESP->StateTimer.Elapsed())   //  AT Restart command sent
        {
            ESP_ReleaseResetPin(pESP->HuartNumber);
            pESP->ModuleReInit();
            return;
        }
        break;

    case 2:
    {
        if(pESP->StateTimer.Elapsed())  //  time to flush Tx UART buffer is over, next try
        {
          pESP->STEP = 0;
        }
    }
    break;

    default:
        { pESP->STEP = 0; }
        break;
    }
}

void ESP::StateStartModule::Process(ESP* pESP)  //  wait for HW module initialization, read initial info sent by module
{
    if(pESP->StateMachineStateChanged())
    {
        pESP->STEP = 0;
        pESP->Module.ModuleReady = 0;
        esp_debug_print("ESP: Start\n");
    }

    pESP->IO.ClearReceivingErrors();     //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it

    switch(pESP->STEP)
    {
    case 0:
        pESP->StateTimer.Set(_3sec_);   //  time to initialize ESP module and finish transmitting initial output info, can take long time
        pESP->StateTimer.Reset();
        ESP_ReleaseResetPin(pESP->HuartNumber);
        pESP->STEP = 1;
        break;

    case 1:
        if(pESP->isCommandReceived(eAT::READY))     //  Some FW send "ready" but some not
        {
          pESP->CurrentState = &pESP->smTestModule;
          esp_debug_print("ESP8266: Ready received\n");
            break;
        }
        if(pESP->StateTimer.Elapsed()) //  "ready" not received
        {
            pESP->CurrentState = &pESP->smTestModule;
        }
        break;

    default:
      pESP->STEP = 0;
        break;
    }
}

void ESP::StateTestModule::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      esp_debug_print("ESP8266: Test by AT\n");
      pESP->IO.ClearRxStream();     //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it
      pESP->StateTimer.Set(_10ms_);
      pESP->StateTimer.Reset();
      pESP->STEP = 0;
    }

    switch(pESP->STEP)
    {
    case 0:
        if(pESP->StateTimer.Elapsed()) { pESP->STEP = 1; }  //  wait to clear input stream
        break;

    case 1:
        esp_debug_print("ESP8266: Debug In.stream:");
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT, sizeof(AT)-1))    //  test if module alive with AT command
        {
            pESP->StateTimer.Set(_200ms_);
            pESP->StateTimer.Reset();

            pESP->STEP = 2;
        }
        break;

    case 2:
        if(pESP->isCommandReceived(eAT::OK))
        {
            esp_debug_print("ESP8266: Test OK\n");

          //pESP->Module.BaudrateDetectionRequired = true;
            pESP->CurrentState = &pESP->smModuleInitialization;
            break;
        }
        if(pESP->StateTimer.Elapsed()) //   //  Module doesn't answer or answer with wrong Baudrate
        {
            //esp_debug_print("\n");
            pESP->CurrentState = &pESP->smDetectModuleBaudrate;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }
}

static uint8_t ESP8266_BAUDRATE_TRIES = 8;

void ESP::StateDetectModuleBaudrate::Process(ESP* pESP)
{
uint32_t Baud = 0;
U16 len;

    if(pESP->StateMachineStateChanged())
    {
        pESP->STEP = 10;
        pESP->Module.BaudRateIndex = 0;
        pESP->IO.ClearRxStream();     //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it
        pESP->StateTimer.Set(_30ms_);
        pESP->StateTimer.Reset();
        esp_debug_print("ESP8266: Baud Rate Detection/Change\n");
    }

    //pESP->IO.ClearReceivingErrors();     //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it

    switch(pESP->STEP)
    {
    case 0:
        if(pESP->Module.BaudRateIndex < ESP8266_BAUDRATE_TRIES)
        {
            switch(pESP->Module.BaudRateIndex)
            {
            case 0: Baud = (uint32_t)ESP::module::eBaudRates::Baud1; break;
            case 1: Baud = (uint32_t)ESP::module::eBaudRates::Baud2; break;
            case 2: Baud = (uint32_t)ESP::module::eBaudRates::Baud3; break;
            case 3: Baud = (uint32_t)ESP::module::eBaudRates::Baud4; break;
            case 4: Baud = (uint32_t)ESP::module::eBaudRates::Baud5; break;
            case 5: Baud = (uint32_t)ESP::module::eBaudRates::Baud6; break;
            case 6: Baud = (uint32_t)ESP::module::eBaudRates::Baud7; break;
            case 7: Baud = (uint32_t)ESP::module::eBaudRates::Baud8; break;

            default:
                pESP->CurrentState = &pESP->smModuleReset; //  exit by error
                break;
            }

            ESP_SetBaudRate(pESP->HuartNumber, Baud);
            pESP->ClearLastCommand();
        }
        else
        {
            //TODO: instead of reset (next line), number of tries can be programmed and then stop trying to save CPU resources until separate call from outside by separate interface method
            pESP->CurrentState = &pESP->smModuleReset; //  Module doesn't respond, start next try from the beginning
        }

        esp_debug_print("ESP8266: Test Baud %u: ", (unsigned int)Baud);

        //pESP->IO.ClearRxStream();

        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT, sizeof(AT)-1))
        {
            pESP->StateTimer.Set(_100ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            esp_debug_print("ERROR: Can't send data (%u)\n", __LINE__);
        }
        break;

    case 1:
        if(pESP->isCommandReceived(eAT::OK))
        {
            pESP->StateTimer.Set(_200ms_);
            pESP->StateTimer.Reset();
            esp_debug_print("OK\n");
            pESP->STEP = 2;
            break;
        }
        if(pESP->StateTimer.Elapsed())   //
        {
            esp_debug_print("fail\n");
            pESP->Module.BaudRateIndex++;
            pESP->IO.ClearRxStream();       //  if controller tries to communicate at wrong baud rate then trash is received continuously so need to clear all of it
            pESP->StateTimer.Set(_30ms_);   //  time to clear Rx stream
            pESP->StateTimer.Reset();
            pESP->STEP = 10;
        }
        break;

    case 2:
        if(pESP->StateTimer.Elapsed())   //  There is OK\r\n coming after CIOBAUD, need to clear it before next step
        {
            pESP->ClearLastCommand();
            pESP->STEP = 3;
        }
        break;

    case 3:
        esp_debug_print("ESP8266: Change baud rate to %u\n", (unsigned int)ESP8266_UART_SPEED);
        len = snprintf(pESP->IO.pCommandString, 30, "AT+UART_CUR=%u,8,1,0,0\r\n", (unsigned int)ESP8266_UART_SPEED);    //  speed,8n1, no flow control

        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
        {
            pESP->StateTimer.Set(_100ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 4;
        }
        else
        {
            esp_debug_print("ESP8266: BaudChange: TX failed\n");
            ESP_SetBaudRate(pESP->HuartNumber, (uint32_t)ESP::module::eBaudRates::Baud2);
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 4:
        if(pESP->isCommandReceived(eAT::OK))
        {
            esp_debug_print("ESP8266: Baud rate changed successfully. Time:%u\n", (unsigned int)(_100ms_) - (unsigned int)(pESP->StateTimer.Get()));
            ESP_SetBaudRate(pESP->HuartNumber, pESP->Module.BaudRate);
            pESP->StateTimer.Set(_200ms_);   //  Now need to wait for ESP module to apply new settings, therefore next step exists
            pESP->StateTimer.Reset();
            pESP->STEP = 5;
            break;
        }
        if(pESP->StateTimer.Elapsed())   //
        {
            esp_debug_print("ESP8266: Change baud failed, %u, Time:%u\n", __LINE__, (unsigned int)(_100ms_) - (unsigned int)(pESP->StateTimer.Get()));
            ESP_SetBaudRate(pESP->HuartNumber, (uint32_t)ESP::module::eBaudRates::Baud2);
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 5:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->ClearLastCommand();
            pESP->CurrentState = &pESP->smModuleInitialization;
        }
        break;

    case 10:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->STEP = 0;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }
}

/*********************************************************
 * Initialize module
 * 1. Turn echo off: ATE0 : Disable echo; ATE1 : Enable echo
 *********************************************************/
void ESP::StateModuleInitialization::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
    }

    switch(pESP->STEP)
    {
    case 0:
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)ATE0, sizeof(ATE0)-1))
        {
            esp_debug_print("ESP8266: Echo Off cmd: ");

            pESP->StateTimer.Set(_200ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        break;

    case 1:
        if(pESP->isCommandReceived(eAT::OK))
        {

            esp_debug_print("done\n");

            pESP->Module.EchoIsOnFlag = false;
            pESP->Module.ModuleReady = true;
            pESP->CurrentState = &pESP->smSelectModuleMode;
            break;
        }
        if(pESP->StateTimer.Elapsed())   //
        {
            esp_debug_print("failed\n");
            pESP->StateTimer.Set(_1sec_);
            pESP->StateTimer.Reset();
            pESP->STEP = 2;
        }
        break;

    case 2:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    default:
        if(pESP->StateTimer.Elapsed()) { pESP->STEP = 0; }
        break;
    }
}

void ESP::StateSelectModuleMode::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      esp_debug_print("ESP8266: Select Mode\n");
      pESP->STEP = 0;
      pESP->ClearLastCommand();
    }

    switch(pESP->STEP)
    {
    case 0:
        if(pESP->Module.ModeRequest == ESP::eModuleMode::Undefined) { break; }
        else
        {
            switch(pESP->Module.ModeRequest)
            {
            case ESP::eModuleMode::Station:
                if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CWMODE_CUR_STA, sizeof(AT_CWMODE_CUR_STA)-1))
                {
                    pESP->StateTimer.Set(_200ms_);
                    pESP->StateTimer.Reset();
                    pESP->STEP = 1;
                }
                else
                {
                    pESP->CurrentState = &pESP->smModuleReset;
                }
                break;

            case ESP::eModuleMode::AccessPoint:  //  First, switch to ModeBoth to find out if ssid is free and generate new ssid if not free
/*              if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CWMODE_CUR_AP, sizeof(AT_CWMODE_CUR_AP)-1))
                {
                    pESP->StateTimer.Set(_200ms_);
                    pESP->StateTimer.Reset();
                    pESP->STEP = 1;
                }
                else
                {
                    pESP->CurrentState = &pESP->smModuleReset;
                }
                break;
*/
            case ESP::eModuleMode::StationAndAccessPoint:
                if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CWMODE_CUR_BOTH, sizeof(AT_CWMODE_CUR_BOTH)-1))
                {
                    pESP->StateTimer.Set(_200ms_);
                    pESP->StateTimer.Reset();
                    pESP->STEP = 1;
                }
                else
                {
                    pESP->CurrentState = &pESP->smModuleReset;
                }
                break;

            default:
              pESP->STEP = 0;
                break;
            }
        }
        break;

    case 1:
        if((pESP->isCommandReceived(eAT::OK)) || (pESP->isCommandReceived(eAT::NOCHANGE)))
        {
            pESP->Module.ModeActual = pESP->Module.ModeRequest;

            switch(pESP->Module.ModeRequest)
            {
            case ESP::eModuleMode::Station:
esp_debug_print("ESP8266: Mode Sta selected\n");
                pESP->CurrentState = &pESP->smJoinAP;
                break;

            case ESP::eModuleMode::AccessPoint:
esp_debug_print("ESP8266: Mode AP selected\n");
                pESP->CurrentState = &pESP->smFindFreeSSID;
                break;

            case ESP::eModuleMode::StationAndAccessPoint:
            default:
esp_debug_print("ESP8266: Mode Sta+Ap selected\n");
                pESP->Module.AP_NamePostfix = 0;
                pESP->CurrentState = &pESP->smFindFreeSSID;
                break;
            }
            break;
        }
        if(pESP->StateTimer.Elapsed())
        {
            pESP->Module.ModeActual = ESP::eModuleMode::Undefined;
            pESP->STEP = 0;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }
}

void ESP::StateFindFreeSSID::Process(ESP* pESP)
{
int len;

  if(pESP->StateMachineStateChanged())
  {
    pESP->STEP = 0;
    pESP->LocalAP.State = eAccessPointState::NotStarted;
    pESP->ClearLastCommand();
    esp_debug_print("ESP8266: Searching Free SSID\n");
  }

  switch(pESP->STEP)
  {
    case 0:
       if( pESP->LocalAP.CheckNameAndPassword() )
       {
          if(pESP->Module.AP_NamePostfix)
          {
              len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWLAP=\"%s_%u\"\r\n", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix);
          }
          else
          {
              len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWLAP=\"%s\"\r\n", pESP->LocalAP.pName);
          }
          if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
          {
              pESP->StateTimer.Set(_10sec_);
              pESP->StateTimer.Reset();
              pESP->STEP = 1;
          }
          else
          {
              pESP->CurrentState = &pESP->smModuleReset;
          }
       }
    break;

    case 1:
      if(pESP->isCommandReceived(eAT::CWLAP))  //  AP Exists
      {
        pESP->Module.AP_NamePostfix++;
        if(pESP->Module.AP_NamePostfix > 200)   //  Some general fault
        {
          pESP->CurrentState = &pESP->smModuleReset;
          break;
        }
        esp_debug_print("ESP8266: trying postfix %u\n", pESP->Module.AP_NamePostfix);
        pESP->StateTimer.Set(_200ms_);
        pESP->StateTimer.Reset();
        pESP->STEP = 2;   //  Wait for coming "OK"
        break;
      }
      if(pESP->isCommandReceived(eAT::OK))  //  AP Name is free
      {
        if( 0 == pESP->Module.AP_NamePostfix ) { esp_debug_print("ESP8266: found free SSID \"%s\"\n", pESP->LocalAP.pName); }
        else { esp_debug_print("ESP8266: found free SSID \"%s_%u\"\n", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix); }

        pESP->CurrentState = &pESP->smSetApParameters;
      }
      if(pESP->StateTimer.Elapsed())   //  No answer
      {
          pESP->CurrentState = &pESP->smModuleReset;
      }
    break;

    case 2:
        if(pESP->isCommandReceived(eAT::OK))
        {
          pESP->STEP = 0; //  Try name with postfix
        }
        if(pESP->StateTimer.Elapsed())   //  No answer
        {
          pESP->STEP = 0;
        }
      break;

    default:
    pESP->STEP = 0;
    break;
  }
}

void ESP::StateSetApParameters::Process(ESP* pESP)
{
int len;

if(pESP->StateMachineStateChanged())
{
    pESP->STEP = 0;
    pESP->LocalAP.State = eAccessPointState::NotStarted;
    pESP->ClearLastCommand(); // clear command buff

    if(pESP->Module.AP_NamePostfix == 0) { esp_debug_print("ESP8266: Start AP:%s\n", pESP->LocalAP.pName); }
    else                                 { esp_debug_print("ESP8266: Start AP:%s_%d\n", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix); }
}

    if(! pESP->LocalAP.CheckNameAndPassword())
    {
        return; //  Name for AP is not assigned yet
    }

    switch(pESP->STEP)
    {
    /* First check if ESP is already running with desired AP ssid */
    case 0:
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CWSAP_CUR_REQ, sizeof(AT_CWSAP_CUR_REQ)-1))
        {
            pESP->StateTimer.Set(_200ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;
    break;

    case 1:
        if(pESP->isCommandReceived(eAT::CWSAP_CUR))  //  Current AP configuration received
        {
          pESP->STEP = 2;
        }
        if(pESP->StateTimer.Elapsed())   //  No answer
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }

        break;

        //  Wait for OK response
    case 2:
        if(pESP->isCommandReceived(eAT::OK))
        {
            pESP->STEP = 3;
        }
        if(pESP->StateTimer.Elapsed())   //  No answer
        {
            pESP->STEP = 3;
        }
      break;

    case 3:
        if( pESP->LocalAP.CheckNameAndPassword() )
        {
            if(pESP->Module.AP_NamePostfix)
            {
                //snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWSAP_CUR=\"%s_%u\",\"%s\",%u,%u\r\n", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix, pESP->LocalAP.pPassword, pESP->LocalAP.Channel, pESP->LocalAP.ecn);
                snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "%s_%u", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix);
            }
            else
            {
                //snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWSAP_CUR=\"%s\",\"%s\",%u,%u\r\n", pESP->LocalAP.pName, pESP->LocalAP.pPassword, pESP->LocalAP.Channel, pESP->LocalAP.ecn);
                snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "%s", pESP->LocalAP.pName);
            }

            if(0 == strcmp(pESP->IO.pCommandString, pESP->IO.pReceivedParameterStr))   //  AP current configuration matches requested
            {
                pESP->LocalAP.State = eAccessPointState::Started;
                if(pESP->Module.AP_NamePostfix)     //  renew the name and set "New Name" flag
                {
                    snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "%s_%u", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix);   //  Generate new name
                    strncpy(pESP->LocalAP.pName, pESP->IO.pCommandString, pESP->LocalAP.NameSize-1);        //  copy new name
                    pESP->LocalAP.NewName = pESP->Module.AP_NamePostfix;
                }
                esp_debug_print("ESP8266: AP Already Started\n");
                pESP->CurrentState = &pESP->smStandby;
                break;
            }
            else    //  requested configuration is different from current one, need to apply it and restart the module
            {
                pESP->STEP = 4;
            }
        }
        break;

    case 4:
        // Generate string to start AP
        if(pESP->Module.AP_NamePostfix)
        {
            len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWSAP_CUR=\"%s_%u\",\"%s\",%u,%u\r\n", pESP->LocalAP.pName, pESP->Module.AP_NamePostfix, pESP->LocalAP.pPassword, pESP->LocalAP.Channel, (unsigned int)pESP->LocalAP.ecn);
        }
        else
        {
            len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize - 1, "AT+CWSAP_CUR=\"%s\",\"%s\",%u,%u\r\n", pESP->LocalAP.pName, pESP->LocalAP.pPassword, pESP->LocalAP.Channel, (unsigned int)pESP->LocalAP.ecn);
        }

        // Start AP
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
        {
            pESP->StateTimer.Set(_2sec_);       //  could be significant delay to set-up new parameters
            pESP->StateTimer.Reset();
            pESP->LocalAP.State = eAccessPointState::Starting;
            pESP->STEP = 5;
        }
        else
        {
            pESP->LocalAP.State = eAccessPointState::NotStarted;
            pESP->LocalAP.pName[0] = 0;       //  empty name string
            pESP->LocalAP.pPassword[0] = 0;        //  empty password string
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 5:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->LocalAP.State = eAccessPointState::Timeout;
            pESP->LocalAP.pName[0] = 0;        //  empty name string
            pESP->LocalAP.pPassword[0] = 0;         //  empty password string

            esp_debug_print("ESP8266: AP Start Timeout !!!\n");

            pESP->LocalAP.State = eAccessPointState::Failed;
            pESP->LocalAP.pName[0] = 0;       //  empty name string
            pESP->LocalAP.pPassword[0] = 0;        //  empty password string

            pESP->STEP = 0;
        }
        if(pESP->isCommandReceived(eAT::OK))
        {
            esp_debug_print("ESP8266: AP Started\n");
            pESP->LocalAP.State = eAccessPointState::Started;
            pESP->CurrentState = &pESP->smStandby;         //
        }
        if(pESP->isCommandReceived(eAT::AT_ERROR) || pESP->isCommandReceived(eAT::FAIL))
        {
            esp_debug_print("ESP8266: AP Start Failed\n");
            pESP->LocalAP.State = eAccessPointState::Failed;
            pESP->LocalAP.pName[0] = 0;       //  empty name string
            pESP->LocalAP.pPassword[0] = 0;        //  empty password string
            pESP->CurrentState = &pESP->smStandby;
        }
        break;

    default:
        esp_debug_print("ESP8266: SetApParam DEFAULT reached, STEP=0\n");
        pESP->STEP = 0;
        break;
    }
}

void ESP::StateJoinAP::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
    }

    switch(pESP->STEP)
    {
    case 0:
        pESP->RemoteAP.StationState = eStationConnectionState::Disconnected;
        if( pESP->RemoteAP.CheckNameAndPassword() )
        {
          pESP->STEP = 1;
        }
        break;

    case 1:
        sprintf(pESP->IO.pCommandString, "AT+CWJAP=\"%s\",\"%s\"\r\n", pESP->RemoteAP.pName, pESP->RemoteAP.pPassword);
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, strlen(pESP->IO.pCommandString)))
        {
            pESP->StateTimer.Set(_20sec_);
            pESP->StateTimer.Reset();
            pESP->RemoteAP.StationState = eStationConnectionState::Connecting;
            pESP->STEP = 2;
        }
        else
        {
            pESP->RemoteAP.StationState = eStationConnectionState::NotConnected;
            pESP->RemoteAP.pName[0] = 0;        //  empty name string
            pESP->RemoteAP.pPassword[0] = 0;    //  empty password string
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 2:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->RemoteAP.StationState = eStationConnectionState::ConnectTimeout;
            pESP->RemoteAP.pName[0] = 0;        //  empty name string
            pESP->RemoteAP.pPassword[0] = 0;    //  empty password string
            pESP->STEP = 0;
        }
        else if(pESP->isCommandReceived(eAT::OK))
        {
            pESP->RemoteAP.StationState = eStationConnectionState::Connected;
            pESP->CurrentState = &pESP->smStandby;
        }
        else if(pESP->isCommandReceived(eAT::FAIL))
        {
            pESP->RemoteAP.pName[0] = 0;        //  empty name string
            pESP->RemoteAP.pPassword[0] = 0;    //  empty password string
            pESP->RemoteAP.StationState = eStationConnectionState::ConnectFailed;
            pESP->STEP = 0;
        }
        break;

    default:
      pESP->STEP = 0;
        break;
    }

}

void ESP::StateStandby::Process(ESP* pESP)
{
U8 i;

  if(pESP->StateMachineStateChanged())
  {
    pESP->STEP = 0;
    pESP->ClearLastCommand();
    esp_debug_print("ESP8266: Standby\n");
  }

    if(pESP->isCommandReceived(eAT::SOCKET_CONNECT))  //  Socket opened in server mode
    {
        if(pESP->IO.pReceivedParameter[0] < pESP->SocketsNum)
        {
            SocketId = (uint8_t)pESP->IO.pReceivedParameter[0];
            pESP->Socket[SocketId].State = ESP::eSocketState::Connected;
            esp_debug_print("ESP: Socket %u Opened\n", SocketId);
            //pESP->DebugFlag_RxStreamToStdOut = true;  //  this sends received content to std out stream (debug)
            return;
        }
    }

    if(pESP->isCommandReceived(eAT::SOCKET_CLOSED))  //  Socket closed in server mode
    {
        if(pESP->IO.pReceivedParameter[0] < pESP->SocketsNum)
        {
            SocketId = (uint8_t)pESP->IO.pReceivedParameter[0];
            pESP->Socket[SocketId].State = ESP::eSocketState::Closed;
            esp_debug_print("ESP8266: Socket %u Closed\n", SocketId);
            //pESP->DebugFlag_RxStreamToStdOut = false;
            return;
        }
    }

    // Send data
    for(i=0; i < pESP->SocketsNum; i++)
    {
        if((pESP->Socket[i].State == eSocketState::Connected) &&    //  for connected sockets
           (pESP->Socket[i].TxLock == true) &&                      //  Previous sending completed and new data send has been requested
           (0 != pESP->Socket[i].TxDataLen))                        //  There is new data to send-out
        {
            pESP->CurrentState = &pESP->smSendData;
            return;
        }
    }

    if(pESP->Module.ConnectionTypeRequest != pESP->Module.ConnectionTypeActual)
    {
        pESP->CurrentState = &pESP->smChangeConnectionType;
        return;
    }

    if(pESP->Server.StartRequest == true)
    {
        if(pESP->Module.ConnectionTypeActual == eModuleConnectionType::Multiple)      //  It is only allowed to start the server in multiple connection mode
        {
            pESP->CurrentState = &pESP->smStartServer;
            return;
        }
        else    //  Request to switch to another mode first
        {
            pESP->SetModuleConnectionType(eModuleConnectionType::Multiple);
            return;
        }
    }

    if(pESP->LocalAP.ChangeIPRequest)
    {
        pESP->CurrentState = &pESP->smChangeAPIP;
        return;
    }

    // Open socket by request
    for(i=0; i < pESP->SocketsNum; i++)
    {
        if(pESP->Socket[i].State == eSocketState::ConnectRequested)
        {
            pESP->CurrentState = &pESP->smOpenSocket;
            return;
        }
    }

    // Close socket by request
    for(i=0; i < pESP->SocketsNum; i++)
    {
        if(pESP->Socket[i].State == eSocketState::CloseRequested)
        {
            pESP->CurrentState = &pESP->smCloseSocket;
            return;
        }
    }

    if(pESP->isCommandReceived(eAT::UNLINK))
    {
        //  TODO: check sockets etc.
    }

    if(pESP->isCommandReceived(eAT::LINK))
    {
        //TODO: react on this, data will come
    }

    //TODO: maybe move this to main ESP handler to catch it properly also during other states?
    if(pESP->isCommandReceived(eAT::REBOOT_DETECTED) || pESP->isCommandReceived(eAT::WDT_RESET))
    {
        //TODO: react on module reset during operation
    }

}

void ESP::StateChangeConnectionType::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      esp_debug_print("ESP8266: Change Connection Type\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        if(pESP->Module.ConnectionTypeRequest == eModuleConnectionType::Multiple)
        {
            if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CIPMUX_MULTIPLE, sizeof(AT_CIPMUX_MULTIPLE)-1))
            {
                pESP->StateTimer.Set(_200ms_);
                pESP->StateTimer.Reset();
                pESP->STEP = 1;
            }
            else
            {
                pESP->CurrentState = &pESP->smModuleReset;
            }
        }
        else if(pESP->Module.ConnectionTypeRequest == eModuleConnectionType::Single)
        {
            if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CIPMUX_SINGLE, sizeof(AT_CIPMUX_SINGLE)-1))
            {
                pESP->StateTimer.Set(_200ms_);
                pESP->StateTimer.Reset();
                pESP->STEP = 1;
            }
            else
            {
                pESP->CurrentState = &pESP->smModuleReset;
            }
        }
        else
        {
            pESP->CurrentState = &pESP->smStandby;
        }
        break;

    case 1:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }

        if(pESP->isCommandReceived(eAT::OK) || pESP->isCommandReceived(eAT::LINKISBUILDED) || pESP->isCommandReceived(eAT::NOCHANGE))
        {
            pESP->Module.ConnectionTypeActual = pESP->Module.ConnectionTypeRequest;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->isCommandReceived(eAT::AT_ERROR))
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    default:
      pESP->STEP = 0;
        break;
    }
}

/*************************************************************************************************
 * SEND DATA
 *
 * Other: When SINGLE packet containing “+++” is received, it returns to command mode. (if command
 *        "AT+CIPSEND" performed. Unvarnished transmission mode)
 *
 *************************************************************************************************/
void ESP::StateSendData::Process(ESP* pESP)
{
unsigned int len;
U8 i;

    if(pESP->StateMachineStateChanged())
    {
        pESP->STEP = 0;
        RetryCounter = 5;
        pESP->ClearLastCommand();
        // TODO: when to set ESP8266_DATA_TRANSMIT_IDLE???  pESP->Sockets[i].TxDataState = ESP8266_DATA_TRANSMIT_IDLE;
        esp_debug_print("ESP8266: Send Data\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        for(i=0; i<ESP8266_SOCKETS_MAX; i++)
        {
            if((pESP->Socket[i].State == eSocketState::Connected) &&
                   (pESP->Socket[i].TxLock == true))
            {
                SocketId = i;
                pESP->Socket[i].TxState = eSocketSendDataStatus::InProgress;
                pESP->STEP = 1;
                return;
            }
        }
        break;

    case 1:
        if(pESP->Socket[SocketId].TxDataLen <= pESP->IO.TxPacketMaxSize)
        {
            pESP->Socket[SocketId].TxPacketLen = pESP->Socket[SocketId].TxDataLen;
        }
        else
        {
            pESP->Socket[SocketId].TxPacketLen = pESP->IO.TxPacketMaxSize;
        }

        if(pESP->Module.ConnectionTypeActual == eModuleConnectionType::Single)
        {
            len = snprintf(pESP->IO.pCommandString, 21, "AT+CIPSEND=%u\r\n", pESP->Socket[SocketId].TxPacketLen);
        }
        else if (pESP->Module.ConnectionTypeActual == eModuleConnectionType::Multiple)
        {
            len = snprintf(pESP->IO.pCommandString, 21, "AT+CIPSEND=%u,%u\r\n", SocketId, pESP->Socket[SocketId].TxPacketLen);
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
            return;
        }

        //pESP->DebugFlag_RxStreamToStdOut = true;

        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
        {
            //esp_debug_print("ESP: Send Data header (St1), len=%u\n", pESP->Socket[SocketId].TxPacketLen);
            pESP->StateTimer.Set(_3sec_);
            pESP->StateTimer.Reset();
            pESP->STEP = 2;
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 2:
        if(pESP->isCommandReceived(eAT::OK))
        {
            // Do nothing, wait for request for the data from ESP
        }
        else if(pESP->StateTimer.Elapsed() && pESP->IO.ListenToTxData == false)
        {
            pESP->Socket[SocketId].TxState = eSocketSendDataStatus::SendFail;
            pESP->Socket[SocketId].TxDataLen = 0;
            pESP->Socket[SocketId].TxLock = 0;
            pESP->CurrentState = &pESP->smStandby;

            esp_debug_print("ESP: No '>' received\n");

            //pESP->DebugFlag_RxStreamToStdOut = false;
        }

        if(pESP->IO.ListenToTxData)    //  module switched to data mode and ready to receive data
        {
            len = (unsigned int)ESP_TransmitBufferSpaceLeft(pESP->HuartNumber);
            if(len > pESP->IO.TxPacketMaxSize) { len = pESP->IO.TxPacketMaxSize; }  // packets longer than 2048 bytes must be separated on pieces with min 20ms delay between them
            if(len != 0 && len < pESP->Socket[SocketId].TxPacketLen)
            {
                if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)pESP->Socket[SocketId].DataTx, len))
                {
                    pESP->Socket[SocketId].DataTx += len;
                    pESP->Socket[SocketId].TxPacketLen -= len;
                    pESP->Socket[SocketId].TxDataLen -= len;
                    pESP->StateTimer.Set(_30ms_);   //  time to send at least part of data out (not time-out)
                    pESP->StateTimer.Reset();
                    //esp_debug_print("ESP: Sent part of data (St2), len=%u\n", len);
                    pESP->STEP = 3;
                }
                else
                {
                    pESP->CurrentState = &pESP->smModuleReset;
                }
            }
            else if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)pESP->Socket[SocketId].DataTx, pESP->Socket[SocketId].TxPacketLen))
            {
                pESP->StateTimer.Set(pESP->DataSendTimeout);  //  Time to send data through TCP, not sure if time-out is correct
                pESP->StateTimer.Reset();
                pESP->IO.ListenToTxData = false;
                //esp_debug_print("ESP: sent packet (St2)\n");
                pESP->Socket[SocketId].TxDataLen -= pESP->Socket[SocketId].TxPacketLen;
                pESP->STEP = 4;
            }
            else
            {
                pESP->CurrentState = &pESP->smModuleReset;
            }
        }

        if(pESP->isCommandReceived(eAT::FAIL) ||
           pESP->isCommandReceived(eAT::AT_ERROR) ||
           pESP->isCommandReceived(eAT::UNLINK))
        {
            pESP->Socket[SocketId].TxState = eSocketSendDataStatus::SendFail;
            esp_debug_print("ESP: SEND FAIL!\n");
            pESP->Socket[SocketId].TxDataLen = 0;
            pESP->Socket[SocketId].TxLock = 0;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->isCommandReceived(eAT::BUSY) ||
           pESP->isCommandReceived(eAT::BUSY_P) ||
           pESP->isCommandReceived(eAT::BUSY_S))
        {
            esp_debug_print("ESP8266: Send Retry, %d\n", RetryCounter);
            pESP->StateTimer.Set(_1sec_);
            pESP->StateTimer.Reset();
            RetryCounter--;
            pESP->STEP = 5;
        }

        break;

    case 3:
        //pESP->DebugFlag_RxStreamToStdOut = false;

        if(pESP->StateTimer.Elapsed())
        {
            if(pESP->IO.ListenToTxData)    //  module switched to data mode and ready to receive data
            {
                len = (unsigned int)ESP_TransmitBufferSpaceLeft(pESP->HuartNumber);
                if(len > pESP->IO.TxPacketMaxSize) { len = pESP->IO.TxPacketMaxSize; }  // packets longer than 2048 bytes must be separated on pieces with min 20ms delay between them
                if(len && len < pESP->Socket[SocketId].TxPacketLen)
                {
                    if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)pESP->Socket[SocketId].DataTx, len))
                    {
                        pESP->Socket[SocketId].DataTx += len;
                        pESP->Socket[SocketId].TxPacketLen -= len;
                        pESP->Socket[SocketId].TxDataLen -= len;
                        pESP->StateTimer.Set(_30ms_);
                        pESP->StateTimer.Reset();
                        //esp_debug_print("ESP: Sent part of packet (St3), len=%u\n", len);
                        break;
                    }
                    else
                    {
                        pESP->CurrentState = &pESP->smModuleReset;
                    }
                }
                else if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)pESP->Socket[SocketId].DataTx, pESP->Socket[SocketId].TxPacketLen))
                {
                    pESP->StateTimer.Set(pESP->DataSendTimeout);  //  Time to send data through TCP, not sure if time-out is correct
                    pESP->StateTimer.Reset();
                    pESP->IO.ListenToTxData = 0;
                    //esp_debug_print("ESP: Rest of packet sent (St3), len=%u\n", pESP->Socket[SocketId].TxPacketLen);
                    pESP->Socket[SocketId].TxDataLen -= pESP->Socket[SocketId].TxPacketLen;
                    //esp_debug_print("ESP: Rest of message: %u\n", pESP->Socket[SocketId].TxDataLen);
                    pESP->STEP = 4;
                    break;
                }
                else
                {
                    pESP->CurrentState = &pESP->smModuleReset;
                }
            }
            else
            {
                pESP->STEP = 4;
            }
        }
        break;

    case 4:
        //pESP->DebugFlag_RxStreamToStdOut = false;

        if(pESP->StateTimer.Elapsed())
        {
            esp_debug_print("ESP: Data Send Timeout\n");
        }

        if(pESP->isCommandReceived(eAT::FAIL) ||
           pESP->isCommandReceived(eAT::AT_ERROR) ||
           pESP->isCommandReceived(eAT::UNLINK))
        {
            esp_debug_print("ESP: Data Send Fail\n");
            // TODO
        }

        if(pESP->isCommandReceived(eAT::SEND_OK))
        {
            if(pESP->Socket[SocketId].TxDataLen)
            {
                //esp_debug_print("ESP: (St4), data left: %u\n", pESP->Socket[SocketId].TxDataLen);
                pESP->StateTimer.Set(_30ms_);   //  wait min 20ms between packets (demand from ESP)
                pESP->StateTimer.Reset();
                pESP->STEP = 6;
                break;
            }
            else
            {
                pESP->Socket[SocketId].TxLock = 0;
                pESP->Socket[SocketId].TxState = eSocketSendDataStatus::SendSuccess;
                esp_debug_print("ESP: Data Send OK\n");
                if(pESP->Socket[SocketId].CloseAfterSending) { pESP->CloseSocket(SocketId); }
                pESP->CurrentState = &pESP->smStandby;
            }
        }
        break;

    case 5:
        //pESP->DebugFlag_RxStreamToStdOut = false;

        if(pESP->StateTimer.Elapsed())
        {
            if(RetryCounter)
            {
                esp_debug_print("ESP8266: Data Send Retry %d\n", RetryCounter);
                pESP->StateTimer.Reset();
                pESP->STEP = 1;
            }
            else
            {
                esp_debug_print("ESP8266: Data Send FAIL\n");
                pESP->Socket[SocketId].TxState = eSocketSendDataStatus::SendFail;
                pESP->Socket[SocketId].TxDataLen = 0;
                pESP->Socket[SocketId].TxLock = 0;
                esp_debug_print("ESP: Data Send Fail\n");
                pESP->CurrentState = &pESP->smStandby;
            }
        }
        break;

    case 6:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->STEP = 1; //  send next packet
        }
        break;

    default:
      pESP->STEP = 0;
        break;
    }
}

void ESP::StateOpenSocket::Process(ESP* pESP)
{
char * str;
U32 len;
U8 i;

    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      esp_debug_print("ESP8266: Connect Socket\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        for(i=0; i<ESP8266_SOCKETS_MAX; i++)
        {
            if(pESP->Socket[i].State == eSocketState::ConnectRequested)
            {
                SocketId = i;
                pESP->STEP = 1;
                return;
            }
        }
        pESP->CurrentState = &pESP->smStandby;
        break;

    case 1:
        if(pESP->Socket[SocketId].State == eSocketState::ConnectRequested)
        {
            // Verify arguments
            if( pESP->Socket[SocketId].Port != 0 &&
                ((pESP->Socket[SocketId].Type == eSocketType::UDP) || (pESP->Socket[SocketId].Type == eSocketType::TCP)) )
            {
                len = strlen(pESP->Socket[SocketId].Address);
                if(0 == len)
                {
                    pESP->Socket[SocketId].State = eSocketState::Closed;
                    pESP->CurrentState = &pESP->smStandby;
                    return;
                }
                len += 40;
                str = (char*)malloc(len);
                if(str)
                {
                    if(pESP->Socket[SocketId].Type == eSocketType::UDP)
                    {
                        snprintf(str, len-1, "AT+CIPSTART=\"UDP\",\"%s\",%u\r\n", pESP->Socket[SocketId].Address, pESP->Socket[SocketId].Port);
                    }
                    else
                    {
                        snprintf(str, len-1, "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", pESP->Socket[SocketId].Address, pESP->Socket[SocketId].Port);
                    }

                    pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::NoError;
                    if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)str, strlen(str)))
                    {
                        pESP->StateTimer.Set(_200ms_);
                        pESP->StateTimer.Reset();
                        pESP->Socket[SocketId].State = eSocketState::Connecting;
                        pESP->STEP = 2;
                    }
                    else
                    {
                        pESP->Socket[SocketId].State = eSocketState::Closed;
                        pESP->Socket[SocketId].Address[0] = 0;      //  empty name string
                        pESP->Socket[SocketId].Port = 0;            //  empty port num
                        pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::InternalError;
                        pESP->CurrentState = &pESP->smStandby;
                    }
                    memmgr_free(str);
                }
                else    //  cannot allocate memory
                {
                    pESP->Socket[SocketId].State = eSocketState::Closed;
                    pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::InternalError;
                    pESP->CurrentState = &pESP->smStandby;
                    return;
                }
            }
            else
            {
                pESP->Socket[SocketId].State = eSocketState::Closed;
                pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::InternalError;;
                pESP->CurrentState = &pESP->smStandby;
            }
        }
        break;

    case 2:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->Socket[SocketId].State = eSocketState::Closed;
            pESP->Socket[SocketId].Address[0] = 0;      //  empty name string
            pESP->Socket[SocketId].Port = 0;            //  empty port
            pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::Timeout;
            pESP->STEP = 0;
        }
        else if(pESP->isCommandReceived(eAT::OK))
        {
            pESP->Socket[SocketId].State = eSocketState::Connected;
            pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::NoError;
            pESP->CurrentState = &pESP->smStandby;
        }
        else if(pESP->isCommandReceived(eAT::NOIP))
        {
            for(i=0; i<ESP8266_SOCKETS_MAX; i++)
            {
                pESP->Socket[SocketId].State = eSocketState::Closed;
                pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::NoAccessPoint;
            }
            pESP->RemoteAP.StationState = eStationConnectionState::Disconnected;
            pESP->CurrentState = &pESP->smJoinAP;
        }
        else if(pESP->isCommandReceived(eAT::AT_ERROR))
        {
            pESP->Socket[SocketId].State = eSocketState::Closed;
            pESP->Socket[SocketId].ErrorFlag = eSocketErrorFlag::FailToConnect;
            pESP->CurrentState = &pESP->smStandby;
        }
        break;

    default:
      pESP->STEP = 0;
        break;
    }
}

void ESP::StateCloseSocket::Process(ESP* pESP)
{
uint8_t i;

    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      //esp_debug_print("ESP8266: Close Socket\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        for(i=0; i<ESP8266_SOCKETS_MAX; i++)
        {
            if(pESP->Socket[i].State == eSocketState::CloseRequested)
            {
                SocketId = i;
                esp_debug_print("ESP8266: Close Socket %d\n", i);
                pESP->STEP = 1;
                return;
            }
        }
        pESP->CurrentState = &pESP->smStandby;
        break;

    case 1:
        if(pESP->Socket[SocketId].State == eSocketState::CloseRequested)
        {
            sprintf(pESP->IO.pCommandString, "AT+CIPCLOSE=%u\r\n", SocketId);
            if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, strlen(pESP->IO.pCommandString)))
            {
                pESP->StateTimer.Set(_100ms_);
                pESP->StateTimer.Reset();
                pESP->Socket[SocketId].State = eSocketState::Closing;
                pESP->STEP = 2;
            }
            else
            {
                pESP->CurrentState = &pESP->smModuleReset;
            }
        }
        break;

    case 2:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->Socket[SocketId].State = eSocketState::Error;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->isCommandReceived(eAT::OK))
        {
            pESP->Socket[SocketId].State = eSocketState::Closed;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->isCommandReceived(eAT::AT_ERROR))  //  according to documentation, modem returns ERROR if there is no connection
        {
            pESP->Socket[SocketId].State = eSocketState::Closed;
            pESP->CurrentState = &pESP->smStandby;
        }

        break;

    default:
        pESP->STEP = 0;
        break;
    }
}

void ESP::StateStartServer::Process(ESP* pESP)
{
uint16_t len;

    if(pESP->StateMachineStateChanged())
    {
        pESP->STEP = 0;
        pESP->Server.State = eServerState::Connecting;
        pESP->ClearLastCommand();
        esp_debug_print("ESP8266: Start Server\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize-1, "AT+CIPSERVER=1,%u\r\n", pESP->Server.Port);

        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
        {
            pESP->StateTimer.Set(_1sec_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
            esp_debug_print("ESP: Cmd send Fail,%d\n", __LINE__);
        }
        break;

    case 1:
        if(pESP->StateTimer.Elapsed())
        {
            pESP->Server.State = eServerState::ConnectTimeout;  //  can't connect
            esp_debug_print("ESP8266: Start Server Timeout\n");
            pESP->Server.StartRequest = false;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->isCommandReceived(eAT::OK))
        {
            //pESP->WiFi_State = WiFi_GetConnectionsInfo;   // TODO now need to get status from the device
            pESP->Server.State = eServerState::Connected;
            pESP->Server.StartRequest = false;
            esp_debug_print("ESP8266: Server Started\n");
            pESP->CurrentState = &pESP->smGetApIP;
        }

        if(pESP->isCommandReceived(eAT::NOCHANGE))
        {
            //pESP->WiFi_State = WiFi_GetConnectionsInfo;   // TODO now need to get status from the device
            pESP->Server.State = eServerState::Connected;
            esp_debug_print("ESP8266: Server Started\n");
            pESP->StateTimer.Set(_100ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 2;   //  wait for OK and then exit
        }

        if(pESP->isCommandReceived(eAT::AT_ERROR))
        {
            pESP->Server.State = eServerState::Error;
            pESP->Server.StartRequest = false;
            pESP->CurrentState = &pESP->smStandby;   //  now need to get status from the device
        }
        break;

    case 2:

        if(pESP->isCommandReceived(eAT::OK) || pESP->StateTimer.Elapsed())
        {
            pESP->Server.StartRequest = false;
            pESP->CurrentState = &pESP->smGetApIP;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }
}

void ESP::StateGetApIP::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      esp_debug_print("ESP8266: Get Ap IP\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, (char*)AT_CIFSR, sizeof(AT_CIFSR)-1))
        {
            pESP->StateTimer.Set(_200ms_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
        }
        break;

    case 1:
        if(pESP->isCommandReceived(eAT::CIFSR_APIP))
        {
            pESP->LocalAP.IP = (((uint32_t)pESP->IO.pReceivedParameter[3])                  |
                                ((uint32_t)pESP->IO.pReceivedParameter[2] << 8  & 0xFF00)   |
                                ((uint32_t)pESP->IO.pReceivedParameter[1] << 16 & 0xFF0000) |
                                ((uint32_t)pESP->IO.pReceivedParameter[0] << 24 & 0xFF000000));

            esp_debug_print("ESP8266: AP IP[hex]:%x\n", (unsigned int)pESP->LocalAP.IP);
        }

        if(pESP->isCommandReceived(eAT::CIFSR_APMAC))
        {
            pESP->LocalAP.MAC = ( (uint64_t)pESP->IO.pReceivedParameter[5] & 0xFF) |
                                  (((uint64_t)pESP->IO.pReceivedParameter[4] << 8) & 0xFF00)   |
                                  (((uint64_t)pESP->IO.pReceivedParameter[3] << 16) & 0xFF0000) |
                                  (((uint64_t)pESP->IO.pReceivedParameter[2] << 24) & 0xFF000000) |
                                  (((uint64_t)pESP->IO.pReceivedParameter[1] << 32) & 0xFF00000000) |
                                  (((uint64_t)pESP->IO.pReceivedParameter[0] << 40) & 0xFF0000000000);

            esp_debug_print("ESP8266: AP MAC:%x%x\n", (unsigned int)(pESP->LocalAP.MAC >> 32) , (unsigned int)pESP->LocalAP.MAC);

            pESP->STEP = 2; //  wait for OK
        }
        break;

    case 2:
        if(pESP->isCommandReceived(eAT::OK) || pESP->StateTimer.Elapsed())
        {
            pESP->CurrentState = &pESP->smStandby;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }

}

void ESP::StateChangeAPIP::Process(ESP* pESP)
{
int len;

    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      esp_debug_print("ESP8266: Set AP IP\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        len = snprintf(pESP->IO.pCommandString, pESP->IO.CommandStringSize-1, "AT+CIPAP_CUR=\"%u.%u.%u.%u\",\"%u.%u.%u.%u\",\"%u.%u.%u.%u\"\r\n",
                                                                                (unsigned int)((pESP->LocalAP.NewIP >> 24) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewIP >> 16) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewIP >> 8)  & 0xFF),
                                                                                (unsigned int)( pESP->LocalAP.NewIP        & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewGateway >> 24) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewGateway >> 16) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewGateway >> 8)  & 0xFF),
                                                                                (unsigned int)( pESP->LocalAP.NewGateway        & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewNetMask >> 24) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewNetMask >> 16) & 0xFF),
                                                                                (unsigned int)((pESP->LocalAP.NewNetMask >> 8)  & 0xFF),
                                                                                (unsigned int)( pESP->LocalAP.NewNetMask        & 0xFF) );
        //esp_debug_print("ESP8266: CH.IP.Str:%s\n", pESP->IO.pCommandString);
        //pESP->DebugFlag_RxStreamToStdOut = true;

        if(SUCCESS == ESP_HuartSend(pESP->HuartNumber, pESP->IO.pCommandString, len))
        {
            pESP->StateTimer.Set(_5sec_);
            pESP->StateTimer.Reset();
            pESP->STEP = 1;
        }
        else
        {
            pESP->CurrentState = &pESP->smModuleReset;
            esp_debug_print("ESP: Cmd send Fail,%d\n", __LINE__);
        }

        break;

    case 1:
        if(pESP->isCommandReceived(eAT::OK))
        {
            esp_debug_print("ESP8266: New IP set successfully\n");
            pESP->LocalAP.IP      = pESP->LocalAP.NewIP;
            pESP->LocalAP.Gateway = pESP->LocalAP.NewGateway;
            pESP->LocalAP.NetMask = pESP->LocalAP.NewNetMask;
            pESP->LocalAP.ChangeIPRequest = false;
            pESP->CurrentState = &pESP->smStandby;
        }

        if(pESP->StateTimer.Elapsed())
        {
            pESP->LocalAP.IP   = 0;
            pESP->LocalAP.Gateway = 0;
            pESP->LocalAP.NetMask = 0;
            pESP->LocalAP.ChangeIPRequest = false;
            esp_debug_print("ESP8266: New IP set Timeout!\n");
            // TODO: report about error here
            pESP->CurrentState = &pESP->smStandby;
        }
        break;

    default:
        pESP->STEP = 0;
        break;
    }

}

void ESP::StateGetConnectionsInfo::Process(ESP* pESP)
{
    if(pESP->StateMachineStateChanged())
    {
      pESP->STEP = 0;
      pESP->ClearLastCommand();
      esp_debug_print("ESP8266: Get Connection Info\n");
    }

    switch(pESP->STEP)
    {
    case 0:
        //TODO: AT+CIPSTATUS
        break;

    default:
        pESP->STEP = 0;
        break;
    }

}

/*
 *
 *
 *
 * TODO
 *
 *
 *
 *
 *
 *
 *
 *TODO
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *TODO
 *
 *
 *
 *
 *
 *TODO
 *
 * */

/*********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************
 *                      Receive Stream Handler
 *********************************************************************************************************************************************************************
 *********************************************************************************************************************************************************************/

void ESP::RxHandler(void)
{
uint8_t tmpU8, res;
unsigned int data_len, id;
char str[6];
U8 NoMatchFound = 0;

  //circular_buffer* cb_Rx;

  //cb_Rx = (circular_buffer*)(pESP->pHuart->pRxBuffPtr);

  if(IO.ReceiveError)       // clear all data until this flag not cleared with module re-init or other actions
  {
    while(SUCCESS == ESP_GetChar(HuartNumber, &tmpU8)){}
  }

  if(IO.DoEmptyRxStream)
  {
      while(SUCCESS == ESP_GetChar(HuartNumber, &tmpU8)){}
      IO.DoEmptyRxStream = false;
      IO.RxBuffCounter = 0;
      IO.ReceiveError = false;
  }

  if(IO.RxIgnoreCounter)           // Ignore too long messages or data echo
  {
    while(SUCCESS == ESP_GetChar(HuartNumber, &tmpU8))
    {
       if(DebugFlag_RxStreamToStdOut) { esp_debug_print("!%c", tmpU8); }     //  copy all data from ESP to std out

       IO.RxIgnoreCounter--;
       if(0 == IO.RxIgnoreCounter)
       {
          if(IO.ListenToTxData) { IO.ListenToTxData = 0; }
          return;
       }
    }
    return;
  }

  if(IO.ReceivingDataStream)
  {
     if(IO.RxOverflowEvent)
     {
         IO.RxOverflowEvent = 0;
         if(IO.CurrentSocketDataLeft > ESP_NumOfDataReceived(HuartNumber))     // grab only what is currently stored in buffer
         {
             IO.CurrentSocketDataLeft = ESP_NumOfDataReceived(HuartNumber);
             Socket[IO.RxSocketId].DataCutFlag = true;
         }
     }

     while(SUCCESS == ESP_GetChar(HuartNumber, IO.pCurrentSocketData))
     {
        if(DebugFlag_RxStreamToStdOut) { esp_debug_print("%c", *IO.pCurrentSocketData); }     //  copy all data from ESP to std out

        IO.pCurrentSocketData++;
        IO.CurrentSocketDataLeft--;
        Socket[IO.RxSocketId].RxDataLen++;
/*
        if(*IO.pCurrentSocketData == '9' &&
                *IO.pCurrentSocketData-1 == '.' &&
                *IO.pCurrentSocketData-2 == '0' &&
                *IO.pCurrentSocketData-3 == '=' &&
                *IO.pCurrentSocketData-4 == 'q')
        {
            esp_debug_print("!!!");
        }
*/
        if(0 == IO.CurrentSocketDataLeft)
        {
           IO.ReceivingDataStream = false;
           Socket[IO.RxSocketId].RxLock = true;  //  Lock data for application
           return;
        }
        if(Socket[IO.RxSocketId].RxDataLen >= Socket[IO.RxSocketId].RxBuffSize) //  Rx buffer is less than incoming data. Lock all data currently received and ignore rest
        {
            Socket[IO.RxSocketId].RxLock = true;  //  Lock data for application
            IO.RxIgnoreCounter = IO.CurrentSocketDataLeft;
            IO.ReceivingDataStream = false;
            //esp_debug_print("|RxIgnoreCounter-1=%u|", IO.RxIgnoreCounter);
            Socket[IO.RxSocketId].DataCutFlag = true;
            esp_debug_print("ESP: Rx Data has been cut out! RxDataLen=%u, RxBuffSize=%u\n", Socket[IO.RxSocketId].RxDataLen, Socket[IO.RxSocketId].RxBuffSize);
            return;
        }
     }
     return;
  }
  else
  {
    // return if previous command has not been processed
    if(NewCommandSemaphore)
    {
        return;
    }

    // receive and parse incoming command
    while(SUCCESS == ESP_GetChar(HuartNumber, (unsigned char*)(IO.pRxBuffer + IO.RxBuffCounter)))
    {
       if(DebugFlag_RxStreamToStdOut)      //  copy all data from ESP to std out
       {
           //if(*(IO.pRxBuffer + IO.RxBuffCounter) == '\r') esp_debug_print("\\r\r");
           //else if(*(IO.pRxBuffer + IO.RxBuffCounter) == '\n') esp_debug_print("\\n\n");
           //else
               esp_debug_print("%c", *(IO.pRxBuffer + IO.RxBuffCounter));
       }

       IO.RxBuffCounter++;
       if(IO.RxBuffCounter > ESP8266_RX_BUFF_LEN - 1)
       {
         IO.RxBuffCounter = 0;
         IO.ReceiveError = true;
          esp_debug_print("ESP Rx Overflow\n");
          continue;
       }

       *(IO.pRxBuffer + IO.RxBuffCounter) = 0;  // make a null-terminated string
       //============ Receive Socket Data in multi-mode =============
       if((IO.pRxBuffer[0] == '+') &&
          (IO.pRxBuffer[1] == 'I') )
       {
         res = sscanf(IO.pRxBuffer, "+IPD,%u,%u%1s", &id, &data_len, IO.pReceivedParameterStr);
         if(3 == res)
         {
             esp_debug_print("ESP: IPD DATA, Socket=%d, Len:%d\n", id, data_len);
            *IO.pRxBuffer = 0;   //  delete +IPD header
            IO.RxBuffCounter = 0;    //  prepare buffer for the next command

            if(id>(ESP8266_SOCKETS_MAX-1))  // wrong ID
            {
               IO.RxIgnoreCounter = data_len;
               return;
            }

            if((Socket[id].State == eSocketState::Closed)   ||
               (Socket[id].DataRx == 0)                     ||
                Socket[id].RxLock                           ||
                Socket[id].RxBuffSize == 0)
            {
                esp_debug_print("\nRxIgnoreCounter=%u\n", (unsigned int)IO.RxIgnoreCounter);
               IO.RxIgnoreCounter = data_len;
               return;
            }
/*
            if(Socket[id].RxBuffSize < data_len)
            {
                IO.CurrentSocketDataLeft = Socket[id].RxBuffSize;
            }
*/
            IO.pCurrentSocketData = Socket[id].DataRx;
            IO.CurrentSocketDataLeft = data_len;
            Socket[id].RxDataLen = 0;
            Socket[id].DataCutFlag = false;
            IO.ReceivingDataStream = true;
            IO.RxSocketId = id;
            return;
         }
         //============ Receive Socket Data in single-mode =============
//TODO work on following commented
#if 0
         res = sscanf(IO.pRxBuffer, "+IPD,%u:", &data_len);       //  First, look if new data coming (async process)
         if(1 == res)
         {
            *IO.pRxBuffer = 0;   //  delete +IPD header
            IO.RxBuffCounter = 0;

            if((Socket[0].State != ESP8266_SOCKET_OPEN) ||
               (Socket[0].DataRx == 0) ||
                Socket[0].RxLock ||
               (Socket[0].RxBuffSize < data_len) )
            {
               IO.RxIgnoreCounter = data_len;
               continue;
            }

            IO.pCurrentSocketData = Socket[0].DataRx;
            IO.CurrentSocketDataLeft = data_len;
            Socket[0].RxDataLen = 0;
            IO.ReceivingDataStream = 1;
            IO.RxSocketId = 0;
            continue;
         }
#endif
       }
       if((IO.pRxBuffer[0] == '>') &&
          (IO.pRxBuffer[1] == ' ') )
       {
           IO.ListenToTxData = true;
           IO.RxBuffCounter = 0;    //  prepare buffer for the next command
           return;
       }
       //=========== Parse command ==================
       if(IO.RxBuffCounter >= 3)         //      at least 3 bytes received
       {
          if((*(IO.pRxBuffer + IO.RxBuffCounter - 1) == '\n') &&
             (*(IO.pRxBuffer + IO.RxBuffCounter - 2) == '\r') &&
             (*(IO.pRxBuffer + IO.RxBuffCounter - 3) == '\r'))     //  echo received
          //============ Receive Echo =============
          {
            //esp_debug_print("ESP Rx1:Len=%d; Msg:%40s\n",IO.RxBuffCounter, IO.pRxBuffer);
              if(IO.RxBuffCounter == 3)
              {
                IO.RxBuffCounter = 0;    //  prepare buffer for the next command
                return;
              }
              switch(*IO.pRxBuffer)
              {
              case 'A':
                  switch(*(IO.pRxBuffer + 1))
                  {
                  // AT
                  case 'T':
                      switch(*(IO.pRxBuffer + 2))
                      {
                      case '\r':
                          if(strstr((const char *)IO.pRxBuffer, "AT\r\r\n")) { StoreCommand( eAT::ECHO_AT ); }
                          else { NoMatchFound = 1; }
                          break;
                      // ATE
                      case 'E':
                          if     (strstr((const char *)IO.pRxBuffer, "ATE0\r\r\n"))   { StoreCommand( eAT::ECHO_ECHO_OFF ); }
                          else if(strstr((const char *)IO.pRxBuffer, "ATE1\r\r\n"))   { StoreCommand( eAT::ECHO_ECHO_ON  ); }
                          else { NoMatchFound = 1; }
                          break;
                      // END ATE

                      default:
                          NoMatchFound = 1;
                          break;
                      } // END AT
                      break;

                  default:
                      NoMatchFound = 1;
                      break;
                  }// END A
                  break;

              default:
                  NoMatchFound = 1;
                  break;
              }

              if(NoMatchFound)
              {
                  esp_debug_print("ESP UNKNOWN MESSAGE.Len=%d; Msg:%40s",IO.RxBuffCounter, IO.pRxBuffer);

                  IO.RxBuffCounter = 0;    //   prepare buffer for the next command
              }
              else
              {
                  IO.RxBuffCounter = 0;    //   prepare buffer for the next command
              }
          }
/*******************************************************************************************************************************************************************************************/
          if(NewCommandSemaphore) { return; }   //  previous command has not beeb parsed
          //============ Receive Response =============
          else if((*(IO.pRxBuffer + IO.RxBuffCounter - 1) == '\n') &&
                  (*(IO.pRxBuffer + IO.RxBuffCounter - 2) == '\r'))     //  new line received
          {
            //esp_debug_print("ESP Rx2:Len=%d; Msg:%40s\n",IO.RxBuffCounter, IO.pRxBuffer);
            if(IO.RxBuffCounter == 2)
              {
                IO.RxBuffCounter = 0;    //  prepare buffer for the next command
                return;
              }
              //    First try to parse string with assumption that message starts from zero position in reseived message, otherwise NoMatchFound flag should be set and advanced parsing start
              switch(*IO.pRxBuffer)
              {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
                  if(0 == strcmp((const char *)(IO.pRxBuffer + 1), ",CONNECT\r\n"))
                  {
                      StoreCommand(eAT::SOCKET_CONNECT);
                      //esp_debug_print("#%d\n", *IO.pRxBuffer);
                      switch(*IO.pRxBuffer)
                      {
                      case '0': IO.pReceivedParameter[0] = 0; break;
                      case '1': IO.pReceivedParameter[0] = 1; break;
                      case '2': IO.pReceivedParameter[0] = 2; break;
                      case '3': IO.pReceivedParameter[0] = 3; break;
                      case '4': IO.pReceivedParameter[0] = 4; break;
                      default: IO.pReceivedParameter[0]  = 255; break;
                      }
                  }
                  else if(0 == strcmp((const char *)(IO.pRxBuffer + 1), ",CLOSED\r\n"))
                  {
                      StoreCommand(eAT::SOCKET_CLOSED);
                      //esp_debug_print("#%d\n", *IO.pRxBuffer);
                      switch(*IO.pRxBuffer)
                      {
                      case '0': IO.pReceivedParameter[0] = 0; break;
                      case '1': IO.pReceivedParameter[0] = 1; break;
                      case '2': IO.pReceivedParameter[0] = 2; break;
                      case '3': IO.pReceivedParameter[0] = 3; break;
                      case '4': IO.pReceivedParameter[0] = 4; break;
                      default: IO.pReceivedParameter[0]  = 255; break;
                      }
                  }
                  else if(0 == strcmp((const char *)(IO.pRxBuffer + 1), ",CONNECT FAIL\r\n"))
                  {
                      StoreCommand(eAT::SOCKET_CONNECT_FAIL);
                      switch(*IO.pRxBuffer)
                      {
                      case '0': IO.pReceivedParameter[0] = 0; break;
                      case '1': IO.pReceivedParameter[0] = 1; break;
                      case '2': IO.pReceivedParameter[0] = 2; break;
                      case '3': IO.pReceivedParameter[0] = 3; break;
                      case '4': IO.pReceivedParameter[0] = 4; break;
                      default: IO.pReceivedParameter[0]  = 255; break;
                      }
                  }
                  else { NoMatchFound = 1; }
                  break;

              case 'A':
                  if     (strstr((const char *)IO.pRxBuffer, "ALREAY CONNECT\r\n"))  { StoreCommand(eAT::ALREADY_CONNECT ); }  // this is real message with grammatical error
                  else if(strstr((const char *)IO.pRxBuffer, "ALREADY CONNECT\r\n")) { StoreCommand(eAT::ALREADY_CONNECT ); } // this message can appear if manufacturer fix gramm. error
                  else { NoMatchFound = 1; }
                  break;

              case 'b':
                  if     (strstr((const char *)IO.pRxBuffer, "busy\r\n"))      { StoreCommand(eAT::BUSY   ); }
                  else if(strstr((const char *)IO.pRxBuffer, "busy p...\r\n")) { StoreCommand(eAT::BUSY_P ); }
                  else if(strstr((const char *)IO.pRxBuffer, "busy s...\r\n")) { StoreCommand(eAT::BUSY_S ); }
                  else { NoMatchFound = 1; }
                  break;

              case 'B':
                  if(strstr((const char *)IO.pRxBuffer, "BAUD->"))
                  {
                      res = sscanf(IO.pRxBuffer, "BAUD->%u", &IO.pReceivedParameter[0]);
                      if(1 == res) { StoreCommand( eAT::BAUDRATE_CONFIRMATION); }
                  }
                  else { NoMatchFound = 1; }
                  break;

              case 'E':
                  if(strstr((const char *)IO.pRxBuffer, "ERROR\r\n")) { StoreCommand(eAT::AT_ERROR ); }
                  else { NoMatchFound = 1; }
                  break;

              case 'F':
                  if(strstr((const char *)IO.pRxBuffer, "FAIL\r\n")) { StoreCommand(eAT::FAIL ); }
                  else { NoMatchFound = 1; }
                  break;

              case 'l':
              case 'L':
                  if(strstr((const char *)IO.pRxBuffer, "Linked\r\n"))               { StoreCommand(eAT::LINKED       ); }
                  else if(strstr((const char *)IO.pRxBuffer, "link is not\r\n"))     { StoreCommand(eAT::LINKISNOT    ); }
                  else if(strstr((const char *)IO.pRxBuffer, "link\r\n"))            { StoreCommand(eAT::LINK         ); }
                  else if(strstr((const char *)IO.pRxBuffer, "Link is builded\r\n")) { StoreCommand(eAT::LINKISBUILDED); }
                  else { NoMatchFound = 1; }
                  break;

              case 'n':
                  if     (strstr((const char *)IO.pRxBuffer, "no change\r\n"))   { StoreCommand(eAT::NOCHANGE ); }
                  else if(strstr((const char *)IO.pRxBuffer, "no ip\r\n"))       { StoreCommand(eAT::NOIP     ); }
                  else if(strstr((const char *)IO.pRxBuffer, "no this fun\r\n")) { StoreCommand(eAT::NO_THIS_FUNCTION); }
                  else { NoMatchFound = 1; }
                  break;

              case 'O':
                  if(strstr((const char *)IO.pRxBuffer, "OK\r\n")) { StoreCommand(eAT::OK); }
                  else { NoMatchFound = 1; }
                  break;

              case 'r':
                  if(strstr((const char *)IO.pRxBuffer, "ready\r\n")) { StoreCommand(eAT::READY); }
                  else { NoMatchFound = 1; }
                  break;

              case 'S':
                  if(strstr((const char *)IO.pRxBuffer, "SEND OK\r\n")) { StoreCommand(eAT::SEND_OK ); }
                  else if(strstr((const char *)IO.pRxBuffer, "STATUS:"))
                  {
                      res = sscanf(IO.pRxBuffer, "STATUS:%u", &IO.pReceivedParameter[0]);
                      if(1 == res) { StoreCommand(eAT::CIPSTATUS); }
                      else         { StoreCommand(eAT::BAD_STRUCTURE); }
                  }
                  else { NoMatchFound = 1; }
                  break;

              case 'w':
                  if     (strstr((const char *)IO.pRxBuffer, "wrong syntax\r\n")) { StoreCommand(eAT::WRONG_SYNTAX ); }
                  else if(strstr((const char *)IO.pRxBuffer, "wdt reset\r\n"))    { StoreCommand(eAT::WDT_RESET    ); }
                  else { NoMatchFound = 1; }
                  break;

              case 'u':
                  if(strstr((const char *)IO.pRxBuffer, "Unlink\r\n")) { StoreCommand(eAT::UNLINK ); }
                  else { NoMatchFound = 1; }
                  break;

              case '[':
                  if(strstr((const char *)IO.pRxBuffer, "[Vendor:www.ai-thinker.com Version:")) { StoreCommand(eAT::REBOOT_DETECTED ); }
                  else { NoMatchFound = 1; }
                  break;

              case '+':
                  switch(*(IO.pRxBuffer + 1))
                  {
                  // +C
                  case 'C':
                      switch(*(IO.pRxBuffer + 2))
                      {
                      // +CI
                      case 'I':
                          switch(*(IO.pRxBuffer + 3))
                          {
                          case 'F':
                              if(strstr((const char *)IO.pRxBuffer, "+CIFSR:APIP"))
                              {
                                  res = sscanf(IO.pRxBuffer, "+CIFSR:APIP,\"%u.%u.%u.%u", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1],
                                                                                                   &IO.pReceivedParameter[2], &IO.pReceivedParameter[3]);
                                  if(4 == res) { StoreCommand(eAT::CIFSR_APIP); }
                                  else         { StoreCommand(eAT::BAD_STRUCTURE); }
                              }
                              else if(strstr((const char *)IO.pRxBuffer, "+CIFSR:APMAC"))
                              {
                                  res = sscanf(IO.pRxBuffer, "+CIFSR:APMAC,\"%x:%x:%x:%x:%x:%x", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1],
                                                                                                   &IO.pReceivedParameter[2], &IO.pReceivedParameter[3],
                                                                                                   &IO.pReceivedParameter[4], &IO.pReceivedParameter[5]);
                                  if(6 == res) { StoreCommand(eAT::CIFSR_APMAC); }
                                  else         { StoreCommand(eAT::BAD_STRUCTURE); }
                              }
                              else
                              {
                                  NoMatchFound = 1;
                              }
                              break;  // END of +CIF

                          case 'O':
                              // +CIOBAUD:(X-X)
                              if(strstr((const char *)IO.pRxBuffer, "+CIOBAUD:("))
                              {
                                 if(ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM >= 2)
                                 {
                                    res = sscanf(IO.pRxBuffer, "+CIOBAUD:(%u-%u)", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1]);
                                    if(2 == res) { StoreCommand(eAT::CIOBAUD_RANGE); }
                                    else         { StoreCommand(eAT::BAD_STRUCTURE); }
                                 }
                                 else
                                 {
                                     StoreCommand(eAT::INTERNAL_ERROR);
                                 }
                              }
                              //         +CIOBAUD:X
                              else if(strstr((const char *)IO.pRxBuffer, "+CIOBAUD:"))
                              {
                                 if(ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM >= 1)
                                 {
                                    res = sscanf(IO.pRxBuffer, "+CIOBAUD:%u", &IO.pReceivedParameter[0]);
                                    if(1 == res) { StoreCommand(eAT::CIOBAUD); }
                                    else         { StoreCommand(eAT::BAD_STRUCTURE); }
                                 }
                                 else
                                 {
                                     StoreCommand(eAT::INTERNAL_ERROR);
                                 }
                              }
                              else
                              {
                                  NoMatchFound = 1;
                              }
                          break;    //  END of +CIO

                          // +CIP
                          case 'P':
                              switch(*(IO.pRxBuffer + 4))
                              {
                              // +CIPS
                              case 'S':
                                  // +CIPSTATUS
                                  if(strstr((const char *)IO.pRxBuffer, "+CIPSTATUS:"))
                                  {
                                      if(ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN > 80 && ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM >= 5)
                                      {
                                          res = sscanf(IO.pRxBuffer, "+CIPSTATUS:%u,\"%5[^\"]\",\"%80[^\"]\",%u,%u,%u",
                                                                                            &IO.pReceivedParameter[0], str, IO.pReceivedParameterStr, &IO.pReceivedParameter[2], &IO.pReceivedParameter[3], &IO.pReceivedParameter[4]);
                                          if(res == 6)
                                          {
                                              str[5] = 0;
                                              if(strstr(str, "TCP"))
                                              {
                                                  IO.pReceivedParameter[1] = (uint8_t)eSocketType::TCP;
                                              }
                                              else if(strstr(str, "UDP"))
                                              {
                                                  IO.pReceivedParameter[1] = (uint8_t)eSocketType::UDP;
                                              }
                                              else
                                              {
                                                  StoreCommand(eAT::BAD_STRUCTURE);
                                              }
                                          }
                                          else
                                          {
                                              StoreCommand(eAT::BAD_STRUCTURE);
                                          }
                                      }
                                      else
                                      {
                                          StoreCommand(eAT::INTERNAL_ERROR);
                                      }
                                  }
                                  else
                                  {
                                      NoMatchFound = 1;
                                  }
                                  break;

                              default:
                                  NoMatchFound = 1;
                              break;
                              } //  END of +CIP
                          break;

                          default:
                              NoMatchFound = 1;
                          break;
                          }
                      break; //  END of +CI

                      // +CW
                      case 'W':
                        switch(*(IO.pRxBuffer + 3))
                        {
                          // +CWJ
                          case 'J':
                              if(strstr((const char *)IO.pRxBuffer, "+CWJAP:"))  //  TODO: new data-set
                              {
                                  res = sscanf(IO.pRxBuffer, "+CWJAP:%u", &IO.pReceivedParameter[0]);
                                  if(1 == res) { StoreCommand(eAT::CWJAP_FAULT); }
                                  else         { StoreCommand(eAT::BAD_STRUCTURE); }
                              }
                              else if(strstr((const char *)IO.pRxBuffer, "+CWJAP_CUR:"))  //  TODO: new data-set
                              {
                                  if(ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN > 40)
                                  {
                                      res = sscanf(IO.pRxBuffer, "+CWJAP_CUR:(\"%40[^\"]\",\"%19[^\"]\",%u,%u", IO.pReceivedParameterStr,
                                                                                                                         IO.pReceivedParameterStr2,
                                                                                                                         &IO.pReceivedParameter[0],
                                                                                                                         &IO.pReceivedParameter[1]);
                                      if(4 == res) { StoreCommand(eAT::CWJAP); }
                                      else         { StoreCommand(eAT::BAD_STRUCTURE); }
                                  }
                                  else
                                  {
                                      StoreCommand(eAT::INTERNAL_ERROR );
                                  }
                              }
                              else
                              {
                                  NoMatchFound = 1;
                              }
                          break;  // END +CWJ

                          case 'L':
                            if(strstr((const char *)IO.pRxBuffer, "+CWLAP:("))  //
                            {
                                if(ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN > 80 && ESP8266_RECEIVED_COMMAND_PARAM_STR2_LEN >=20)
                                {
                                    res = sscanf(IO.pRxBuffer, "+CWLAP:(%u,\"%80[^\"]\",%d,\"%19[^\"]\",%u,%u,%u",
                                            &IO.pReceivedParameter[0], IO.pReceivedParameterStr, &IO.pReceivedParameter[1], IO.pReceivedParameterStr2,
                                            &IO.pReceivedParameter[2], &IO.pReceivedParameter[3], &IO.pReceivedParameter[4]);
                                    if(7 == res) { StoreCommand(eAT::CWLAP); }
                                    else         { StoreCommand(eAT::BAD_STRUCTURE); }
                                }
                                else
                                {
                                    StoreCommand(eAT::INTERNAL_ERROR );
                                }
                            }
                            else
                            {
                                NoMatchFound = 1;
                            }
                            break;

                          // +CWM
                          case 'M':
                              if(strstr((const char *)IO.pRxBuffer, "+CWMODE_CUR:"))
                              {
                                  res = sscanf(IO.pRxBuffer, "+CWJAP:%u", &IO.pReceivedParameter[0]);
                                  if(1 == res) { StoreCommand(eAT::CWMODE); }
                                  else         { StoreCommand(eAT::BAD_STRUCTURE); }
                              }
                              else
                              {
                                  NoMatchFound = 1;
                              }
                          break; // END +CWM

                          // +CWS
                          case 'S':
                              if(strstr((const char *)IO.pRxBuffer, "+CWSAP_CUR:"))
                              {
                                  if(ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN > 80)
                                  {
                                      // name and password are non-empty strings
                                      res = sscanf(IO.pRxBuffer, "+CWSAP_CUR:\"%80[^\"]\",\"%*[^\"]\",%u,%u,%u,%u",
                                              IO.pReceivedParameterStr, &IO.pReceivedParameter[0], &IO.pReceivedParameter[1], &IO.pReceivedParameter[2], &IO.pReceivedParameter[3]);
                                      if(5 == res) { StoreCommand(eAT::CWSAP_CUR); }
                                      else  //  name is non-empty string, password is empty string
                                      {
                                          res = sscanf(IO.pRxBuffer, "+CWSAP_CUR:\"%80[^\"]\",\"\",%u,%u,%u,%u",
                                                           IO.pReceivedParameterStr, &IO.pReceivedParameter[0], &IO.pReceivedParameter[1], &IO.pReceivedParameter[2], &IO.pReceivedParameter[3]);
                                          if(5 == res) { StoreCommand(eAT::CWSAP_CUR); }
                                          else         { StoreCommand(eAT::BAD_STRUCTURE); }
                                      }
                                  }
                                  else
                                  {
                                      StoreCommand(eAT::INTERNAL_ERROR);
                                  }
                              }
                              else
                              {
                                  NoMatchFound = 1;
                              }
                          break; // END +CWS

                          default:
                          NoMatchFound = 1;
                          break;
                        } //  END +CW
                        break;

                      default:
                          NoMatchFound = 1;
                          break;
                      } // END +C
                      break;

                  default:
                      NoMatchFound = 1;
                      break;
                  }  // END +
                  break;

              default:
                 /* if(5 == sscanf(IO.pRxBuffer, "%u.%u.%u.%u%c", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1], &IO.pReceivedParameter[2], &IO.pReceivedParameter[3], &c))
                  {
                      if(c == '\r') { StoreCommand(eAT::IP ); }
                  }
                  else*/
                  {
                      NoMatchFound = 1;
                  }
                  break;
              }
              if(NoMatchFound)
              {
#if 0
               if(strstr((const char *)IO.pRxBuffer, "ready\r\n"))     { ESP8266_StoreCommand(pESP, ESP8266_RX_READY); }
               else if(strstr((const char *)IO.pRxBuffer, "OK\r\n"))   { ESP8266_StoreCommand(pESP, ESP8266_RX_OK); }
               //         +CIOBAUD:(X-X)
               else if(strstr((const char *)IO.pRxBuffer, "+CIOBAUD:("))
               {
                if(ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM >= 2)
                {
                   res = sscanf(IO.pRxBuffer, "+CIOBAUD:(%u-%u)", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1]);
                   if(2 == res) { ESP8266_StoreCommand(pESP, ESP8266_RX_CIOBAUD_RANGE); }
                   else         { ESP8266_StoreCommand(pESP, ESP8266_RX_BAD_STRUCTURE); }
                }
                else
                {
                    ESP8266_StoreCommand(pESP, ESP8266_RX_INTERNAL_ERROR);
                }
               }
               //         +CIOBAUD:X
               else if(strstr((const char *)IO.pRxBuffer, "+CIOBAUD:"))
               {
                if(ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM >= 1)
                {
                   res = sscanf(IO.pRxBuffer, "+CIOBAUD:%u", &IO.pReceivedParameter[0]);
                   if(1 == res) { ESP8266_StoreCommand(pESP, ESP8266_RX_CIOBAUD); }
                   else         { ESP8266_StoreCommand(pESP, ESP8266_RX_BAD_STRUCTURE); }
                }
                else
                {
                    ESP8266_StoreCommand(pESP, ESP8266_RX_INTERNAL_ERROR);
                }
               }
               //         +CWJAP:str
               else if(strstr((const char *)IO.pRxBuffer, "+CWJAP:"))
               {
                 res = sscanf(IO.pRxBuffer, "+CWJAP:\"%40s\"", IO.pReceivedParameterStr);    //  40 name length hardcoded, can make problems
                 if(1 == res) { ESP8266_StoreCommand(pESP, ESP8266_RX_CWJAP); }
                 else         { ESP8266_StoreCommand(pESP, ESP8266_RX_BAD_STRUCTURE); }
               }
               //         BAUD->X
               else if(strstr((const char *)IO.pRxBuffer, "BAUD->"))
               {
                   res = sscanf(IO.pRxBuffer, "BAUD->%u", &IO.pReceivedParameter[0]);
                   if(1 == res) { ESP8266_StoreCommand(pESP, ESP8266_BAUDRATE_CONFIRMATION); }
               }
               else if(strstr((const char *)IO.pRxBuffer, "no change\r\n"))        { StoreCommand(eAT::NOCHANGE        ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "FAIL\r\n"))             { StoreCommand(eAT::FAIL            ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "ALREAY CONNECT\r\n"))   { StoreCommand(eAT::ALREADY_CONNECT ); }//+  // this is real message with grammatical error
               else if(strstr((const char *)IO.pRxBuffer, "ALREADY CONNECT\r\n"))  { StoreCommand(eAT::ALREADY_CONNECT ); }//+  // this message can appear if manufacturer fix gramm. error
               else if(strstr((const char *)IO.pRxBuffer, "SEND OK\r\n"))          { StoreCommand(eAT::SEND_OK         ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "wrong syntax\r\n"))     { StoreCommand(eAT::WRONG_SYNTAX    ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "ERROR\r\n"))            { StoreCommand(eAT::ERROR           ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "Linked\r\n"))           { StoreCommand(eAT::LINKED          ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "Link\r\n"))             { StoreCommand(eAT::LINK            ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "Unlink\r\n"))           { StoreCommand(eAT::UNLINK          ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "busy\r\n"))             { StoreCommand(eAT::BUSY            ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "busy p...\r\n"))        { StoreCommand(eAT::BUSY_P          ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "busy s...\r\n"))        { StoreCommand(eAT::BUSY_S          ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "link is not\r\n"))      { StoreCommand(eAT::LINKISNOT       ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "no ip\r\n"))            { StoreCommand(eAT::NOIP            ); }//+
               else if(strstr((const char *)IO.pRxBuffer, "no this fun\r\n"))      { StoreCommand(eAT::NO_THIS_FUNCTION); }//+
               else if(5 == sscanf(IO.pRxBuffer, "%u.%u.%u.%u%c", &IO.pReceivedParameter[0], &IO.pReceivedParameter[1], &IO.pReceivedParameter[2], &IO.pReceivedParameter[3], &c))
                {
                  if(c == '\r') { StoreCommand(eAT::IP ); }
                }
#endif
                  IO.RxBuffCounter = 0;    //  prepare buffer for next command

              }
              else
              {
                  IO.RxBuffCounter = 0;    //  prepare buffer for next command
              }
            }
       }
       else if(IO.RxBuffCounter >= 2)         //      at least 2 bytes received
       {
          if((*(IO.pRxBuffer + IO.RxBuffCounter - 1) == '\n') &&
             (*(IO.pRxBuffer + IO.RxBuffCounter - 2) == '\r'))     //  empty line received
             {
                  IO.RxBuffCounter = 0;    //  prepare buffer for the next command
             }
       }

       //============= Receive Tx Echo (multi-mode) ===============
       //res = sscanf(IO.pRxBuffer, "AT+CIPSEND=%u,%u%c", &id, &data_len, &c);
       {

       }
    }
  }
}

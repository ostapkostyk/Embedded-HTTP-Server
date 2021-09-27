/**
  ******************************************************************************
  * @file    ESP8266.hpp
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

#ifndef ESP8266_HPP_
#define ESP8266_HPP_

#include "Timer.h"
#include <string.h>

extern "C" {
#include "common.h"
#include "ESP8266_Interface.h"
}

using namespace mTimer;

namespace OKO_ESP8266
{
/* forward declaration of enums is not supported by compiler used in this project, therefore some enums are moved from .cpp to separate header file */
#include "ESP8266_def.hpp"

/* Configuration */
#define ESP8266_UART_SPEED  ((uint32_t)230400)  //  this speed will be set after baudrate detection. Can be any desired speed supported by ESP8266: 0:9600; 1:115200; 2:19200; 3:38400; 4:74880; 5:230400; 6:460800; 7:921600

#define ESP8266_SOCKETS_MAX  5      //  5: id 0-4, hardware-specific value, refer to ESP8266 documentation!
#define ESP8266_RX_BUFF_LEN  100    //  Buffer size for responses on AT commands
#define ESP8266_AP_NAME_LEN  40     //  Access Point name maximum length
#define ESP8266_AP_PWD_LEN   40     //  Access Point password max length

#define ESP8266_AP_CH_NUM   13      //  number of acceptable channels for AP mode

#define ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM  6    //  number of parameters to hold module response data
#define ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN  (ESP8266_AP_NAME_LEN + 4 + ESP8266_AP_PWD_LEN + 20)     //  length of the string using to store string responses of the ESP module. Must be at least 81 for correct work; 4 is possible postfix size
#define ESP8266_RECEIVED_COMMAND_PARAM_STR2_LEN 20                                                      //  length of the second string using to store string responses of the ESP module.

/* Enable/Disable debug output to stdout */
#define ESPDEBUG    1
#define esp_debug_print(...) do { if (ESPDEBUG) fprintf(stdout, ##__VA_ARGS__); } while (0) //  debug print for this module. Compiler should not exclude all calls from the code if ESPDEBUG not defined

/* END of Configuration */

/*************************************************************
 *                      ESP Class
 *************************************************************/

class ESP
{
public:
    /* Constructor */
    ESP(uint8_t HuartNumber);

    /* Main Handler - must be called regularly (e.g. in main loop) */
    void Process();

/*******************************************
 *
 *******************************************/
/* Module Toggle (On/Off using Enable IO-pin) */
enum class eModuleToggle{Enable = 0, Disable};

/* Modem Modes */
enum class eModuleMode{ Undefined = 0, Station, AccessPoint, StationAndAccessPoint };

/* Access Point ECN types */
enum class eECNType{Open = 0, WPA_PSK = 2, WPA2_PSK = 3, WPA_WPA2_PSK = 4};

/* Module Connection Type: Single or Multiple */
enum class eModuleConnectionType{Undefined = 0, Single, Multiple};

/* Station connection states */
enum class eStationConnectionState{NotConnected = 0, Connecting, Connected, ConnectFailed, ConnectTimeout, Disconnected};

/* AccessPoint states */
enum class eAccessPointState{NotStarted = 0, Starting, Timeout, Failed, Started};

/* Server States */
enum class eServerState{Undefined = 0, GotIP = 2, Connected = 3, Disconnected = 4, Connecting = 200, ConnectTimeout = 254, Error = 255};

/* Socket States */
enum class eSocketState{Closed = 0, Open, ConnectRequested, Connecting, Connected, CloseRequested, Closing, Error};

/* Socket Error Flags */
enum class eSocketErrorFlag {NoError = 0, FailToConnect, Timeout, NoAccessPoint, InternalError};

/* Socket Type: UDP or TCP */
enum class eSocketType {UDP = 0, TCP};

/* Data transmit states */
enum class eSocketSendDataStatus{Idle = 0, SendRequested, InProgress, SendFail, SendSuccess};

/* Public Methods */
/* Enables/Disables Module using "Enable" IO-pin. Module is disabled by default and whole engine will not work saving CPU resources */
void ModuleToggle(eModuleToggle toggle);

/* returns true if module initialization finished (module responds on AT commands) */
bool isModuleReady();

/* Sets Module connection type Single or Multiple. Returns true if module have switched to requested state or false if not */
bool SetModuleConnectionType(eModuleConnectionType Type);

/* Switches module to Station mode and connects to Access Point with name AP_Name and Password  AP_PWD */
STATUS SwitchToStationMode(char* AP_Name, char* AP_PWD);

/* Switches module to Access Point Mode and sets name as AP_Name, password as AP_PWD, channel to ch, encryption to ecn.
 * If AP_Name is already occupied in the network then module with assign name with next free postfix in format
 * <AP_Name>_postfix up to 100 and set the flag that the name has been changed, available in API */
STATUS SwitchToAccessPointMode(char* AP_Name, char* AP_PWD, uint16_t ch, eECNType ecn);

/* Future feature, not implemented yet */
STATUS SwitchToBothStaAndAPMode();

/* returns current module mode (Station or Access Point or Both) */
ESP::eModuleMode GetCurrentModuleMode();

/* Starts Server to listen on port Port */
void StartServer(unsigned int Port);

/* Stops Server */
void StopServer();

/* Gets Server State */
ESP::eServerState GetServerState();

/* Open socket of type Type (UDP or TCP) if there is free one. If success then writes socket ID to SocketID
 * (1 to ESP8266_SOCKETS_MAX) and returns SUCCESS. If all sockets occupied then returns ERROR */
STATUS OpenSocket(uint8_t &SocketID, eSocketType Type);

/* Connects socket <SocketID> to provided Address and Port. Returns SUCCESS or ERROR.
 * Socket with SocketID must be opened by OpenSocket() before. */
STATUS ConnectSocket(uint8_t SocketID, char * Address, unsigned int Port);

/* Listen to connected socket. Returns SUCCESS if SocketID is in range of existing sockets
 * and this socket is connected (see ESP::eSocketState) */
STATUS ListenSocket(uint8_t SocketID, uint8_t * RxBuffer, uint16_t BufferSize);

/* check if there are data received, returns num of data in buffer OR -1 if message has been cut */
uint16_t SocketRecv(uint8_t SocketID);

/* Initialize Data Send process. Return SUCCESS if socket is connected and data prepared to be sent, otherwise ERROR */
STATUS SocketSend(uint8_t SocketID,  uint8_t* Data, uint16_t DataLen);

/* Same as SocketSend() but closes socket after sending completion */
STATUS SocketSendClose(uint8_t SocketID,  uint8_t* Data, uint16_t DataLen);

/* Closes socket. Returns error if SocketID is out of range (doesn't exist), otherwise SUCCESS
 * If Socket is not "Open" or "Closed", means it is connected or in process of connection/disconnection
 * then this method only requests it to be closed and state machine tries to close it later */
STATUS CloseSocket(uint8_t SocketID);

/* Returns current socket state */
ESP::eSocketState GetSocketState(uint8_t SocketID);

/* Returns data send status */
ESP::eSocketSendDataStatus GetDataSendStatus(uint8_t SocketID);

/* Returns connection state to remote access point */
ESP::eStationConnectionState StationConnectionState();

/* Returns Local Access Point state (when ESP module starts as AP or both AP and Station) */
ESP::eAccessPointState LocalAccessPointState();

/* Returns postfix number if Access Point started with different name (see SwitchToAccessPointMode()) or
 * zero if name has not changed */
uint8_t AccessPointNameChanged();

/* Sets IP address of the Access Point */
bool SetAccessPointIP(uint32_t ip, uint32_t gw, uint32_t mask);

/******************************************
 *      PRIVATE MEMBERS
 ******************************************/
private:
    uint8_t HuartNumber;
    bool HuartConfigured;                                       //  disables whole engine if false (UART is not configured). Changes to true if configured successfully by ESP_HuartInit()
    eModuleToggle ModuleToggleFlag = eModuleToggle::Disable;    //  Module disabled by default. Module must be enabled from the main program to start communication

    uint8_t STEP;   //  State machine step
    Timer StateTimer{Timer::Down, _1sec_, false};    //  down counting timer with dummy delay and disabled
    const unsigned long DataSendTimeout = _3sec_;    //  defines timeout for sending data with socket

    const unsigned long FlushRxUARTTime = _500ms_;   // value for the timer below
    Timer RxOverflowTimer{Timer::Down, FlushRxUARTTime, false}; //  if Rx buffer overflows (usually happen when ESP module starts with the wrong speed) then this timer keeps engine off for time-out time to flush all trash coming from the module

    bool SMStateChanged;        //  flag to initialize new state in state-machine (indicates that the state has been changed)
    /* Debug flags */
    bool DebugFlag_RxStreamToStdOut;
    bool DebugFlag2 = false;
    /* New command semaphore - flag blocking from storing new command from Rx stream until engine make one round (in case when Rx stream received (using interrupts) more than one command and main handler has not been ran in between) */
    bool NewCommandSemaphore;
    bool FirstTime;     //  flag for one-time at start initialization, using to initialize something (e.g. UART communication)

    /* PRIVATE METHODS */
    void RxHandler();                   //  Handles Rx stream coming from ESP module. Parses commands and data from sockets
    void ModuleReInit();                //  Re-initialize ESP module in case of fault that cannot be handled by engine
    bool isCommandReceived(eAT Cmd);    //  check if specific AT-command received from ESP module
    void StoreCommand(eAT Cmd);         //  stores AT command received from ESP module
    void ClearLastCommand();            //  clears last received AT-command

    /* STATE MACHINE */
    class StateMachine
    {
    public:
        virtual ~StateMachine(){}
        virtual void Process(ESP* pESP) = 0;
    };

    /* State Machine Control */
    StateMachine* CurrentState;
    StateMachine* LastState;
    bool StateMachineStateChanged() const { return SMStateChanged; };

    /* INCLUDED CLASSES */

/***********************************************************************************************
*   Class module handles hardware-states and general states of the ESP module
************************************************************************************************/
    class module
    {
    public:
        // Constructor:
        module();

        // Supported by ESP8266 baudrates. Used for Baudrate detection. Baud rates for testing and there sequence can be modified if needed (but not a number of them)
        enum class eBaudRates : uint32_t {Baud1 = 9600, Baud2 = 115200, Baud3 = 19200, Baud4 = 38400, Baud5 = 74880, Baud6 = 230400, Baud7 = 460800, Baud8 = 921600};

        bool ModuleReady = false;
        eModuleMode ModeActual;
        eModuleMode ModeRequest;
        eModuleConnectionType ConnectionTypeActual;
        eModuleConnectionType ConnectionTypeRequest;
        uint8_t BaudRateIndex;
        uint32_t BaudRate = ESP8266_UART_SPEED; //  ESP8266 baudrate is switched to this speed after baudrate detection. TODO: public method can be implemented to set this member from outside.
        bool EchoIsOnFlag = true;               //  echo is ON by default on ESP8266, flag is cleared by state machine after successful command ATE0
        uint8_t AP_NamePostfix;                 //  Access Point Name Postfix. Is added to Requested Name automatically if another AP with this name is broadcasting

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }
    };

    module Module{};    //  create module instance ones (cannot be destroyed but can be re-initialized with constructor)
    module* pModule = &Module;

/***********************************************************************************************
 *  Base Data-class for Access Point and Station settings and credentials
 ***********************************************************************************************/
    class AccessPointCredentials
    {
    public:
        /* Constructor */
        AccessPointCredentials();

        /* CheckNameAndPassword: returns true if Name is not empty and length of both within
         * maximum length (has end-of-line within array). Otherwise false */
        bool CheckNameAndPassword();

        char *pName, *pPassword;
        const size_t NameSize = sizeof(Name);       //  Name array size, used in calls like snprintf
        const size_t PasswordSize = sizeof(Name);   //  Password array size, used in calls like snprintf

        uint32_t IP;        //  Current IP address
        uint32_t NetMask;   //  Current Net Mask
        uint32_t Gateway;   //  Current Gateway
        uint64_t MAC;

        uint32_t NewIP;     //  Requested new IP address
        uint32_t NewNetMask;//  Requested new NetMask
        uint32_t NewGateway;//  Requested new Gateway

        uint16_t Channel;
        eECNType ecn;

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }

        char Name[ESP8266_AP_NAME_LEN + 4] = "";    //  4 is for possible postfix number e.g. _100
        char Password[ESP8266_AP_PWD_LEN] = "";
    };

    /* This class derived from AccessPointCredentials holds data of ESP module running as Access Point (own ESP's IP, mask and so on) */
    struct AccessPoint : AccessPointCredentials
    {
        /* Constructor */
        AccessPoint();

        eAccessPointState State;            //  Describes Local Access Point (own ESP8266) State

        // NewName: non-zero value signalize that name changed because requested name was not possible to assign (for example such AP is already existing).
        //non-zero value contains postfix used for new name
        uint8_t NewName;

        bool ChangeIPRequest;   //  request to change IP address of Access Point

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }
    };

    /* This class derived from AccessPointCredentials holds data of remote Access Point we are connecting to (e.g. WiFi router) */
    struct Station : AccessPointCredentials
    {
        /* Constructor */
        Station();

        eStationConnectionState StationState;

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }
    };

    AccessPoint  LocalAP{};             //  own ESP's settings and credentials
    AccessPoint *pLocalAP = &LocalAP;

    Station  RemoteAP{};                //  remote (router) settings and credentials
    Station *pRemoteAP = &RemoteAP;

/***********************************************************************************************
 *  This class handles server-mode related states of SP module
 ***********************************************************************************************/
    class server
    {
    public:
        /* Constructor */
        server();
        eServerState State;
        bool StartRequest;
        unsigned int Port;
    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }
    };

    server Server{};            //  instance of server class (only create, never destroy)
    server *pServer = &Server;

/***********************************************************************************************
 *  This class (input-output) handles communication with ESP module over HUART
 ***********************************************************************************************/
    class io
    {
    public:
        /* Constructor */
        io();

        bool ListeningToTxData(void) const { return ListenToTxData; }   //  returns true in module switched from AT commands mode to data mode and ready to get Tx stream
        void StopListenToTxData(void) { ListenToTxData = false; }

        void ClearReceivingErrors();
        void ClearRxStream();

        const unsigned int TxPacketMaxSize = 2048;                //  defines maximum size of one TCP/UDP packet (modem limitation)

        char* pRxBuffer, *pCommandString, *pReceivedParameterStr, *pReceivedParameterStr2;
        const size_t  RxBufferSize = sizeof(RxBuffer);
        const size_t  CommandStringSize = sizeof(CommandString);
        const size_t  ReceivedParameterStrSize = sizeof(ReceivedParameterStr);
        const size_t  ReceivedParameterStr2Size = sizeof(ReceivedParameterStr2);
        unsigned int* pReceivedParameter;
        eAT ReceivedCommand;

        uint8_t  RxSocketId;
        uint16_t RxBuffCounter;
        uint32_t RxIgnoreCounter;
        uint16_t CurrentSocketDataLeft;
        uint8_t  *pCurrentSocketData;

        bool DoEmptyRxStream;
        bool RxOverflowEvent;
        bool RxOverflowFlag;
        bool ReceiveError;
        bool ListenToTxData;
        bool ReceivingDataStream;

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }

        /* memory allocated statically to see memory footprint at compile time, better debug possibilities etc.  */
        char RxBuffer[ESP8266_RX_BUFF_LEN] = "";
        char CommandString[ESP8266_AP_NAME_LEN+ESP8266_AP_PWD_LEN+40] = "";
        unsigned int    ReceivedParameter[ESP8266_RECEIVED_COMMAND_NUM_OF_PARAM];
        char            ReceivedParameterStr[ESP8266_RECEIVED_COMMAND_PARAM_STR_LEN];
        char            ReceivedParameterStr2[ESP8266_RECEIVED_COMMAND_PARAM_STR2_LEN];
    };

    io IO{};
    io* pIO = &IO;

/***********************************************************************************************
 *  Data class to store socket-related data and handle status of sockets
 ***********************************************************************************************/
    class socket
    {
    public:
        /* Constructor */
        socket();

        eSocketState State;
        eSocketErrorFlag ErrorFlag;
        eSocketType Type;
        char *Address;
        uint16_t Port;
        uint8_t *DataRx;
        uint16_t RxBuffSize;
        uint16_t RxDataLen;
        bool  RxLock;
        uint8_t *DataTx;
        uint16_t TxDataLen;
        uint16_t TxPacketLen;
        bool  TxLock;
        eSocketSendDataStatus TxState;
        bool DataCutFlag;    //  indicates HUART buffer overflow during receiving income stream or that the length of Rx message is biger than provided buffer length for the message
        bool CloseAfterSending;

        //uint16_t CurrentTxSocketId;     //  needed for state machine, keeps the number of current socket (0 to ESP8266_SOCKETS_MAX)

    //private:
        /* Reload operator new to be able to call constructor and re-initialize members any time */
        inline void* operator new( size_t, void* where ) { return where; }
    };

    socket Socket[ESP8266_SOCKETS_MAX];     //  create sockets, never destroyed, can be re-initialized by constructor
    socket *pSocket[ESP8266_SOCKETS_MAX];
    const uint8_t SocketsNum = ESP8266_SOCKETS_MAX;

/***********************************************************************************************
 *                           STATE MACHINE STATES
************************************************************************************************/
    class StateModuleReset          : public StateMachine { public: void Process(ESP* pESP); ~StateModuleReset(){}          };
    class StateStartModule          : public StateMachine { public: void Process(ESP* pESP); ~StateStartModule(){}          };
    class StateTestModule           : public StateMachine { public: void Process(ESP* pESP); ~StateTestModule(){}           };
    class StateDetectModuleBaudrate : public StateMachine { public: void Process(ESP* pESP); ~StateDetectModuleBaudrate(){} };
    class StateModuleInitialization : public StateMachine { public: void Process(ESP* pESP); ~StateModuleInitialization(){} };
    class StateSelectModuleMode     : public StateMachine { public: void Process(ESP* pESP); ~StateSelectModuleMode(){}     };
    class StateFindFreeSSID         : public StateMachine { public: void Process(ESP* pESP); ~StateFindFreeSSID(){}         };
    class StateSetApParameters      : public StateMachine { public: void Process(ESP* pESP); ~StateSetApParameters(){}      };
    class StateJoinAP               : public StateMachine { public: void Process(ESP* pESP); ~StateJoinAP(){}               };
    class StateChangeConnectionType : public StateMachine { public: void Process(ESP* pESP); ~StateChangeConnectionType(){} };
    class StateStartServer          : public StateMachine { public: void Process(ESP* pESP); ~StateStartServer(){}          };
    class StateGetApIP              : public StateMachine { public: void Process(ESP* pESP); ~StateGetApIP(){}              };
    class StateChangeAPIP           : public StateMachine { public: void Process(ESP* pESP); ~StateChangeAPIP(){}           };
    class StateGetConnectionsInfo   : public StateMachine { public: void Process(ESP* pESP); ~StateGetConnectionsInfo(){}   };
    class StateStandby              : public StateMachine
    {
        public: void Process(ESP* pESP); ~StateStandby(){}
        private:
        uint8_t SocketId;
    };
    class StateSendData             : public StateMachine
    {
        public: void Process(ESP* pESP); ~StateSendData(){}
        private:
        uint8_t SocketId;     //  needed in StateSendData, keeps the number of current socket (0 to ESP8266_SOCKETS_MAX)
        uint8_t RetryCounter;
    };
    class StateOpenSocket           : public StateMachine
    {
        public: void Process(ESP* pESP); ~StateOpenSocket(){}
        private:
        uint8_t SocketId;
    };
    class StateCloseSocket          : public StateMachine
    {
        public: void Process(ESP* pESP); ~StateCloseSocket(){}
        private:
        uint8_t SocketId;
    };
    //class  : public StateMachine { public: void Process(ESP* pESP); ~(){} };

    /* State Machine States */
    StateModuleReset            smModuleReset;
    StateStartModule            smStartModule;
    StateTestModule             smTestModule;
    StateDetectModuleBaudrate   smDetectModuleBaudrate;
    StateModuleInitialization   smModuleInitialization;
    StateSelectModuleMode       smSelectModuleMode;
    StateFindFreeSSID           smFindFreeSSID;
    StateSetApParameters        smSetApParameters;
    StateJoinAP                 smJoinAP;
    StateStandby                smStandby;
    StateChangeConnectionType   smChangeConnectionType;
    StateSendData               smSendData;
    StateOpenSocket             smOpenSocket;
    StateCloseSocket            smCloseSocket;
    StateStartServer            smStartServer;
    StateGetApIP                smGetApIP;
    StateChangeAPIP             smChangeAPIP;
    StateGetConnectionsInfo     smGetConnectionsInfo;

};

}   //  END of namespace OKO_ESP8266

#endif /* ESP8266_HPP_ */

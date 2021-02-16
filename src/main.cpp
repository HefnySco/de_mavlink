#include <iostream>
#include <unistd.h> //sleep
#include <signal.h>

#include "./helpers/colors.hpp"
#include "messages.hpp"
#include "configFile.hpp"
#include "udpClient.hpp"
#include "fcb_main.hpp"
#include "fcb_andruav_message_parser.hpp"

using namespace uavos;

bool exit_me = false;

// UAVOS Current PartyID read from communicator
std::string  PartyID;
// UAVOS Current GroupID read from communicator
std::string  GroupID;
std::string  ModuleID;

uavos::fcb::CFCBMain& cFCBMain = uavos::fcb::CFCBMain::getInstance();
uavos::fcb::CFCBAndruavMessageParser cAndruavMessageParser = uavos::fcb::CFCBAndruavMessageParser();

uavos::CConfigFile& cConfigFile = CConfigFile::getInstance();

uavos::comm::CUDPClient& cUDPClient = uavos::comm::CUDPClient::getInstance();  

void onReceive (const char * jsonMessage, int len);
void uninit ();


const std::string generateForwardSendCMD (
        // target ID could be empty string if commType is broadcast.
        const std::string& targetID,
        // communication type [peer-to-peer or broadcast] 
        const std::string& commType,
        // Inter module communication or what ?
        const std::string& commandType,
        // message type ID
        const int messageType,
        // message contents
        const Json& jmsg)
{
        Json fullMessage;

        fullMessage[ANDRUAV_PROTOCOL_TARGET_ID]         = std::string(targetID);
        fullMessage[INTERMODULE_COMMAND_TYPE]           = std::string(commType);
        fullMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE]      = messageType;
        fullMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]       = jmsg;
        return fullMessage.dump();
}

void sendJMSG (const std::string& targetPartyID, const Json& jmsg, const int& andruav_message_id, const bool& internal_message)
{
        
        Json webrtcMsg;
        std::string commType = CMD_COMM_INDIVIDUAL;
        if (internal_message == true)
        {
            commType = CMD_TYPE_INTERMODULE;
        }
        else
        {
            if (targetPartyID.length() != 0 )
            {
                    commType = CMD_COMM_GROUP;
            }
        
        }

        const std::string fullMessage = generateForwardSendCMD (targetPartyID.c_str(), commType, std::string(CMD_TYPE_INTERMODULE),
                andruav_message_id, jmsg);
        cUDPClient.SendJMSG(fullMessage);
}

/**
 * creates JSON message that identifies Module
**/
const Json createJSONID (bool reSend)
{
        Json& jsonConfig = cConfigFile.GetConfigJSON();
        Json jsonID;        
        
        jsonID[INTERMODULE_COMMAND_TYPE] =  CMD_TYPE_INTERMODULE;
        jsonID[ANDRUAV_PROTOCOL_MESSAGE_TYPE] =  TYPE_AndruavModule_ID;
        Json ms;
        
        ms["a"] = jsonConfig["module_id"];
        ms["b"] = jsonConfig["module_class"];
        ms["c"] = jsonConfig["module_messages"];
        ms["d"] = jsonConfig["module_features"];
        ms["e"] = jsonConfig["module_key"];
        ms["z"] = reSend;

        jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD] = ms;
        std::cout << jsonID.dump(4) << std::endl;              
        return jsonID;
}

void onReceive (const char * jsonMessage, int len)
{
    static bool bFirstReceived = false;
        
    #ifdef DEBUG        
        std::cout << _INFO_CONSOLE_TEXT << "RX MSG: " << jsonMessage << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    Json jMsg;
    
    jMsg = Json::parse(jsonMessage);

    if (std::strcmp(jMsg[INTERMODULE_COMMAND_TYPE].get<std::string>().c_str(),CMD_TYPE_INTERMODULE)==0)
    {
        const Json cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
        const int messageType = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    
        if (messageType== TYPE_AndruavModule_ID)
        {
        const Json moduleID = cmd ["e"];
        PartyID = std::string(moduleID[ANDRUAV_PROTOCOL_SENDER].get<std::string>());
        GroupID = std::string(moduleID[ANDRUAV_PROTOCOL_GROUP_ID].get<std::string>());

        if (!bFirstReceived)
        { 
            // tell server you dont need to send ID again.
            std::cout << _SUCCESS_CONSOLE_TEXT_ << "Communicator Server Found " <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            Json jsonID = createJSONID(false);
            cUDPClient.SetJSONID (jsonID.dump());
            bFirstReceived = true;
        }
        return ;
        }

    }
    
    cAndruavMessageParser.parseMessage(jMsg);
    
}

/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{
    std::string configName = "config.module.json";
    if (argc  > 1)
    {
        configName = argv[1];
    }

    // Reading Configuration
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ << "=================== " << "STARTING PLUGIN ===================" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    
    cConfigFile.InitConfigFile (configName.c_str());
    Json& jsonConfig = cConfigFile.GetConfigJSON();
    

    // UDP Server
    cUDPClient.init(jsonConfig["s2s_udp_target_ip"].get<std::string>().c_str(),
            std::stoi(jsonConfig["s2s_udp_target_port"].get<std::string>().c_str()),
            jsonConfig["s2s_udp_listening_ip"].get<std::string>().c_str() ,
            std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>().c_str()));
    
    
    Json jsonID = createJSONID(true);
    cUDPClient.SetJSONID (jsonID.dump());
    cUDPClient.SetMessageOnReceive (&onReceive);
    cUDPClient.start();

    cFCBMain.registerSendJMSG(sendJMSG);
    cFCBMain.init(jsonConfig);
    
}


void uninit ()
{

    cFCBMain.uninit();
    
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    
    cUDPClient.stop();

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Unint_after Stop" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    // end program here
	exit(0);
}


// ------------------------------------------------------------------------------
//   Quit Signal Handler
// ------------------------------------------------------------------------------
// this function is called when you press Ctrl-C
void quit_handler( int sig )
{
	std::cout << _INFO_CONSOLE_TEXT << std::endl << "TERMINATING AT USER REQUEST" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
	
	try 
    {
        exit_me = true;
        uninit();
	}
	catch (int error){}

    exit(0);
	

}


int main (int argc, char *argv[])
{
    signal(SIGINT,quit_handler);
	
    init (argc, argv);

    while (!exit_me)
    {
       sleep (1);
       
    }

}
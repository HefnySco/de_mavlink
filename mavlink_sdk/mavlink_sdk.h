#ifndef MAVLINK_SDK_H_
#define MAVLINK_SDK_H_

#include <memory>
#include "./helpers/colors.h"
#include "generic_port.h"
#include "mavlink_communicator.h"
#include "vehicle.h"
#include "mavlink_waypoint_manager.h"
#include "mavlink_events.h"


namespace mavlinksdk
{
    class CMavlinkSDK : protected mavlinksdk::comm::CCallBack_Communicator, protected mavlinksdk::CCallBack_Vehicle, protected mavlinksdk::CMavlinkEvents, protected mavlinksdk::CCallBack_WayPoint
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CMavlinkSDK& getInstance()
            {
                static CMavlinkSDK instance;

                return instance;
            }

            CMavlinkSDK(CMavlinkSDK const&)           = delete;
            void operator=(CMavlinkSDK const&)        = delete;

        
            // Note: Scott Meyers mentions in his Effective Modern
            //       C++ book, that deleted functions should generally
            //       be public as it results in better error messages
            //       due to the compilers behavior to check accessibility
            //       before deleted status

        private:

            CMavlinkSDK() 
            {
                m_sysid  = 0;
                m_compid = 0;
                m_callback_vehicle  = (mavlinksdk::CCallBack_Vehicle*) this;
                m_mavlink_events    = (mavlinksdk::CMavlinkEvents*) this;
                m_callback_waypoint = (mavlinksdk::CCallBack_WayPoint*)this; 
            }                    // Constructor? (the {} brackets) are needed here.

            
        public:
            
            ~CMavlinkSDK ();

        public:
            void start (mavlinksdk::CMavlinkEvents * mavlink_events);
            void connectUDP (const char *target_ip, int udp_port);
            void connectSerial (const char *uart_name, int baudrate);
            void stop();


        // Reading Status
        public:
            inline const int& getSysId () 
            {
                return m_sysid;
            }
            inline const int& getCompId ()
            {
                return m_compid;
            }

        // Writing Status
        public:    
            std::unique_ptr<mavlinksdk::CVehicle>& getVehicle  ()
            {
                return m_vehicle;
            }

            std::unique_ptr<mavlinksdk::CMavlinkWayPointManager>& getWayPointManager()
            {
                return m_mavlink_waypoint_manager;
            }

            void sendMavlinkMessage(const mavlink_message_t& mavlink_message);
            
        protected:
              mavlinksdk::CMavlinkEvents * m_mavlink_events; 
              mavlinksdk::CCallBack_Vehicle* m_callback_vehicle;
              mavlinksdk::CCallBack_WayPoint* m_callback_waypoint;
              std::shared_ptr<mavlinksdk::comm::GenericPort> m_port;
              std::unique_ptr<mavlinksdk::comm::CMavlinkCommunicator> m_communicator;
              std::unique_ptr<mavlinksdk::CVehicle> m_vehicle;
              std::unique_ptr<mavlinksdk::CMavlinkWayPointManager> m_mavlink_waypoint_manager;
              bool m_stopped_called = false;
              int m_sysid;
              int m_compid;

        // CCallback_Communicator inheritance
        protected:
            void OnMessageReceived (mavlink_message_t& mavlink_message) override;
            void OnConnected (const bool& connected) override;

        // CCalback_WayPointManager inheritance
        protected:
            inline void onMissionACK (const int& result, const int& mission_type, const std::string& result_msg)  override
            {
                std::cout << _INFO_CONSOLE_TEXT << "onMissionACK " << std::to_string(result) << " - " << result_msg << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->onMissionACK (result, mission_type, result_msg);
            }


            inline void onMissionSaveFinished (const int& result, const int& mission_type, const std::string& result_msg) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "onMissionSaveFinished " << std::to_string(result) << " - " << result_msg << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->onMissionSaveFinished (result, mission_type, result_msg);
            }
        

            inline void onWaypointReached (const int& sequence) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "onWaypointReached " << std::to_string(sequence) << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->onWaypointReached (sequence);
            }

            inline void onWayPointReceived (const mavlink_mission_item_int_t& mission_item_int) override
            {
                std::cout << _INFO_CONSOLE_TEXT << "onWayPointReceived " << std::to_string(mission_item_int.seq) << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->onWayPointReceived (mission_item_int);
            }

            inline void onWayPointsLoadingCompleted () override 
            {
                m_mavlink_events->onWayPointsLoadingCompleted();
            }
   
    
            
        // CCallback_Vehicle inheritance
        protected:
            inline void OnHeartBeat_First (const mavlink_heartbeat_t& heartbeat) override 
            {
                m_mavlink_events->OnHeartBeat_First (heartbeat);
            };
            inline void OnHeartBeat_Resumed (const mavlink_heartbeat_t& heartbeat) override 
            {
                m_mavlink_events->OnHeartBeat_Resumed (heartbeat);
            };
            inline void OnArmed (const bool& armed) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "OnArmed " << std::to_string(armed) << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->OnArmed (armed);

            };
            inline void OnFlying (const bool& isFlying) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "OnFlying " << std::to_string(isFlying) << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->OnFlying (isFlying);
            };

            inline void OnACK (const int& result, const std::string& result_msg) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "OnACK " << std::to_string(result) << " - " << result_msg << _NORMAL_CONSOLE_TEXT_ << std::endl;    
                m_mavlink_events->OnACK (result, result_msg);
            };

            inline void OnStatusText (const std::uint8_t& severity, const std::string& status) override 
            {
                std::cout << _INFO_CONSOLE_TEXT << "Status " << status << _NORMAL_CONSOLE_TEXT_ << std::endl;
                m_mavlink_events->OnStatusText (severity, status);
            };
            
            inline void OnModeChanges(const int& custom_mode, const int& firmware_type)  override
            {
                m_mavlink_events->OnModeChanges (custom_mode, firmware_type);
            };

            inline void OnHomePositionUpdated(const mavlink_home_position_t& home_position)  override
            {
                m_mavlink_events->OnHomePositionUpdated (home_position);
            }
            


    };
}



#endif // MAVLINK_SDK_H_
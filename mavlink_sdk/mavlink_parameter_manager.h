#ifndef PARAMETER_MANAGER_H_
#define PARAMETER_MANAGER_H_


#include <map>
#include <vector>

namespace mavlinksdk
{

    typedef enum {
        LOADING_PARAMS_LIST_EMPTY       = 0,
        LOADING_PARAMS_LOAD_ALL_INIT    = 1,
        LOADING_PARAMS_ONE_BY_ONE       = 2,
        LOADING_PARAMS_LIST_LOADED      = 3
    } ENUM_LOADING_PARAMS_STATUS;

    class CCallBack_Parameter
    {
        public:
            virtual void OnParamReceived(const std::string& param_name, const mavlink_param_value_t& param_message, const bool& changed)    {};
            virtual void OnParamReceivedCompleted ()                                                                                        {};
    };

    class CMavlinkParameterManager
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CMavlinkParameterManager& getInstance()
            {
                static CMavlinkParameterManager instance;
                return instance;
            }

            CMavlinkParameterManager(CMavlinkParameterManager const&)               = delete;
            void operator=(CMavlinkParameterManager const&)                        = delete;

            
            // Note: Scott Meyers mentions in his Effective Modern
            //       C++ book, that deleted functions should generally
            //       be public as it results in better error messages
            //       due to the compilers behavior to check accessibility
            //       before deleted status
        private:

            CMavlinkParameterManager() {};


        public:
                
            ~CMavlinkParameterManager ()
            {

            }

            

        public:
            
            void set_callback_parameter (mavlinksdk::CCallBack_Parameter* callback_parameter);
            void reloadParemeters ();

        public:

            void handle_heart_beat (const mavlink_heartbeat_t& heartbeat);
            void handle_param_value (const mavlink_param_value_t& param_message);


        public:

            const std::map<std::string, mavlink_param_value_t>& getParametersList() const
            {
                return m_parameters_list;
            }

            const bool isParametersListAvailable() const
            {
                return (m_parameter_read_mode== mavlinksdk::ENUM_LOADING_PARAMS_STATUS::LOADING_PARAMS_LIST_LOADED);
            }


        protected:

            uint16_t getFirstMissingParameterByIndex();

        protected:
            mavlinksdk::CCallBack_Parameter* m_callback_parameter;

            std::map<std::string, mavlink_param_value_t> m_parameters_list;
            std::vector<uint16_t> m_parameters_id;
            mavlinksdk::ENUM_LOADING_PARAMS_STATUS m_parameter_read_mode = mavlinksdk::ENUM_LOADING_PARAMS_STATUS::LOADING_PARAMS_LIST_EMPTY;
            int m_parameter_read_count = 0;
            uint16_t m_parameters_last_index_read = 0;
            uint64_t m_parameters_last_receive_time = 0;
    };
        
    
}


#endif
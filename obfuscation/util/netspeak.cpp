// Copyright (C) 2018 Janek Bevendorff

#include "netspeak.hpp"

namespace netspeak_util {

namespace {
std::shared_ptr<netspeak::NetspeakRS3> netspeakInstance;
}

/**
 * Get a configure singleton Netspeak instance.
 * This function requires you to call \link init() before first invocation.
 *
 * @return Netspeak instance
 */
std::shared_ptr<netspeak::NetspeakRS3> const instance()
{
    return netspeakInstance;
}

/**
 * Initialize Netspeak and return a pointer to a configured instance.
 * Successive calls to this function will not reinitialize the instance.
 *
 * @param netspeakHome Netspeak home directory
 */
void init(boost::filesystem::path const& netspeakHome)
{
    if (!netspeakInstance) {
        aitools::util::log("Loading Netspeak from", netspeakHome);
        netspeak::Configurations::Map netspeakConfig;
        netspeakConfig[netspeak::Configurations::path_to_home] = netspeakHome.string();
        netspeakConfig[netspeak::Configurations::cache_capacity] = "5000";
        netspeakInstance = std::make_shared<netspeak::NetspeakRS3>();
        netspeakInstance->initialize(netspeakConfig);
    }
}

}   // namespace netspeak_util

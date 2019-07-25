// Copyright (C) 2018 Janek Bevendorff

#ifndef OBFUSCATION_SEARCH_NETSPEAK_HPP
#define OBFUSCATION_SEARCH_NETSPEAK_HPP

#include <netspeak/NetspeakRS3.hpp>
#include <memory>

namespace netspeak_util {

std::shared_ptr<netspeak::NetspeakRS3> const instance();
void init(boost::filesystem::path const& netspeakHome);

}   // namespace netspeak_util

#endif //OBFUSCATION_SEARCH_NETSPEAK_HPP

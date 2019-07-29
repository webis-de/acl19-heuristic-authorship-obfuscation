/*
 * Copyright 2017-2019 Janek Bevendorff, Webis Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "operators/ContextlessSynonymOperator.hpp"
#include "util/NgramProfile.hpp"
#include "util/LayeredOStream.hpp"
//#include "util/netspeak.hpp"

#include "Obfuscator.hpp"

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

int main(int argc, char const* argv[])
{
    std::string inputFilename;
    std::string outputFilename;
    std::string targetProfileFilename;
    std::string netspeakHome;
    std::vector<std::string> targetProfileFilenames;

    bpo::options_description desc("Options");
    desc.add_options()
            ("help,h",
                    "Show this help")
            ("input,i",
                    bpo::value<std::string>(&inputFilename)->required()->value_name("FILE"),
                    "Input text file to be obfuscated")
            ("output,o",
                    bpo::value<std::string>(&outputFilename)->required()->value_name("FILE"),
                    "Output file for the obfuscated text")
            ("strip-pos,s",
                    "Strip POS tags from input text")
            ("profile,p",
                    bpo::value<std::string>(&targetProfileFilename)->value_name("FILE")->required(),
                    "Target n-gram profile (will be regenerated if --profile-source is set)")
            ("netspeak,n",
                    bpo::value<std::string>(&netspeakHome)->value_name("DIR")->required(),
                    "Netspeak home directory")
            ("profile-source-files,f",
                    bpo::value<std::vector<std::string>>(&targetProfileFilenames)->multitoken()->value_name("FILE [FILE ...]"),
                    "Source files to generate a target profile from")
            ("profile-strip-pos",
                    "Strip POS tags from target files before generating target profile");

    bpo::variables_map vm;
    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << desc << std::endl;
            return EXIT_SUCCESS;
        }

        vm.notify();

        if (vm.count("profile-source-files") && targetProfileFilenames.empty()) {
            throw bpo::error("--target-files requires at least one filename");
        }
        if (vm.count("profile-strip-pos") && !vm.count("profile-source-files")) {
            throw bpo::error("--profile-strip-pos requires --profile-source-files to be set");
        }
    } catch (bpo::error const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Usage: " << desc << std::endl;
        return EXIT_FAILURE;
    }

    // read source txt
    std::ifstream inputFile;
    inputFile.open(inputFilename);
    if (!inputFile) {
        std::cerr << "Could not open file '" << inputFilename << "'" << std::endl;
        return EXIT_FAILURE;
    }
    std::stringstream inputBuffer;
    inputBuffer << inputFile.rdbuf();
    inputFile.close();

    // open output file
    std::fstream outputFile;
    outputFile.open(outputFilename);
    if (!outputFile) {
        std::cerr << "Could not open output file '" << outputFilename << "'" << std::endl;
        return EXIT_FAILURE;
    }
    LayeredOStream outputBuffer(outputFilename, outputFile);

    // read or generate target profile
    auto targetProfile = std::make_shared<NgramProfile>();
    if (vm.count("profile-source-files")) {
        std::cout << "Generating target profile..." << std::endl;
        unsigned int flags = 0;
        if (vm.count("profile-strip-pos")) {
            flags |= NgramProfile::STRIP_POS_ANNOTATIONS;
        }
        targetProfile->generate(targetProfileFilenames, flags);

        try {
            std::cout << "Saving target profile to '" << targetProfileFilename << "'..." << std::endl;
            targetProfile->save(targetProfileFilename);
        } catch (std::exception const& e) {
            std::cerr << "Error saving target profile: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        try {
            std::cout << "Loading target profile from '" << targetProfileFilename << "'..." << std::endl;
            targetProfile->load(targetProfileFilename);
        } catch (std::exception const& e) {
            std::cerr << "Error loading target profile: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

//    std::cout << "Initializing Netspeak..." << std::endl;
//    try {
//        netspeak_util::init(netspeakHome);
//    } catch (std::exception const& e) {
//        std::cerr << "Error loading Netspeak:\n" << e.what() << std::endl;
//        return EXIT_FAILURE;
//    }

    unsigned int flags = 0;
    if (vm.count("strip-pos")) {
        flags |= NgramProfile::STRIP_POS_ANNOTATIONS;
    }
    Obfuscator obfuscator;
    obfuscator.obfuscate(inputBuffer, outputBuffer, targetProfile, flags);

    outputFile.flush();
    outputFile.close();

    return EXIT_SUCCESS;
}

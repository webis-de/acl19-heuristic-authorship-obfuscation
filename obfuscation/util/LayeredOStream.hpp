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

#ifndef OBFUSCATION_SEARCH_LAYEREDOSTREAM_HPP
#define OBFUSCATION_SEARCH_LAYEREDOSTREAM_HPP

#include <sstream>

/**
 * Layered output stream implementation.
 * Changes to a stream will be passed to the layer below.
 */
class LayeredOStream: public std::stringstream {
public:
    explicit LayeredOStream(LayeredOStream& baseStream); // NOLINT
    explicit LayeredOStream(std::stringstream& baseStream);
    explicit LayeredOStream(char const* filename, std::fstream& baseStream);
    explicit LayeredOStream(std::string const& filename, std::fstream& baseStream);
    virtual LayeredOStream& flushBase();
    virtual LayeredOStream& flushBase(bool truncate);

private:
    std::string m_filename;
    std::iostream& m_baseStream;
};

#endif //OBFUSCATION_SEARCH_LAYEREDOSTREAM_HPP

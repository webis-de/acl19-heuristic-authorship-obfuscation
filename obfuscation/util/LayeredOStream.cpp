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

#include "LayeredOStream.hpp"

#include <fstream>

LayeredOStream::LayeredOStream(LayeredOStream& baseStream)
        : std::stringstream()
        , m_filename()
        , m_baseStream(baseStream)
{
}

LayeredOStream::LayeredOStream(std::stringstream& baseStream)
        : std::stringstream()
        , m_filename()
        , m_baseStream(baseStream)
{
}

LayeredOStream::LayeredOStream(std::string const& filename, std::fstream& baseStream)
        : std::stringstream()
        , m_filename(filename)
        , m_baseStream(baseStream)
{
}

LayeredOStream::LayeredOStream(char const* filename, std::fstream& baseStream)
        : std::stringstream()
        , m_filename(filename)
        , m_baseStream(baseStream)
{
}

/**
 * Flush stream and pass on buffer contents to base stream.
 */
LayeredOStream& LayeredOStream::flushBase()
{
    return flushBase(false);
}

/**
 * Flush stream and pass on buffer contents to base stream.
 *
 * @param truncate truncate the base stream before writing
 */
LayeredOStream& LayeredOStream::flushBase(bool truncate)
{
    flush();

    if (truncate) {
        auto* fstream = dynamic_cast<std::fstream*>(&m_baseStream);
        auto* sstream = dynamic_cast<std::stringstream*>(&m_baseStream);

        if (fstream) {
            if (fstream->is_open()) {
                fstream->close();
            }
            fstream->open(m_filename, std::fstream::out | std::fstream::trunc);
        } else if (sstream) {
            sstream->str("");
        }
    }

    m_baseStream << rdbuf();

    auto* lstream = dynamic_cast<LayeredOStream*>(&m_baseStream);
    if (lstream) {
        lstream->flushBase(truncate);
    } else {
        m_baseStream.flush();
    }

    return *this;
}

# Heuristic Authorship Obfuscation Framework

A* search framework for heuristic authorship obfuscation.
[[Paper Link](https://webis.de/downloads/publications/papers/stein_2019m.pdf)]

    @InProceedings{stein:2019m,
      author =              {Janek Bevendorff and Martin Potthast and Matthias Hagen and Benno Stein},
      booktitle =           {Proceedings of ACL 2019},
      title =               {{Heuristic Authorship Obfuscation}},
      year =                2019
    }

    
## Dependencies
    
    git submodule update --init --recursive
    
    sudo apt install build-essential libboost-locale-dev libboost-serialization-dev \
        libboost-thread-dev libboost-filesystem-dev libboost-program-options-dev \
        libboost-regex-dev libssl-dev

The Netspeak operators are commented out for now, since they need the Netspeak C++
libraries and a local version of the index (not included in this repository).

## Compilation

    mkdir build
    cd build
    cmake ..
    make [-j8]

## Customization

As of now, the search configuration is done in-code. You can find which operators
are being applied at which individual path costs in `obfuscation/Obfuscator.cpp`.

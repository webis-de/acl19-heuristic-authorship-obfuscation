#!/usr/bin/env python3
#
# Copyright 2017-2019 Janek Bevendorff, Webis Group
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Helper script for generating word list dictionaries.

import nltk
from nltk import FreqDist
from nltk.corpus import brown, gutenberg, reuters, wordnet


def clean(s):
    return s.lower().replace("_", " ")


english_frequencies = None


def load_english_frequencies():
    nltk.download(['brown', 'gutenberg', 'reuters'])

    global english_frequencies
    english_frequencies = FreqDist(w.lower() for w in brown.words())
    english_frequencies.update(w.lower() for w in gutenberg.words())
    english_frequencies.update(w.lower() for w in reuters.words())


def generate_hypernyms():
    dictionary = {}
    for pos in ["n", "a"]:
        for synset in wordnet.all_synsets(pos):
            hypernyms = synset.hypernyms()
            if not hypernyms:
                continue

            word_list = []

            for hypernym in hypernyms:
                for lemma in hypernym.lemmas():
                    word = clean(lemma.name())
                    if english_frequencies[word] == 0:
                        continue
                    word_list.append(word)

            if word_list:
                key = clean(synset.lemmas()[0].name())
                dictionary[key] = dictionary.get(key, []) + word_list

    with open("hypernym-dictionary.tsv", "w") as f:
        for key in sorted(dictionary.keys()):
            f.write("{}\t{}\n".format(key, "\t".join(dictionary[key])))


def generate_synonyms():
    dictionary = {}
    for pos in ["n", "a"]:
        for synset in wordnet.all_synsets(pos):
            hypernyms = synset.hypernyms()
            if not hypernyms:
                continue

            word_list = []

            for hypernym in hypernyms:
                for hyponym in hypernym.hyponyms():
                    if not hyponym:
                        continue

                    for lemma in hyponym.lemmas():
                        word = clean(lemma.name())
                        if english_frequencies[word] == 0:
                            continue
                        word_list.append(word)

            if word_list:
                key = clean(synset.lemmas()[0].name())
                dictionary[key] = dictionary.get(key, []) + word_list

    with open("synonym-dictionary.tsv", "w") as f:
        for key in sorted(dictionary.keys()):
            f.write("{}\t{}\n".format(key, "\t".join(dictionary[key])))


def main():
    print("Loading English word frequencies...")
    load_english_frequencies()

    print("Generating hypernym dictionary...")
    generate_hypernyms()

    print("Generating synonym dictionary...")
    generate_synonyms()


if __name__ == '__main__':
    main()

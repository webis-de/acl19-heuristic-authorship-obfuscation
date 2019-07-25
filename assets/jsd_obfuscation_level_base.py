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

# Plot JSD obfuscation epsilon_0 base line of an authorship corpus

from glob import glob
import nltk
from nltk import FreqDist
from math import fsum, log2, sqrt
import numpy as np
import os
import matplotlib.pyplot as plt
import re
import sys


def normalize(string: str) -> str:
    """
    Normalize a text string.

    :param string: input string
    :return: normalized string
    """
    string = string.replace("\xef\xbb\xbf", "")                      # remove UTF-8 BOM
    string = string.replace("\ufeff", "")                            # remove UTF-16 BOM
    # string = unicodedata.normalize("NFKD", string)                   # convert to NFKD normal form
    string = re.compile(r"[0-9]").sub("0", string)                   # map all numbers to "0"
    string = re.compile(r"(?:''|``|[\"„“”‘’«»])").sub("'", string)   # normalize quotes
    string = re.compile(r"(?:[‒–—―]+|-{2,})").sub("--", string)      # normalize dashes
    string = re.compile(r"\s+").sub(" ", string)                     # collapse whitespace characters

    return string.strip()


def kld_i(p, q):
    """
    Calculate a single KLD summand.

    :param p: probability 1
    :param q: probability 2
    :return: KLD summand
    """
    if p == 0:
        return 0.0

    return p * log2(p / q)


def js_dist(t1_freq, t2_freq):
    """
    Calculate Jensen-Shannon distance.

    :param t1_freq: text 1 distribution
    :param t2_freq: text 2 distribution
    :return:
    """
    mixed_freq = set(t1_freq) | set(t2_freq)

    jsd_vals_p = []
    jsd_vals_q = []

    for ngram in mixed_freq:
        p_norm = t1_freq[ngram] / t1_freq.N()
        q_norm = t2_freq[ngram] / t2_freq.N()

        m = 0.5 * (p_norm + q_norm)
        jsd_vals_p.append(kld_i(p_norm, m))
        jsd_vals_q.append(kld_i(q_norm, m))

    return sqrt(fsum(jsd_vals_p) + fsum(jsd_vals_q))


def hellinger_dist(t1_freq, t2_freq):
    """
    Calculate Hellinger distance.

    :param t1_freq: text 1 distribution
    :param t2_freq: text 2 distribution
    :return:
    """
    mixed_freq = set(t1_freq) | set(t2_freq)
    dist_vals = []

    for ngram in mixed_freq:
        p_norm = t1_freq[ngram] / t1_freq.N()
        q_norm = t2_freq[ngram] / t2_freq.N()
        dist_vals.append(pow(sqrt(p_norm) - sqrt(q_norm), 2))

    # return 1.0 / sqrt(2) * sqrt(fsum(dist_vals))
    return sqrt(fsum(dist_vals))


def byte_ngrams(t1, t2, order=3):
    """
    Generate byte n-gram distributions.

    :param t1: text1
    :param t2: text2
    :param order: n-gram order
    :return: tuple containing FreqDists
    """

    t1 = t1.encode(encoding="utf-8")
    t2 = t2.encode(encoding="utf-8")

    t1_freq = FreqDist(tuple(t1[i:i + order]) for i in range(len(t1) - order + 1))
    t2_freq = FreqDist(tuple(t2[i:i + order]) for i in range(len(t2) - order + 1))

    return t1_freq, t2_freq


def pos_ngrams(t1, t2, order=3):
    """
    Generate POS n-gram distributions.

    :param t1: text1
    :param t2: text2
    :param order: n-gram order
    :return: tuple containing FreqDists
    """

    t1_freq = FreqDist()
    t2_freq = FreqDist()

    t1 = nltk.sent_tokenize(t1)
    for s in t1:
        pos_tags = nltk.pos_tag(nltk.word_tokenize(s))
        t1_freq.update(tuple(map(lambda x: x[1], pos_tags[i:i + order])) for i in range(len(pos_tags) - order + 1))

    t2 = nltk.sent_tokenize(t2)
    for s in t2:
        pos_tags = nltk.pos_tag(nltk.word_tokenize(s))
        t2_freq.update(tuple(map(lambda x: x[1], pos_tags[i:i + order])) for i in range(len(pos_tags) - order + 1))

    return t1_freq, t2_freq


def remove_outliers_iqr(y, min, max):
    """
    Remove outliers using the interquartile range (IQR) method by John Tukey.

    :param y: y values
    :param min: lower quartile
    :param max: upper quartile
    :return: non-outliers
    """
    if type(y) is not np.ndarray:
        y = np.array(y)

    q1, q2 = np.percentile(y, [min, max])
    iqr = q2 - q1
    lower_bound = q1 - (iqr * 1.5)
    upper_bound = q2 + (iqr * 1.5)

    return y[(y > lower_bound) & (y < upper_bound)]


def main():
    if len(sys.argv) < 2:
        print("Usage: {} PATH".format(os.path.basename(sys.argv[0])), file=sys.stderr)
        exit(1)

    corpus = sys.argv[1]

    cases = {}
    with open(os.path.join(corpus, "truth.txt"), "r") as f:
        for l in f:
            fields = l.strip().split(" ")
            cases[fields[0]] = fields[1]

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_ylim((0.4, 1.5))
    ax.set_xlabel("Text length (characters)")
    ax.set_ylabel("JS distance")

    labels_set = {"Y": False, "N": False}
    min_x = 100
    max_x = min_x

    # x and y values of different-authors curves
    diff_xs = []
    diff_ys = []

    for case in list(cases.keys()):
        print("Case: {}".format(case))

        known_text = ""
        for fname in glob(os.path.join(corpus, case, "known*.txt")):
            known_text += normalize(open(fname, encoding="utf-8").read())

        unknown_text = normalize(open(os.path.join(corpus, case, "unknown.txt"), encoding="utf-8").read())

        max_x_tmp = min(len(known_text), len(unknown_text))
        max_x = max(max_x, max_x_tmp)
        x = tuple(range(min_x, max_x_tmp + 1, 100))
        y = []
        for xi in x:
            known_xi = known_text[:xi]
            unknown_xi = unknown_text[:xi]
            n1, n2 = byte_ngrams(known_xi, unknown_xi)
            d = js_dist(n1, n2)
            # d = hellinger_dist(n1, n2)
            y.append(d)

            if cases[case] == "N":
                if len(y) > len(diff_ys):
                    diff_xs.append(xi)
                    diff_ys.append([d])
                else:
                    diff_ys[len(y) - 1].append(d)

        # only show one label for each curve type
        color = "#ffbf00" if cases[case] == "Y" else "#7a16ff"
        label = ""
        if not labels_set[cases[case]]:
            label = "same author" if cases[case] == "Y" else "different authors"
            labels_set[cases[case]] = True

        plt.semilogx(x, y, color=color, basex=2, label=label, linewidth=0.5, alpha=0.36)

    # calculate epsilon thresholds
    eps_x = []
    eps0_y = []
    eps05_y = []
    eps07_y = []
    eps099_y = []
    for x, y in zip(diff_xs, diff_ys):
        if x < 2048:
            continue

        if len(y) > 4:
            y = remove_outliers_iqr(y, 30, 70)

        if len(y) < 2:
            continue

        eps_x.append(x)
        eps0_y.append(np.percentile(y, 0))
        eps05_y.append(np.percentile(y, 50))
        eps07_y.append(np.percentile(y, 70))
        eps099_y.append(np.percentile(y, 99))

    # fit epsilon lines
    e0_coefs = np.polyfit([log2(x) for x in eps_x], eps0_y, 1)
    e05_coefs = np.polyfit([log2(x) for x in eps_x], eps05_y, 1)
    e07_coefs = np.polyfit([log2(x) for x in eps_x], eps07_y, 1)
    e099_coefs = np.polyfit([log2(x) for x in eps_x], eps099_y, 1)
    line_x = tuple(range(min_x, max_x + 1, (max_x - min_x) // 2))

    print("\nε_{{0}} coefs: [{:.05}, {:.05}]".format(float(e0_coefs[0]), float(e0_coefs[1])))
    plt.semilogx(line_x, [e0_coefs[0] * log2(x) + e0_coefs[1] for x in line_x],
                 color="#e54709", basex=2, label=r"$\epsilon_0$", linewidth=1.5)

    print("\nε_{{0.5}} coefs: [{:.05}, {:.05}]".format(float(e05_coefs[0]), float(e05_coefs[1])))
    plt.semilogx(line_x, [e05_coefs[0] * log2(x) + e05_coefs[1] for x in line_x],
                 color="#3f00af", basex=2, label=r"$\epsilon_{0.5}$", linewidth=1.5, linestyle="dashed")

    print("\nε_{{0.7}} coefs: [{:.05}, {:.05}]".format(float(e07_coefs[0]), float(e07_coefs[1])))
    plt.semilogx(line_x, [e07_coefs[0] * log2(x) + e07_coefs[1] for x in line_x],
                 color="#3f00af", basex=2, label=r"$\epsilon_{0.7}$", linewidth=1.5, linestyle="dashed")

    print("\nε_{{0.99}} coefs: [{:.05}, {:.05}]".format(float(e099_coefs[0]), float(e099_coefs[1])))

    plt.legend()
    plt.show(block=True)


if __name__ == "__main__":
    main()

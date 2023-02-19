#!/usr/bin/python3

# Copyright (c) 2021-2023 Ben Ashbaugh
#
# SPDX-License-Identifier: MIT or Apache-2.0

import gen

from mako.template import Template
from mako.exceptions import RichTraceback

from collections import OrderedDict
from collections import namedtuple

import argparse
import sys
import urllib
import xml.etree.ElementTree as etree
import urllib.request

# parse_xml - Helper function to parse the XML file from a URL or local file.
def parse_xml(path):
    file = urllib.request.urlopen(path) if path.startswith("http") else open(path, 'r')
    with file:
        tree = etree.parse(file)
        return tree

# noneStr - returns string argument, or "" if argument is None.
def noneStr(s):
    if s:
        return s
    return ""

if __name__ == "__main__":
    args = gen.parse_args()
    spec = gen.load_spec(args)

    apisigs = gen.get_apisigs(spec)

    extapis = gen.get_extapis(spec, apisigs)

    try:
        loader_template = Template(filename='openclext.cpp.mako')

        print('Generating openclext.cpp...')
        gen = open(args.directory + '/openclext.cpp', 'wb')
        gen.write(
          loader_template.render_unicode(
              genExtensions={},
              spec=spec,
              apisigs=apisigs,
              extapis=extapis).
          encode('utf-8', 'replace'))

        test_template = Template(filename='call_all.c.mako')

        print('Generating call_all.c test function...')
        gen = open(args.directory + '/call_all.c', 'wb')
        gen.write(
          test_template.render_unicode(
              genExtensions={},
              spec=spec,
              apisigs=apisigs,
              extapis=extapis).
          encode('utf-8', 'replace'))

    except:
        traceback = RichTraceback()
        for (filename, lineno, function, line) in traceback.traceback:
            print('%s(%s) : error in %s' % (filename, lineno, function))
            print('    ', line)
        print('%s: %s' % (str(traceback.error.__class__.__name__), traceback.error))

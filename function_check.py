#!/usr/bin/env python

import argparse
import re
import common
import json



if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', dest='configfile', help='config file', default='config.json')
    args = parser.parse_args()

    functable, spec_functable = common.parse(args.configfile)

    FUNCNAME = 0
    with open('/proc/kallsyms') as f:
        content = f.read()
    
    for i in range(len(functable)):
        pattern=r'\b'+functable[i][FUNCNAME]+r'(\.(isra|constprop)\.[0-9]+\b){0,1}'
        result = re.search(pattern, content)
        if result==None:
            print("function %s cannot be found in current kernel version" % \
                    functable[i][FUNCNAME])

    for i in range(len(spec_functable)):
        pattrn=r'\b'+spec_functable[i][FUNCNAME]+r'(\.(isra|constprop)\.[0-9]+\b){1}'
        result = re.search(pattern, content)
        if result==None:
            print("function %s cannot be found in current kernel version" % \
                    spec_functable[i][FUNCNAME])

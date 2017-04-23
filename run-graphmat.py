#!/usr/bin/env python

import sys
import os
import json
from pprint import pprint
import subprocess
import argparse
import time

GRAPHMAT_DIR = "/nobackup/swapnilh/benchmarks/GraphMat/"
OUT_DIR = "/u/s/w/swapnilh/private/2017_FOMA/results-address-translation/"

def execute(cmd):
    p = subprocess.call(cmd, shell=True)
#    p.wait()
    
def setup_env():
    os.environ['LD_LIBRARY_PATH']= os.environ['LD_LIBRARY_PATH'] + '/nobackup/swapnilh/pin-tools/pin-3.2-81205-gcc-linux/intel64/runtime/pincrt/' +\
            '/nobackup/swapnilh/pin-tools/pin-3.2-81205-gcc-linux/intel64/lib-ext/' +\
            '/nobackup/swapnilh/pin-tools/pin-3.2-81205-gcc-linux/extras/xed-intel64/lib'
    os.environ['OMP_NUM_THREADS'] = '1'

def main(argv):
    setup_env()
    
    parser = argparse.ArgumentParser(description='Runscript for the GraphMat workloads')
    parser.add_argument('-w', action="append", dest='workloads_list', default=[])
    parser.add_argument('-d', action="append", dest='databases_list', default=[])
    parser.add_argument('-o', action="store", dest='out_dir', default='logs-'+time.strftime("%d-%b"))
    results = parser.parse_args()

    print "Settings if custom values passed"
    print "Workloads: " + str(results.workloads_list)
    print "Databases: " + str(results.databases_list)
    print "Output Dir: " +  OUT_DIR + results.out_dir
    
    with open(GRAPHMAT_DIR + 'workloads.json') as data_file:    
        data = json.load(data_file)
    for workload in data:
        if results.workloads_list != [] and workload not in results.workloads_list:
            continue
        for parameters in data[workload]:
            if results.databases_list != [] and parameters['database'] not in results.databases_list:
                print parameters['database']
                continue
            run_cmd = GRAPHMAT_DIR + 'bin/' + workload + ' '\
                    + GRAPHMAT_DIR + 'data/' + parameters['database']\
                    + ' ' + parameters['args']
            print(run_cmd)
            execute(run_cmd)    

if __name__ == "__main__":
    main(sys.argv[1:])

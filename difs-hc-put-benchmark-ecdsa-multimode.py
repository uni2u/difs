#!/usr/bin/python3
import csv
import shutil
import signal
import tempfile

from time import sleep
from datetime import datetime
from multiprocessing.pool import ThreadPool
from subprocess import DEVNULL, Popen, TimeoutExpired, call
import subprocess
from pprint import pprint
from functools import partial

CONCURRENCY = 1
REPEAT = 10
put_id = 1

REPO_DIR = '/tmp/repo/'


def run_get(id_):
    start = datetime.now()
    ndn_cmd = "/hashchain/%d" % (put_id)
    print(f"ndngetfile /difs {ndn_cmd}")
    p = Popen(
        ['ndngetfile', '/difs', ndn_cmd],
        stdout=DEVNULL, stderr=DEVNULL)
    code = p.wait()
    end = datetime.now()
    delta = end - start
    return (id_, code, delta.total_seconds())

def run_del(id_):
    start = datetime.now()
    ndn_cmd = "/hashchain/%d" % (put_id)
    print(f"ndndelfile /difs {ndn_cmd}")
    p = Popen(
        ['ndndelfile', '/difs', ndn_cmd],
        stdout=DEVNULL, stderr=DEVNULL)
    code = p.wait()
    end = datetime.now()
    delta = end - start
    return (id_, code, delta.total_seconds())

def run_put(id_, tfile):
    global  put_id
    print("id_ :",put_id)
    print("tfile: ", tfile)
    ndn_name = "/hashchain/%d" % (put_id)

    print("ndn_name: ",ndn_name)
    start = datetime.now()
    ndn_cmd = "ndnputfile -c /difs /hashchain/%d %s" % (put_id,tfile.name)
    print("ndn_cmd: ",ndn_cmd)
    p = Popen(
        [ndn_cmd],
        shell=True, stdout=DEVNULL, universal_newlines=True)
        # shell=True, stdout=subprocess.PIPE, universal_newlines=True)
    code = p.wait()
    print("code is :",code)
    # p.communicate()
    # result = p.stdout.readlines()
    end = datetime.now()
    delta = end - start
    put_id = put_id + 1
    return (id_, code, delta.total_seconds())

def run_test(size):
    id = 0

    with tempfile.NamedTemporaryFile() as tfile:
        print('Testing ... multimode')


        for iteration in range(REPEAT):
            print(f'Iteration {iteration}')

            # temp_put = partial(run_put, tfile = tfile)
            # id = id+1
            # pool = ThreadPool(CONCURRENCY)
            # results = pool.map(temp_put, [id])
            # yield [
            #     (size, iteration, *values)
            #     for values in results
            # ]
            id = id+1
            pool = ThreadPool(CONCURRENCY)
            results = pool.map(run_get, [id])
            yield [
                (size, iteration, *values)
                for values in results
            ]            
            # results = pool.map(run_del, [id])
            # yield [
            #     (size, iteration, *values)
            #     for values in results
            # ]

sizes = [2 ** i for i in range(27, 31)]
# sizes = [2 ** 27]

import time
timestr = time.strftime("%Y%m%d")
with open(f'{timestr}-hashchain-multimode-ecdsa-results.tsv', 'w') as tsvfile:
    writer = csv.writer(tsvfile, delimiter='\t')
    for size in sizes:
        for row in run_test(size):
            pprint(row)
            writer.writerows(row)
            tsvfile.flush()
        put_id = put_id + 1

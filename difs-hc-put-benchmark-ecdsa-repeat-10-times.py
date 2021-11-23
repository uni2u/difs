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


# def reset_repo():
#     print('Resetting repo')
#     try:
#         call(['killall', 'ndn-difs'])
#         shutil.rmtree(REPO_DIR)
#     except:
#         pass

def run_get(id_):
    start = datetime.now()
    p = Popen(
        ['ndngetfile', '/difs', '/hashchain/1'],
        stdout=DEVNULL, stderr=DEVNULL)
    code = p.wait()
    end = datetime.now()
    delta = end - start
    return (id_, code, delta.total_seconds())

def run_del(id_):
    start = datetime.now()
    ndn_cmd = "/hashchain/%d" % (put_id-1)
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
    # reset_repo()
    # p_nfd, p_repo = None, None

    # def restart_repo(p_nfd, p_repo):
    #     print('Restarting repo')
    #     if p_repo:
    #         p_repo.send_signal(signal.SIGINT)
    #         try:
    #             p_repo.wait(1)
    #         except TimeoutExpired:
    #             p_repo.kill()

    #     if p_nfd:
    #         p_nfd.send_signal(signal.SIGINT)
    #         try:
    #             p_nfd.wait(1)
    #         except TimeoutExpired:
    #             p_nfd.kill()

    #     p_repo = Popen(['ndn-difs', '-c', 'manager-sign.conf'], stdout=DEVNULL)
    #     sleep(5)

    #     return p_nfd, p_repo

    with tempfile.NamedTemporaryFile() as tfile:
        print(f'Testing with {size}')


        # p_nfd, p_repo = restart_repo(p_nfd, p_repo)

        call([
            'fallocate', '-l', str(size), tfile.name
        ])

        for iteration in range(REPEAT):
            print(f'Iteration {iteration}')

            #p_nfd, p_repo = restart_repo(p_nfd, p_repo)

            temp_put = partial(run_put, tfile = tfile)
            id = id+1
            pool = ThreadPool(CONCURRENCY)
            results = pool.map(temp_put, [id])
            yield [
                (size, iteration, *values)
                for values in results
            ]
            results = pool.map(run_del, [id])
            yield [
                (size, iteration, *values)
                for values in results
            ]
        #p_repo.kill()

# sizes = [2 ** i for i in range(27, 31)]
sizes = [2 ** 27]

with open('2021-11-22-hashchain-ecdsa-results.tsv', 'w') as tsvfile:
    writer = csv.writer(tsvfile, delimiter='\t')
    for size in sizes:
        for row in run_test(size):
            pprint(row)
            writer.writerows(row)
            tsvfile.flush()

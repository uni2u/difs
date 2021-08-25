#!/usr/bin/env python3
import datetime
import shlex
import subprocess
import time

branch = 'master'

def test(sha256: bool):
    subprocess.call(shlex.split('rm -rf /tmp/repo/'))
    nfd = subprocess.Popen(
        shlex.split('nfd'),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL)
    time.sleep(1)

    repo = subprocess.Popen(shlex.split('./build/bin/ndn-difs -c node01.conf'))
    time.sleep(1)

    start = datetime.datetime.now()
    ret = subprocess.call(shlex.split('build/bin/ndnputfile /difs /demo file'))
    end = datetime.datetime.now()

    if ret != 0:
        print('ERROR')

    nfd.terminate()
    repo.terminate()

    diff = end - start
    return diff

with open('result.txt', 'a') as file:
    file.write(f'{branch}\n')

    for i in range(10):
        result = test('True')
        file.write(f'{result.total_seconds():.6f}\n')
        file.flush()

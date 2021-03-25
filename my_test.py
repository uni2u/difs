#!/usr/bin/env python3
import datetime
import shlex
import subprocess
import time
 
branches = ('difs', 'hash-chain')
 
 
def test(sha256: bool):
    subprocess.call(shlex.split('rm -rf /var/lib/ndn/repo/0'))
    nfd = subprocess.Popen(
        shlex.split('nfd'),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL)
    time.sleep(1)
 
    repo = subprocess.Popen(shlex.split('./build/bin/ndn-repo-ng -c repo-0.conf'))
    time.sleep(1)
 
    start = datetime.datetime.now()
    flag = 'D' if sha256 else ''
    ret = subprocess.call(shlex.split(
        f'build/bin/ndnputfile -{flag}s 1000 /example/repo '
        f'/example/data/0 file'))
    end = datetime.datetime.now()
 
    if ret != 0:
        print('ERROR')
 
    nfd.terminate()
    repo.terminate()
 
    diff = end - start
    return diff
 
 
with open('result.txt', 'a') as file:
    for branch in branches:
        print(f'branch {branch}')
        subprocess.call(shlex.split(f'git checkout {branch}'))
        subprocess.call(shlex.split(f'./waf'))
        file.write(f'{branch}\n')
 
        for flag in (True, False):
            print(f'sha256 {flag}')
            file.write(('Sha256' if flag else 'RSA') + '\n')
 
            for i in range(10):
                result = test(flag)
                file.write(f'{result.total_seconds():.6f}\n')
                file.flush()

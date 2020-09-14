import hashlib
import json
import shlex
import subprocess
import sys

from os import path
from pprint import pprint

import paramiko


PATH = '/var/lib/ndn/repo/'


def sync(remote):
    subprocess.call(
        shlex.split(f'rsync -a --no-g --no-o {remote}:{PATH} {PATH}'),
#        shlex.split('rsync -a --no-g --no-o '+remote+':'+PATH+' '+ PATH),
        stdout=subprocess.DEVNULL)


def edit_manifest(remote, localrepo, ndnname):
    hashid = hashlib.sha1(ndnname.encode()).hexdigest()

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(remote)
    sftp = ssh.open_sftp()
    filename = path.join(PATH, 'manifest', hashid)
    print(filename)
    with sftp.open(filename) as file:
        manifest = json.load(file)

    storage = manifest['storages'][0].copy()
    storage['storage_name'] = localrepo
    manifest['storages'].append(storage)
    pprint(manifest)

    with sftp.open(filename, 'w') as file:
        file.write(json.dumps(manifest))

    sftp.close()
    ssh.close()


def run(data_host, manifest_host, localrepo, ndnname):
    sync(data_host)
    edit_manifest(manifest_host, localrepo, ndnname)


if __name__ == '__main__':
    try:
        _, data_host, manifest_host, localrepo, ndnname = sys.argv
    except ValueError:
        print(
            'Usage: ndncopyfile <data host> <manifest host> '
            '<local prefix> <ndnname>')
        exit(1)
    else:
        run(data_host, manifest_host, localrepo, ndnname)

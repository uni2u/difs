# NDNFS

**본 문서는 [ndnfs-port](https://github.com/remap/ndnfs-port) 의 README 에 대한 내용임**

- Port of NDN file system ([NDNFS](https://github.com/wentaoshang/NDNFS) 최초 작성자: Wentao; Qiuhan)
- build 및 install 사항은 [INSTALL,md](https://github.com/zhehaowang/ndnfs-port/blob/master/INSTALL.md) 참조
- 버그 및 리포트: [zhehao@remap.ucla.edu](mailto:zhehao@remap.ucla.edu)

## Usage

NDNFS 는 NDN-친화적인 (friendly) file system 이다. 마운트 되면 파일 시스템의 데이터는 NDNFS-server 를 통해 NDN 을 사용하여 가져올 (fetched) 수 있다.

실행:
```
$ mkdir /tmp/ndnfs
$ mkdir /tmp/dir
$ ./build/ndnfs -s [actual folder path] [mount point path]
```

- [Actual folder path]: 파일이 실제 local file system 에 저장되는 위치
- [mount point path]: 마운트 포인트
- '-s': FUSE 가 단일 스레드를 실행하도록 함
- '-d': NDNFS 의 모든 디버그 출력
`$ ./build/ndnfs -d /tmp/dir /tmp/ndnfs`
- '-f': foreground 에서 실행하고 디버그 정보 확인
`$ ./build/ndnfs -s -f /tmp/dir /tmp/ndnfs`
  - '-f' 를 사용하는 경우 'ndnfs' 프로세스가 종료되면 NDNFS 가 자동으로 마운트 해제됨
- 마운트 해제
`$ umount /tmp/ndnfs`
- 마운트 강제 해제 '-f'
`$ umount -f /tmp/ndnfs`
- '-o prefix = [prefix]': ndnfs file prefix 구성
- '-o log = [log file path]': ndnfs log file 경로 구성
- '-o db = [database file path]': ndnfs db file 경로 구성
`$ ./build/ndnfs /tmp/dir /tmp/ndnfs -o prefix=/ndn/broadcast/ndnfs -o log=ndnfs.log -o db=/home/zhehao/ndnfs.db`
  - prefix "/ndn/broadcast/ndnfs" 를 사용하여 /tmp/dir 을 /tmp/ndnfs 로 마운트하고,
  - 실행중인 디렉토리의 ndnfs.log 에 로그를 작성하고,
  - /home/zhehao/ndnfs.db 를 DB 파일로 사용 (현재 db 파일의 절대 경로를 사용)
- 현재 구현에서 ndnfs 를 실행하기 전에 실제 경로에 이미 존재하는 파일을 검색하지 않음
  - NDNFS-server 를 통해 파일을 사용할 수 있게 하려면 NDNFS 를 실행한 후 마운트 지점에 해당 파일을 위치하여야 함

## NDNFS-server

NDNFS-server 는 NDN 을 통해 원격으로 NDNFS 의 읽기 access 를 지원한다.

실행: (FS server 를 실행하기 전 [NFD](https://github.com/named-data/NFD) 를 설치하고 구성하여야 함)

`$ ./build/ndnfs-server`

- '-p': prefix 구성
- '-d': db 파일 선택
- '-f': 파일 시스템 root 식별 (NDNFS 구성과 동일하여야 함)
- '-l': 로그 파일 경로 구성
`$ ./build/ndnfs-server -p /ndn/broadcast/ndnfs -l ndnfs-server.log -f /tmp/ndnfs -d /tmp/ndnfs.db`
  - prefix "/ndn/broadcast/ndnfs" 를 사용하고,
  - ndnfs-server.log 에 로그를 작성하고,
  - 실행중인 디렉토리의 DB 파일로 ndnfs.db 를 사용하며
  - 마운트 된 /tmp/ndnfs 의 콘텐츠를 제공함

빠른 테스트를 위해 NFD, NDNFS-server 및 NDNFS 가 실행중인지 확인
```
$ echo "Hello, world!" > /tmp/ndnfs/test.txt
$ ndnpeek -pfPv /ndn/broadcast/ndnfs/test.txt
```
- NDN 을 통해 제공되는 'test.txt' 파일 확인 (ndnpeek 는 [ndntools](https://github.com/named-data/ndn-tools) 를 설치하여야 함)

## NDNFS-client

NDNFS 데이터에 원격으로 access 하려면 [NDN-JS library](https://github.com/named-data/ndn-js) 를 활용한 [NDN-JS Firefox addon](https://github.com/named-data/ndn-js/blob/master/ndn-protocol.xpi?raw=true) 를 사용할 수 있다; 또는 테스트의 built-in client 를 활용할 수 있다. (Firefox addon client 를 사용하여 테스트 하는 것을 권장함)

**Instructions for Firefox addon:**
- locally NFD 실행
- Firefox addon's hub 를 localhost 로 설정
- nfdc 를 호스트 (ndnfs-server 또는 ndn testbed 의 호스트) 에 연결 (localhost 도 가능하며 localhost 를 사용하는 경우 nfdc 가 필요하지 않음)
- "[prefix]/[file or directory]" 경로를 URI 로 입력하고 가져옴 (fetch)

현재 Firefox addon 은 확장자에 따라 파일의 MIME 유형을 유추하려고 시도하며 지정된 MIME 유형에 대한 기본 동작을 호출한다. (.pdf 의 예외처리: pdf,js 대신 다운로드 대화상자 호출)

기본 동작 mimetype 을 알 수 없는 NDNFS 의 콘텐츠는 파일로 저장된다.

**Instructions for client application:**
- locally NFD 실행
- nfdc 를 호스트 (ndnfs-server 또는 ndn testbed 의 호스트) 에 연결 (localhost 도 가능하며 localhost 를 사용하는 경우 nfdc 가 필요하지 않음)
- 클라이언트 application 실행
`$ ./build/test-client`
- "show [prefix]/[file or directory]": 실행중인 클라이언트에서 특정 파일 또는 디렉토리의 metadata 를 찾음
- "fetch [prefix]/[file or directory][local save path]": 특정 파일을 가져와서 로컬로 저장

## New features

- network-ready data packets 대신 sqlite3 데이터베이스에 서명만 저장하고 요청시 NDN 데이터 패킷을 조립
- 새로운 meta-info branch (분기) 에 mime_type 을 publish
- NDNJS Firefox addon 및 최신 버전의 NDN-CPP 와 동작하도록 업데이트
- 비동기적 서명 활용 (실험중이며 local branch 에 위치함)

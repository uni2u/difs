
# issue list check

- 참조 사이트: [repo-ng redmine](https://redmine.named-data.net/projects/repo-ng)
- commit history: [repo-ng gerrit](https://gerrit.named-data.net/q/project:repo-ng)

## Feature

|상태|우선순위|제목|날짜|
|:---:|:---:|:---|:---|
|In Progress|Normal|[Forwarding hint support](https://redmine.named-data.net/issues/4634)|2019/05/15|
|Closed|High|[Automatic per-data-packet (per-group) prefix registrations](https://redmine.named-data.net/issues/4247)|2017/10/23|
|New|Normal|[Management Dispatcher for repo commands](https://redmine.named-data.net/issues/4129)|2018/10/27|
|New|Normal|[Storage management/quota features](https://redmine.named-data.net/issues/3768)|2016/09/07|
|New|High|[Provide option to enable prefix caching behavior](https://redmine.named-data.net/issues/3767)|2016/09/07|
|New|High|[Enable external command to trigger prefix registration with local NFD](https://redmine.named-data.net/issues/3766)|2017/06/08|
|Closed|Normal|[ndnwatchfile: Tool to execute repo watch protocol](https://redmine.named-data.net/issues/1791)|2017/06/08|
|Closed|Normal|[New Insert Protocol: Watching prefix](https://redmine.named-data.net/issues/1784)|2017/06/08|
|Closed|Normal|[Configuration to enable and disable validation](https://redmine.named-data.net/issues/1778)|2017/06/08|
|Closed|Normal|[Backdoor to insert data packets into repo](https://redmine.named-data.net/issues/1485)|2014/04/19|

### check item

- [Backdoor to insert data packets into repo](https://redmine.named-data.net/issues/1485)
  - 상태: 완료됨 
  - data packet 을 repository 에 insert 하는 backdoor 기능
  - ccnx repo STATUS_PORT feature 와 유사하여야 함
  - repo-ng 는 sqlite3 을 사용하고 있으며 sqlite3 을 활용한 해당 기능이 완료됨을 보고

- [Configuration to enable and disable validation](https://redmine.named-data.net/issues/1778)
  - 상태: 완료됨
  - 유효성 검사에 대한 활성/비활성 선택 구성
  - repo 는 서명된 interest 및 데이터 패킷의 유효성 검증을 '사용/비사용' 구성이 가능하여야 함
  - configure 파일을 활용하여 수행 가능
```
[repo-ng.conf]

repo
{
  ...
  validator
  {
    trust-anchor
    {
      type any
    }
    ...
  }
  ...
}
```

- [New Insert Protocol: Watching prefix](https://redmine.named-data.net/issues/1784)
  - 상태: 완료됨
  - 상태 감시를 위한 새로운 insert 프로토콜이 필요함 (watching prefix)
  - `watching prefix` 를 지원하여 repo 에서 동일한 prefix 를 사용하지만 exclude selector 가 다른 데이터 (매번 업데이트됨) 를 요청 (interest) 하여야 함
  - watched prefix insertion protocol 변경 이슈는 거의 없음
    - 시작: /ucla/cs/repo/**watch**/**start**/...
    - 확인: /ucla/cs/repo/**watch**/**check**/...
    - 중지: /ucla/cs/repo/**watch**/**stop**/...
  - 불필요한 (redundant) 파라미터 컴포넌트인 'WatchStatus' 를 제거함
  - 코드 반영: [repo-ng code gerrit](https://gerrit.named-data.net/1052)

- [ndnwatchfile: Tool to execute repo watch protocol](https://redmine.named-data.net/issues/1791)
  - 상태: 완료됨
  - repo watch protocol 을 실행하는 tool 이 필요함
  - 이 tool 은 'watch request', 'watch status' prefix 에 대하여 'watch stop' 을 보낼 수 있어야 함
    - 'processId' 를 'watched' prefix 로 교체
  - 코드 반영: [repo-ng code gerrit](https://gerrit.named-data.net/1073)

- [Enable external command to trigger prefix registration with local NFD](https://redmine.named-data.net/issues/3766)
  - 상태: 시작
  - localNFD 로 prefix 등록을 트리거 하기위한 외부 (external) 명령 (command) 활성화
    - repository 원격 사용자가 제어 가능한 네트워크에서 콘텐츠를 사용할 수 있도록 요청할 수 있어야 함
  - [_마지막 의견_]
    - 소비자 (consumer) 가 repository 에 prefix 를 등록하도록 하지 않고 repository 를 가리키는 NextHopFaceId 로 interest 를 보내는 방법?

- [Provide option to enable prefix caching behavior](https://redmine.named-data.net/issues/3767)
  - 상태: 시작
  - prefix caching 을 활성화하는 옵션 필요
    - 일부 응용의 경우 지정된 prefix 에 대한 모든 내용을 repository 에 저장하는 것이 좋음
    - 이를 달성하기 위하여 repository 가 원하는 prefix (예: root 노드) 를 등록하고 interest 를 반영함
    - prefix caching 동작 (behavior) 은 ndn-rtc traffic 기록 storage 를 제공
  - [_마지막 의견_]

- [Storage management / quota features](https://redmine.named-data.net/issues/3768)
  - 상태: 시작
  - storage 관리/할당 (quota) feature 필요성
    - prefix 또는 인증서 (certificate) 당 storage 할당량을 적용하고 이전 데이터를 보존할 수 있는 기능이 필요함
  - [_마지막 의견_]

- [Management Dispatcher for repo commands](https://redmine.named-data.net/issues/4129)
  - 상태: 시작
  - repo command 를 위한 management dispatcher 필요
    - 'insert', 'delete', 'watch' command 를 허용하기 위한 'ndn::mgmt::Dispatcher' 사용
  - [_마지막 의견_]
    - v1 유효성 검증에 대한 dependency 를 줄이기 위해 필요함
    - dispatcher 를 통합함 (v1, v2 통합)
      - v2::security 통합 테스트 실패
      - v1::security 통합 테스트 실패
    - 테스트 실패에 대하여 응답
      - ndn-cxx v1 Validator, 특히 ValidatorNull 을 사용하여야 함
      - v2 Validator 로의 전환은 [#4019](https://redmine.named-data.net/issues/4091) 에서 수행되고 있으며 v2 ValidationPolicyConfig 가 준비되어야 함

- [Automatic per-data-packet (per-group) prefix registrations](https://redmine.named-data.net/issues/4247)
  - 상태: 완료됨
  - 데이터 패킷 (그룹) 당 자동 prefix 등록
    - v2::Certificate 로 전환 후 데이터 패킷이 있을 수 있는 passing node 에서 repository 의 인증서를 제공하는 기능을 복원 (restore) 할 필요가 있음
    - 이렇게 하기 위하여 repo 가 데이터의 prefix 를 localNFD 에 자동으로 등록하여야 함
  - [_마지막 의견_]
    - 저장된 각 데이터 패킷의 정확한 name 을 등록하면 인증서 검색 (retrieve) interest 는 데이터 name 의 실제 prefix 이기 때문에 문제가 없음
      - prefix 등록의 세분화를 결정하는 configuration 옵션 (예: 데이터 name 에서 마지막 _k_ 컴포넌트 제외
      - 파라미터를 저장하는 데이터베이스 필드와 함께 세분화를 결정하는 repo insert 명령의 파라미터
    - 프로그램 로직은 prefix 등록이 필요한 마지막 데이터가 삭제될 때까지 prefix 등록 절차가 종료되지 않도록 하여야 함
    - 주어진 ReadHandle 은 데이터 prefix 등록을 담당하고 RepoStorage 는 데이터 insert 및 deletion 시 신호를 보내야하며 ReadHandle 은 이러한 신호에 반응하여야 함
      - 데이터 insert 후 빠르게 delete 하면 [#2720](https://redmine.named-data.net/issues/2720) 등록 취소에 영향을 줄 수 있음
      - 시작시 많은 등록이 있는 경우 [#1589](https://redmine.named-data.net/issues/1589), [#2174](https://redmine.named-data.net/issues/2174) 는 [#2293](https://redmine.named-data.net/issues/2293) 이 구현될 때 까지 등록 실패를 일으킬 수 있음

- [Forwarding hint support](https://redmine.named-data.net/issues/4634)
  - 상태: In Progress
  - Forwarding hint 지원
    - insert 명령 interest 에서 위임 (delegation) 리스트 디코딩
    - 디코딩 된 위임 리스트에 따라 forwarding hint 를 fetching data interest 로 추가
    - unit test
    - 통합 test 에서 관련 작업
  - [_마지막 의견_]
    - [repo-ng code gerrit](https://gerrit.named-data.net/4807)
    - 다음과 같이 담당자에게 [알람](https://www.lists.cs.ucla.edu/pipermail/nfd-dev/2019-May/003644.html) 을 보냈으나 답이 없음
```
Hi Weijia

I notice that you have uploaded [https://gerrit.named-data.net/4807](https://gerrit.named-data.net/4807) to add
forwarding hint to repo-ng.
I reviewed this Change 11 months ago, but you never responded.
Can you finish it up?

Yours, Junxiao
```

## Bug

|상태|우선순위|제목|날짜|
|:---:|:---:|:---|:---|
|Closed|Urgent|[Build failure with Boost 1.70](https://redmine.named-data.net/issues/4967)|2019/07/09|
|New|Normal|[ndnputfile does not work](https://redmine.named-data.net/issues/4944)|2019/11/11|
|New|Low|[Small typos in Repo Deletion Protocol WiKi Page](https://redmine.named-data.net/issues/4654)|2018/07/07|
|Closed|Normal|[repo-ng fails to compile on Ubuntu](https://redmine.named-data.net/issues/4425)|2017/12/23|
|Closed|Normal|[Validation policy error (Command interest /example/repo/1/insert/...)](https://redmine.named-data.net/issues/4377)|2017/11/02|
|New|High|[ERROR: insert command failed with code 402](https://redmine.named-data.net/issues/4376)|2017/11/02|
|New|Normal|[Unrecognized 'registration-subset' option in 'tcp_bulk_insert' section in configuration file](https://redmine.named-data.net/issues/4375)|2017/11/06|
|New|Normal|[Unrecognized 'registration-subset' option](https://redmine.named-data.net/issues/4356)|2017/10/24|
|Closed|Normal|[skiplist-prev test: does not process Nack](https://redmine.named-data.net/issues/3714)|2017/05/25|
|Closed|Normal|[tools: does not process Nack](https://redmine.named-data.net/issues/3713)|2017/05/25|
|Closed|Normal|[WriteHandle: does not process Nack](https://redmine.named-data.net/issues/3712)|2017/05/25|
|Closed|Normal|[WatchHandle: does not process Nack](https://redmine.named-data.net/issues/3711)|2017/05/25|
|New|Normal|[repo-ng fails to connect to the nfd when the database stores a large number of content segments](https://redmine.named-data.net/issues/2740)|2015/04/10|
|Closed|Normal|[repo-ng cannot set the max-packets to a large number](https://redmine.named-data.net/issues/2736)|2015/07/02|
|New|Normal|[Cannot ndnputfile with default configuration](https://redmine.named-data.net/issues/2321)|2014/12/28|
|Closed|Normal|[ndngetfile doesn't write anything into the file](https://redmine.named-data.net/issues/2296)|2017/06/08|
|Closed|Normal|[Compilation fails with latest ndn-cxx library](https://redmine.named-data.net/issues/2165)|2015/07/02|
|Closed|Normal|[ERROR: Failed to register prefix in local hub's daemon](https://redmine.named-data.net/issues/2023)|2017/06/16|
|Closed|Normal|[Multiple errors with ndnputfile](https://redmine.named-data.net/issues/1896)|2017/06/08|
|Closed|Normal|[RepoNG cannot compile against the latest version of ndn-cxx (commit 46ffa695c6f5a4861436f3b73f8dbad63575056a)](https://redmine.named-data.net/issues/1629)|2014/05/21|
|Closed|Normal|[WriteHandle doesn't send interest for single insert](https://redmine.named-data.net/issues/1574)|2017/06/08|
|Closed|Normal|[WriteHandle has a ~2 second delay before starting sending Interests](https://redmine.named-data.net/issues/1572)|2017/06/08|

- [~~Build failure with Boost 1.70~~](https://redmine.named-data.net/issues/4967)
  - 상태: 완료됨
  - macOS 10.14 / Boost 1.70.0 에서 build fail
  - [_마지막 의견_]
    - [repo-ng code gerrit](https://gerrit.named-data.net/5515)

- [ndnputfile does not work](https://redmine.named-data.net/issues/4944)
  - 상태: 시작
  - `ndnputfile /example/repo/1 /example/data/1/ test.txt` 시 `ERROR: RepoCommandResponse malformed` 발생
  - [_마지막 의견_]
    - RepoCommandResponse 의 writeDecode 에 의해 발생
      - 타입은 '207' 이지만 실제 '101' 타입의 에러이며
      - 이는 RepoCommandResponse-ControlResponse 의 상위 클래스 유형 번호 (클래스 상속으로 인한 문제)
    - `ndnputfile` 명령은 기본적으로 EndBlockID 를 추가하지 않기 때문에 validation 오류 발생으로 write-handle insert 가 실패함
      - https://github.com/named-data/repo-ng/blob/master/src/handles/write-handle.cpp#L48
    - 실패에 대한 메시지 "EndBlockId is required but missing"
      - https://github.com/named-data/repo-ng/blob/master/src/handles/command-base-handle.hpp#L68
    - code flow:
      - https://github.com/named-data/repo-ng/blob/master/src/handles/write-handle.cpp#L127 (sends RepoCommandResponse:: response) -->
      -  https://github.com/named-data/ndn-cxx/blob/master/ndn-cxx/mgmt/dispatcher.cpp#L202 (gets as resp) -->
      - https://github.com/named-data/ndn-cxx/blob/master/ndn-cxx/mgmt/dispatcher.cpp#L218 (and here resp.wireEncode() calls base method)

- [Small typos in Repo Deletion Protocol WiKi Page](https://redmine.named-data.net/issues/4654)
  - 상태: 시작
  - repo-ng [WiKi](https://redmine.named-data.net/projects/repo-ng/wiki/Repo_Deletion_Protocol) 페이지 오타
    - `/ucla/cs/repo/delete/<RepoCommandParameter>/<timestamp>/<random-value>/<SignatureInfo>/<SignatureValue` 에서 마지막에 `>` 추가되어야 함
    - 'Deletion status check' 에서의 예제는 'delete' 에 대한 것으로 'delete check' 관련 내용이 아님

- [~~repo-ng fails to compile on Ubuntu~~](https://redmine.named-data.net/issues/4425)
  - 상태: 완료됨
  - ubuntu17.10 및 14.04 (64bit) 에서 boost::asio::is_service 가 불완전한 형식이기 때문에 컴파일 에러 발생
    - 'tests/integrated/test-basic-interest-read.cpp' 및 'tools/ndnrepowatch.cpp' 에 io_service 헤더 파일을 추가하여 컴파일 오류 수정

- [~~Validation policy error (Command interest /example/repo/1/insert/...~~)](https://redmine.named-data.net/issues/4377)
  - 상태: 완료됨
  - repo-ng 에 파일을 추가하면 에러 발생
    - Validation policy error (Command interest  '/example/repo/1/insert/%C9%22%07%1D%08%07example%08%04data%08%011%08%09%FD%00%00%01_%93%80%DF7%CC%01%00/%16+%1B%01%01%1C%26%07%24%08%09localhost%08%08operator%08%03KEY%08%08%01%BC%C0I%F5%12KZ/%17%FD%01%00%99%B3%A7%81%DFCb%0B%CB%CC%03T%12Q%19%2F%11eo%8C%21%CF%B4E%28%1C%9Dle%D0%1E%F4%97%C0%192J%CB%99%8B%073h%86s%AA%F9%0E%C4%E3%1D%5E%17%BD%3C%7D%EFM%E6%94%3D%BC%96%12%A9%01%81u%23u%29%8A4%FE%7C8%E0%D9%C2j%E0%D8%27%98yF%D1%1B%85%5C%A1%1CQ%A6%99%B6%3B%D0_%BEe_%7D%15%E1%BFw%5E%FFKE2Vb%86%13%FF%EC%B6%98x%8CbYT%88%C8K%7Co%E1%DB%F8%15%C4F%99%A7%0D%19%C55%F1%8A%3DO%9C%DE%E1%8A%1D%9E%E0JA%BE%87iH%BB6%1CG%24%00t%13R%F4%26f%EEa%DEm%13%EB%F6_O%F6%A5%B4%8B%F8%D2%C0%1F%CC%F5m%3E%A4%9C%E5J%D2%90Ww%28%F7%A1%F3%14PC%C4%A8fu9%0D%7D%D9%94%DF%A7%A8s+%F4%8C%9AJ%F5%8C%E0%DF%FFY%0E%D0D-p%B1%EC%FF%E8%8D%85%C6%86%5B%A6%86-vt%22%08%97%9Bq%F9'  doesn't include timestamp component)
  - [_마지막 의견_]
    - [repo-ng code gerrit](https://gerrit.named-data.net/4349)

- [ERROR: insert command failed with code 402](https://redmine.named-data.net/issues/4376)
  - 상태: 시작
  - ubuntu16.04.2 기반에서 ndnputfile 시 `ERROR: insert command failed with code 402` 발생
    - 매우 작은 파일을 사용하는 경우 문제 없음
    - `ndnputfile -s /diarmuidcire1/repo/1 /diarmuidcire1/data/1/ test.txt`
  - [_마지막 의견_] 

- [Unrecognized 'registration-subset' option in 'tcp_bulk_insert' section in configuration file](https://redmine.named-data.net/issues/4375)
  - 상태: 시작
  - ndn-cxx, NFD 최신버전 설치시 오류 발생
    - `ERROR: Unrecognized 'registration-subset' option in 'tcp_bulk_insert' section in configuration file '/usr/local/etc/ndn/repo-ng.conf'`
  - [_마지막 의견_]
    - 위 이슈의 건은 `src/repo.cpp` 의 다음 부분을 수정함
```
...
for (const auto section : dataConf) {

위의 코드를 다음과 같이 수정

... 
for (const auto section : tcpBulkInsert) {
```

- [Unrecognized 'registration-subset' option](https://redmine.named-data.net/issues/4356)
  - 상태: 시작
  - ndn-cxx, NFD 최신버전 설치시 오류 발생
    - [#4375](https://redmine.named-data.net/issues/4375) 에서 발생하는 오류와 같음
    - repo-ng.conf 에서 'tcp_bulk_insert' 섹션에 'registration-subset' 옵션은 보이지 않고 'data' 섹션에  'registration-subset' 옵션이 있음

- [~~skiplist-prev test: does not process Nack~~](https://redmine.named-data.net/issues/3714)
  - 상태: 완료됨
  - 'tests/other/skiplist-prev.hpp' 는 Nack 을 처리하지 않는 Face::expressInterest 를 사용하고 있기 때문에 overhead 발생
  - [_마지막 의견_]
    - repo-ng:commit:42290b2b12b06ccbb028f04367f016f924f213e3 에서 수정

- [~~tools: does not process Nack~~](https://redmine.named-data.net/issues/3713)
  - 상태: 완료됨
  - ndngetfile, ndnputfile, ndnrepowatch 는 Nack 을 처리하지 않는 Face::expressInterest 를 사용하고 있기 때문에 overhead 발생
  -  [_마지막 의견_]
    - repo-ng:commit:42290b2b12b06ccbb028f04367f016f924f213e3 에서 수정

- [~~WriteHandle: does not process Nack~~](https://redmine.named-data.net/issues/3712)
  - 상태: 완료됨
  - repo::WriteHandle 은 Nack 을 처리하지 않는 Face::expressInterest 를 사용하고 있기 때문에 overhead 발생
  -  [_마지막 의견_]
    - repo-ng:commit:42290b2b12b06ccbb028f04367f016f924f213e3 에서 수정

- [~~WatchHandle: does not process Nack~~](https://redmine.named-data.net/issues/3711)
  - 상태: 완료됨
  - repo::WatchHandle 은 Nack 을 처리하지 않는 Face::expressInterest 를 사용하고 있기 때문에 overhead 발생
  -  [_마지막 의견_]
    - repo-ng:commit:42290b2b12b06ccbb028f04367f016f924f213e3 에서 수정

- [repo-ng fails to connect to the nfd when the database stores a large number of content segments](https://redmine.named-data.net/issues/2740)
  - 상태: 시작
  - 많은 contents segments 를 저장할 때 repo-ng 가 NFD 에 연결하지 못하는 오류 발생
    - 1GB 등의 큰 파일 구성
    - ndnputfile 을 사용하여 repo-ng 에 insert (>1000000 개 segments)
    - 연결이 실패하여 repo-ng 를 다시 실행
    - "ERROR: error while connecting to the forwarder" 발생
  - [_마지막 의견_]
    - [#2742](https://redmine.named-data.net/issues/2742) 와 같은 버그

- [~~repo-ng cannot set the max-packets to a large number~~](https://redmine.named-data.net/issues/2736)
  - 상태: 완료됨
  - repo-ng 의 max-packets 조절 (large num 으로)
    - max-packets 를 9999999999 로 설정
    - 'sudo ndn-repo-ng' 실행
    - "ERROR: conversion of data to type "i" failed".
It is suspect this is caused by "src/repo.cpp: repoConfig.nMaxPackets = repoConf.get("storage.max-packets");" 발생
  - [_마지막 의견_]
    - ubuntu 32bit 에서의 ndn-cxx 구성 문제로 판단
    - wscript 의 PKG_CONFIG_PATH 에 '/usr/local/lib32/pkgconfig' 를 추가하지 않으면 repo-ng 가 ndn-cxx 를 찾을 수 없음

- [Cannot ndnputfile with default configuration](https://redmine.named-data.net/issues/2321)
  - 상태: 시작
  - 기본 제공되는 configuration 으로 repo-ng 설치 및 build 완료 후 `ndnputfile -u /example/repo/1 /example/data/1/%FD%00%00%01G%F0%C8%AD- test.txt` 명령어에 대한 에러 발생
    - 'ERROR: command response timeout'
  - [_마지막 의견_]
    - sample 'repo.conf' 파일을 사용하는 경우 storage 경로는 '/var/db/ndn-repo-ng' 로 지정되어 있으며 이 경로가 local 환경에 맞게 수정되어야 함
    - 'sudo ndn-repo-ng' 명령어를 사용하여야 함 (언제 명령어를 사용하는지 알 수 없음)

- [~~ndngetfile doesn't write anything into the file~~](https://redmine.named-data.net/issues/2296)
  - 상태: 완료됨
  - `ndnputfile /tcd/repo/1  /tcd/data/1/ /Users/diarmuidcire/Documents/VIDEO0037.mp4` 이후 `ndngetfile -o media.mp4 /tcd/data/1` 하면 empty 데이터로 보임
  - [_마지막 의견_]
    - 'ndngetfile.cpp' 의 버그로 생각됨
    - [repo-ng code gerrit](https://gerrit.named-data.net/1546)

- [~~Compilation fails with latest ndn-cxx library~~](https://redmine.named-data.net/issues/2165)
  - 상태: 완료됨
  - 'repo-ng' 인스톨 시 success 상태로 확인되었으나 실제 실패인 경우 발생
    - 'ndn-cxx' 라이브러리 컴파일 실패
  - [_마지막 의견_]

- [~~ERROR: Failed to register prefix in local hub's daemon~~](https://redmine.named-data.net/issues/2023)
  - 상태: 완료됨
  - local hub 의 데몬에서 prefix 등록 실패
    - OpenWRT 에서 ndn-repo 실행시 오류 발생
    - 'ERROR: Failed to register prefix in local hub's daemon'
  - [_마지막 의견_]
    - NFD-RIB 에 4 개의 개별 경로를 등록하면 ndn::nfd::Controller 에 stop-and-wait ([#2293](https://redmine.named-data.net/issues/2293)) 가 없어 일부 명령이 거부될 수 있음
    - 또는 OpenWRT 에서 계산 능력이 부족함
    - [repo-ng code gerrit](https://gerrit.named-data.net/3940)
    - 과도한 FIB 엔트리 문제 발생으로 인한 수정
    - dispatcher 로 전환하여야 함

- [~~Multiple errors with ndnputfile~~](https://redmine.named-data.net/issues/1896)
  - 상태: 완료됨
  - [_마지막 의견_]
    - [#1887](https://redmine.named-data.net/issues/1887) 와 동일

- [~~RepoNG cannot compile against the latest version of ndn-cxx (commit 46ffa695c6f5a4861436f3b73f8dbad63575056a)~~](https://redmine.named-data.net/issues/1629)
  - 상태: 완료됨
  - [_마지막 의견_]

- [~~WriteHandle doesn't send interest for single insert~~](https://redmine.named-data.net/issues/1574)
  - 상태: 완료됨
  - '-s' 옵션을 사용하여 ndnputfile 실행시 interest 가 발생하지 않고 404 에러 발생
    - 'ndnputfile -s' 는 데이터를 insert 하는데 사용
    - ndnputfile 은 /example/data/1/testfile 의 setInterestFilter 설정
    - repo 는 /example/data/1/testfile 에 대한 interest 발생
    - nfd 가 먼저 /example/data/1/testfile 의 interest 를 받은 다음 /example/data/1/testfile 의 다음 hop 추가
    - interest delay 가 발생하면 정상동작
  - [_마지막 의견_]
    - 오랜시간 해결되지 않아서 질문자가 해결함 (해결 방법 및 내용은 없음)

- [~~WriteHandle has a ~2 second delay before starting sending Interests~~](https://redmine.named-data.net/issues/1572)
  - 상태: 완료됨
  - interest 를 발생후 delay 발생
  - [_마지막 의견_]

## Task

|상태|우선순위|제목|날짜|
|:---:|:---:|:---|:---|
|New|Normal|[Add Sync support for repo](https://redmine.named-data.net/issues/4523)|2018/11/15|
|Closed|Normal|[Remove selectors from old interest format](https://redmine.named-data.net/issues/4522)|2019/01/13|
|New|High|[Can't insert segmented data](https://redmine.named-data.net/issues/4374)|2017/11/01|
|Closed|Urgent|[Adapt to ndn-cxx v2::KeyChain and Validator](https://redmine.named-data.net/issues/4091)|2018/10/27|
|New|High|[Downloading with a different piplines](https://redmine.named-data.net/issues/3320)|2015/11/05|
|New|Normal|[ndnputfile ERROR: check response timeout](https://redmine.named-data.net/issues/3304)|2015/11/03|
|Closed|Normal|[Fix test cases update for ImplictSha256DigestComponent](https://redmine.named-data.net/issues/2172)|2017/06/15|
|Closed|Normal|[Expand basic documentation and installation instructions for repo-ng](https://redmine.named-data.net/issues/1897)|2017/06/15|
|Closed|High|[The problem of ndnputfile and ndngetfile](https://redmine.named-data.net/issues/1887)|2017/06/08|
|Closed|High|[Initiation read entries from database](https://redmine.named-data.net/issues/1867)|2017/06/08|
|New|Normal|[Client library to handle repo-ng](https://redmine.named-data.net/issues/1858)|2014/08/29|
|Closed|Normal|[Rebuild storage when repo starts](https://redmine.named-data.net/issues/1831)|2014/08/17|
|New|Normal|[Example producer to help testing / guide development application for watch protocol](https://redmine.named-data.net/issues/1829)|2014/08/06|
|Closed|Normal|[ERROR: command response timeout from "ndnputfile"](https://redmine.named-data.net/issues/1810)|2017/06/08|
|New|Normal|[RepoSync](https://redmine.named-data.net/issues/1809)|2016/07/19|
|Closed|Normal|[use make_shared to initiate data](https://redmine.named-data.net/issues/1802)|2017/06/08|
|In Progress|Normal|[repo performance test tools: repong-put-packet repong-get-packet repong-remove-packet](https://redmine.named-data.net/issues/1801)|2015/03/03|
|Closed|Normal|[Visualize contents of the repo](https://redmine.named-data.net/issues/1789)|2017/06/08|
|New|Normal|[create wiki about access control](https://redmine.named-data.net/issues/1783)|2014/07/24|
|New|Normal|[ndndumpfile: local dump tool](https://redmine.named-data.net/issues/1779)|2014/07/18|
|Closed|Normal|[skiplist: Changing to const_iterators](https://redmine.named-data.net/issues/1745)|2017/06/08|
|Closed|Low|[Use test-specific identity in test-basic-command-insert-delete.cpp](https://redmine.named-data.net/issues/1739)|2017/07/18|
|Closed|High|[implement skiplist for index of repo](https://redmine.named-data.net/issues/1737)|2014/07/23|
|In Progress|Normal|[Repo-ng slowdown for large repo sizes](https://redmine.named-data.net/issues/1695)|2014/08/11|
|New|Normal|[WriteHandle doesn't stop expressing Interests when FinalBlockId is present in the last Data packet](https://redmine.named-data.net/issues/1573)|2014/06/19|
|Closed|Normal|[WriteHandle skips 14th segment (%0C)](https://redmine.named-data.net/issues/1571)|2014/05/02|
|Closed|Normal|[Copying KeyChain crashes repo](https://redmine.named-data.net/issues/1552)|2014/04/29|
|Closed|Normal|[License change to GPL 3.0](https://redmine.named-data.net/issues/1540)|2018/10/27|
|Closed|Urgent|[Build with ndn-cxx instead of ndn-cpp-dev](https://redmine.named-data.net/issues/1538)|2014/04/25|
|Closed|High|[ndngetfile: retrieve tool](https://redmine.named-data.net/issues/1514)|2014/05/01|
|Closed|High|[ndnputfile: insert tool](https://redmine.named-data.net/issues/1513)|2014/07/23|
|New|Normal|[Command-line tool to print out names of stored data](https://redmine.named-data.net/issues/1498)|2014/04/28|
|Closed|Normal|[Make use ValidatorConfig](https://redmine.named-data.net/issues/1479)|2014/07/23|
|New|Normal|[Deletion progress check](https://redmine.named-data.net/issues/1478)|2014/04/28|
|New|Normal|[Deletion of segmented data without EndBlockId](https://redmine.named-data.net/issues/1477)|2014/04/14|
|Closed|Normal|[Select data by PublisherPublicKeyLocator](https://redmine.named-data.net/issues/1437)|2014/06/19|
|Closed|Normal|[Repo table reorganized or in-memory index](https://redmine.named-data.net/issues/1434)|2014/07/23|
|Closed|Normal|[Definition of rightmost selector](https://redmine.named-data.net/issues/1433)|2014/06/19|
|Closed|Normal|[Define command syntax for insertion/deletion to repo](https://redmine.named-data.net/issues/1258)|2014/06/19|

- [Add Sync support for repo](https://redmine.named-data.net/issues/4523)
  - 상태: 시작
  - ChronoSync 를 사용하여 실시간 데이터를 동기화 (기존 watch prefix insert 프로토콜을 대체)
  - [_마지막 의견_]
    - PSync 사용에 대한 개인 의견만 있음

- [Remove selectors from old interest format](https://redmine.named-data.net/issues/4522)
  - 상태: 완료됨
  - selector 및 관련 테스트 코드를 삭제하고 interest 데이터 matching 프로세스 단순화
    - 새로운 버전의 NDN interest 는 selector 기능을 단순화
  - [_마지막 의견_]
    - name column 에는 NAME-VALUE 만 저장
    - [https://github.com/3rd-ndn-hackathon/repo-sql/blob/91eb215e90dd66e0711f3a8d705ef4b788c7a4d7/src/database-conn.cpp#L90](https://github.com/3rd-ndn-hackathon/repo-sql/blob/91eb215e90dd66e0711f3a8d705ef4b788c7a4d7/src/database-conn.cpp#L90)  
    - [https://github.com/3rd-ndn-hackathon/repo-sql/blob/91eb215e90dd66e0711f3a8d705ef4b788c7a4d7/src/database-conn.cpp#L21](https://github.com/3rd-ndn-hackathon/repo-sql/blob/91eb215e90dd66e0711f3a8d705ef4b788c7a4d7/src/database-conn.cpp#L21)

- [Can't insert segmented data](https://redmine.named-data.net/issues/4374)
  - 상태: 시작
  - repo-ng 기본 구성으로 세그먼트 데이터 삽입이 되지 않음
    - '$ ndnputfile /icear/content-provider/1 /icear/content-provider/data/chair chair.jpg'
    - ERROR: insert command failed with code 402
  - [_마지막 의견_]

- [Adapt to ndn-cxx v2::KeyChain and Validator](https://redmine.named-data.net/issues/4091)
  - 상태: 종료됨
  - ndn-cxx v2::KeyChain 및 v2::Validator 를 사용하도록 repo-ng 수정
  - [_마지막 의견_]
    - [repo-ng code gerrit](https://gerrit.named-data.net/#/c/4122/)

- [Downloading with a different piplines](https://redmine.named-data.net/issues/3320)
  - 상태: 시작
  - ndngetfile tool 에 다른 파이프라인으로 다운로드하는 기능이 없음
    - NDN 다운로드 속도가 느려질 수 있음
  - [_마지막 의견_]

- [ndnputfile ERROR: check response timeout](https://redmine.named-data.net/issues/3304)
  - 상태: 시작
  - repo-ng 에 8.5G 파일 insert 시 타임아웃 발생
    - 3.0, 3.5, 4.3, 2.7 GB 파일 insert 동일 오류 발생
    - 구 버전을 사용하는 경우 2.2 GB 저장 가능
    - NFD 버전 0.4.0-rc1
  - [_마지막 의견_]
    - ndnputfile 및 ndn-repo-ng 프로세스가 모든 RAM 을 소비하고 프로세스를 삭제함

- [Fix test cases update for ImplictSha256DigestComponent](https://redmine.named-data.net/issues/2172)
  - 상태: 완료됨
  - ImplictSha256DigestComponent 에 대한 수정 테스트 케이스 업데이트
  - [_마지막 의견_]
    - _ContentStore matching test cases: update for ImplictSha256DigestComponent_ 추가

- [Expand basic documentation and installation instructions for repo-ng](https://redmine.named-data.net/issues/1897)
- 상태: 완료됨
- README, INSTALL 문서가 완전하지 않음
  - 샘플 기반의 config 파일의 필요성 및 위치 설명
  - /var/db/ndn-repo-ng 디렉토리를 수동으로 구성하고 repo-ng 를 실행할 사용자에 대한 rw 권한 설정
  - ndnputfile 및 ndngetfile을 사용한 repo 작업 테스트 과정
  - /var/db 에 단일 db 가 있는 시스템 서비스인 것 처럼 설치하지만 ~/.ndn/client.conf 의 사용자 별 구성을 고려함
  - NFD 와 같이 별도의 사용자로 실행될 수 있도록 하여야 함
  - 문서화 필요
- [_마지막 의견_]
  - [repo-ng code gerrit](https://gerrit.named-data.net/3939)

- [The problem of ndnputfile and ndngetfile](https://redmine.named-data.net/issues/1887)
  - 상태: 완료됨
  - ndnputfile -s 옵션과 함께 insert 된 데이터를 ndngetfile 할 수 없음
    - ndnputfile -s 를 사용하면 prefix 뒤에 버전 번호를 자동으로 임의 추가
    - ndngetfile 을 사용하는 경우 prefix 만 사용하거나 -u 옵션을 이용하여 버전 번호가 있는 prefix 를 사용하도록 결정할 수 있으나 모두 동작하지 않음
  - [_마지막 의견_]
    - [repo-ng code gerrit](https://gerrit.named-data.net/1163)

- [Initiation read entries from database](https://redmine.named-data.net/issues/1867)
  - 상태: 종료됨
  - 'nfd-start' 와 'ndn-repo-ng' 를 실행후 오류 발생
    - "ERROR: Initiation Read Entries from Database Prepare error"
  - [_마지막 의견_]
    - [#1887](https://redmine.named-data.net/issues/1887) 에서 동일 이슈를 다룸

- [Client library to handle repo-ng](https://redmine.named-data.net/issues/1858)
  - 상태: 시작
  - 클라이언트 라이브러리는 RepoCommandParameter 및 interest 를 보다 쉽게 생성하여야 함
  - [_마지막 의견_]
    - extractRepoCommandInterest (Data & repoCommandResponse, RepoCommandParameter & parameter) 함수가 명확한지 확실하지 않음
    - extractRepoCommandResponse(Data& reply, RepoCommandResponse& response) 과 extractRepoCommandParameter(Interest interest, Name prefix, RepocommandParameter parameter) 로 분리하여야 함
    - 의미가 없는 것으로 판단됨

- [Rebuild storage when repo starts](https://redmine.named-data.net/issues/1831)
  - 상태: 종료됨
  - storage rebuild 관련 storage api 가 있으나 초기화 과정에 누락됨
  - [_마지막 의견_]
    - rebuilding storage 의미가 불분명함
    - repository 시작 후 index 를 다시 작성하는 것으로 이 기능은 storage API 에서 제공하고 있으나 초기화시에 호출되지 않음
    - 'rebuilding Index' 로 확인함

- [Example producer to help testing / guide development application for watch protocol](https://redmine.named-data.net/issues/1829)
  - 상태: 시작
  - watch 프로토콜에 대한 어플리케이션의 테스트/가이드 예제
  - [_마지막 의견_]

- [ERROR: command response timeout from "ndnputfile"](https://redmine.named-data.net/issues/1810)
  - 상태: 종료됨
  - ndnputfile 명령어를 사용하는 경우 타임아웃 발생
    - face 를 만들기 위한 시도를 하였으나 즉시 제거됨 
  - [_마지막 의견_]
    - [#1887](https://redmine.named-data.net/issues/1887) 에서 동일 이슈를 다룸

- [RepoSync](https://redmine.named-data.net/issues/1809)
  - 상태: 시작
  - repo 간 동기화 필요 (ChronoSync 기반)
  - [_마지막 의견_]
    - 최근 summit 에서 의견이 있었음
    - repo 가 설정되면 repo 동기화가 시작되며 이는 sync handle 명령을 통해 수행
    - 2016년 6월 30일 회의를 통해
    - 동기화 상태 및 동기화 트리를 기록하는 DB 추가 (그렇지 않으면 repo 를 재실행하는 순간 초기화됨)
    - 기존 RepoSync 는 NDN 에서 'Action-Based' 및 'Data-Based' 동기화 모델을 검토하여야 함
    - 코드 구현 품질이 매우 좋지 않기 때문에 재구성 필요 

- [use make_shared to initiate data](https://redmine.named-data.net/issues/1802)
  - 상태: 종료됨
  - ndn-cxx 새 버전에서 make_shared 에서 데이터를 시작하지 않으면 데이터 put 에대한 경고 발생
  - [_마지막 의견_]
    - watch prefix 커밋에서 수정됨
    - shared_ptr 을 사용하여 데이터를 넣지 않는 handle 의 응답 대체

- [repo performance test tools: repong-put-packet repong-get-packet repong-remove-packet](https://redmine.named-data.net/issues/1801)
  - 상태: 시작
  - repo 성능 테스트 도구 필요
    - ndnputpacket, ndngetpacket, ndnremovepacket
  - [_마지막 의견_]
    - ndn 의 성능 도구가 아닌 repo 의 성능 도구

- [Visualize contents of the repo](https://redmine.named-data.net/issues/1789)
  - 상태: 종료됨
  - repo 내용을 시각화 하는 도구의 필요성
  - [_마지막 의견_]
    - index.cpp 및 repostorage.cpp에서 구현되며 repo는 dataEnumerate 함수를 사용하여 repostorage 외부에 정의 된 콜백 함수를 호출

- [create wiki about access control](https://redmine.named-data.net/issues/1783)
  - 상태: 시작
  - repo-ng access control 에 대한 WiKi 필요성
  - [_마지막 의견_]
    - [validator configuration WiKi](http://redmine.named-data.net/projects/ndn-cxx/wiki/CommandValidatorConf) 참조
    - [Access Control in NDN Repo](https://redmine.named-data.net/attachments/104) 참조

- [ndndumpfile: local dump tool](https://redmine.named-data.net/issues/1779)
  - 상태: 시작
  - tcp-bulk-insert-handle 을 사용하여 파일을 로컬 저장소에 덤프
  - [_마지막 의견_]

- [skiplist: Changing to const_iterators](https://redmine.named-data.net/issues/1745)
  - 상태: 종료됨
  - [기사](http://stackoverflow.com/questions/765257/should-i-prefer-iterators-over-const-iterators) 에서 caller 는 iterator 가 참조하는 값을 수정할 수 있다고 지적함
    - 모든 iterator 를 const_iterator 로 변경하여 변화되지 않도록 하는 것이 필요함
  - [_마지막 의견_]
    - index 는 'std::set' 을 기반으로 하기 때문에 더이상 필요하지 않음

- [Use test-specific identity in test-basic-command-insert-delete.cpp](https://redmine.named-data.net/issues/1739)
  - 상태: 종료됨
  - 현재 테스트 예제는 'default' ID 를 사용함 (dumps 및 서명)
    - 테스트 케이스가 특정 ID 를 생성하고 호스트 시스템의 기본 설정을 변경하지 않고 명시적으로 서명하여 데이터 패킷을 작성
  - [_마지막 의견_]
    - repo-ng : commit : 047a6fb40b806199d24a2ef4d0cbbca78e1dbf78 에서 해결

- [implement skiplist for index of repo](https://redmine.named-data.net/issues/1737)
  - 상태: 종료됨
  - repo index 에 대한 skiplist 구현
  - [_마지막 의견_]

- [Repo-ng slowdown for large repo sizes](https://redmine.named-data.net/issues/1695)
  - 상태: In Progress
  - BAMS/BMS 데모 응용의 경우 1GB 이상의 데이터를 repo-ng 에서 사용할 수 없음
    - 이 응용은 약 20 개의 데이터 포인트에 대한 1Hz 샘플링이 포함
    - 각 데이터 포인트는 현재 데이터 객체로 저장
    - 작은 객체의 큰 데이터 세트에 대한 성능 향상 
  - [_마지막 의견_]
    - index 및 storage 개발중이며 성능은 말하기 어려움
    - index 및 sqlite 를 포함한 repoStorage 구현 완료
    - 동작 test 되지 않음

- [WriteHandle doesn't stop expressing Interests when FinalBlockId is present in the last Data packet](https://redmine.named-data.net/issues/1573)
  - 상태: 시작
  - ndnputfile 에서 'ERROR: insertion check command invalidated: RepoCommandResponse( StatusCode: 404 )' 발생
    - FinalBlockID 를 지정하지만 WriteHandle 은 계속진행되어 존재하지 않는 Data 패킷에 대한 interest 를 계속 발생
  - [_마지막 의견_]
    - ndnputfile 추후 commit 에서 수정될 예정

- [WriteHandle skips 14th segment (%0C)](https://redmine.named-data.net/issues/1571)
  - 상태: 종료됨
  - ndnputfile (http://gerrit.named-data.net/#/c/775) 에서 문제 발견
    - repo-ng 에서 13 번 segment 를 요청하지 않음
  - [_마지막 의견_]

- [Copying KeyChain crashes repo](https://redmine.named-data.net/issues/1552)
  - 상태: 종료됨
  - KeyChain 은 복사되지 않아야 함
    - lib 최근 변경으로 인하여 KeyChain 을 복사하면 응용 중단
    - 모든 KeyChain 사본은 reference 로 교체
  - [_마지막 의견_]
    - read--handle KeyChain 을 제외한 모든 곳은 reference 로 교체
    - commit: repo-ng|a53ee59a36d26d7f78de7faa69fba849f1909127 적용

- [License change to GPL 3.0](https://redmine.named-data.net/issues/1540)
  - 상태: 종료됨
  - 라이센스를 GPL3.0 으로 전환
  - [_마지막 의견_]

- [Build with ndn-cxx instead of ndn-cpp-dev](https://redmine.named-data.net/issues/1538)
  - 상태: 종료됨
  - ndn-cxx 적용
    - ndn-cxx 를 include 할 수 있도록 함
    - ndn-cxx 사용을 위한 wscript 업데이트
  - [_마지막 의견_]

- [ndngetfile: retrieve tool](https://redmine.named-data.net/issues/1514)
  - 상태: 종료됨
  - ndnputfile '-u': unversioned; ndn-name 에 버전 구성이 포함
    - '-u' 옵션이 없는 경우 interest 는 최신 버전을 찾아야 하며 'MustBeFresh = yes', 'ChildSelector=rightmost' 가 있어야 함
    - 최신 버전을 찾기위한 Exclude 를 사용하지 않음
    - ndn-tlv-peek 을 통해 최신버전을 찾을 수 있으며 repo 에서 버전이 지정되지 않은 세그먼트 및 세그먼트화 되지 않은 데이터 검색을 지원하지 않아도 됨
  - [_마지막 의견_]

- [ndnputfile: insert tool](https://redmine.named-data.net/issues/1513)
  - 상태: 종료됨
  - insert 프로세스를 유지하기 위한 'insert check' 명령 사용
  - [_마지막 의견_]

- [Command-line tool to print out names of stored data](https://redmine.named-data.net/issues/1498)
  - 상태: 시작
  - 이전 repo 에서 ndnnamelist 명령을 사용하여 repo 에 저장된 데이터 패킷의 name 을 나열하였으며 이와 비슷한 기능이 'ndn-repo-ng' 에서도 필요함
  - [_마지막 의견_]
    - ndnnamelist 명령은 repo-ng 에 정의되지 않은 Name Enumeration Protocol 을 사용함
    - ndnnamelist 명령은 프로토콜에 의존하지 않고 단순히 지정된 파일을 열고 내용을 확인하고 각 패킷을 분석하고 name 을 보여줌
    - sqlite3 에 저장된 모든 데이터 패킷의 name 을 보여주는 기능이 필요함

- [Make use ValidatorConfig](https://redmine.named-data.net/issues/1479)
  - 상태: 종료됨
  - 현재 CommandInterestValidator 는 interest 를 검증하는 기능만 제공
    - ValidatorConfig 는 간단한 config file 을 사용하여 원하는 신뢰 모델을 설정할 수 있는 방법 제공
    - 이 구성파일은 repo 구성의 일부가 되어야 함 (NRD 처럼)
  - [_마지막 의견_]
    - validator-config 와 command-interest-validator 를 결합한 Validator
    - 더이상 이러한 모델은 필요하지 않으며 '표준' 방식으로 두 작업을 모두 수행하는 ValidatorConf 만 사용하여야 함
    - interest 검증 뿐만 아니라 데이터의 유효성 검사도 필요함

- [Deletion progress check](https://redmine.named-data.net/issues/1478)
  - 상태: 시작
  - 세그먼트화 된 데이터 삭제의 while loop 으로 인해 삭제 진행 상태의 점검이 지원되지 않음
  - [_마지막 의견_]
    - interest 처리시 삭제 프로세스가 완료되었기 때문에 삭제 프로세스 검사에서 '완료' 를 리턴하지 않을 수 있음 (검사 전에 삭제 완료되는 경우)
    - 많은 양의 데이터를 삭제하는 경우에 상태 검사를 할 수 없음

- [Deletion of segmented data without EndBlockId](https://redmine.named-data.net/issues/1477)
  - 상태: 시작
  - EndBlockId 가 없는 세그먼트화 된 데이터를 삭제할 수 없음
    - ERROR: 403 발생
  - [_마지막 의견_]

- [Select data by PublisherPublicKeyLocator](https://redmine.named-data.net/issues/1437)
  - 상태: 종료됨
  - PublisherPublicKeyLocator selector 를 활용한 data select 과정 구현 필요
  - [_마지막 의견_]

- [Repo table reorganized or in-memory index](https://redmine.named-data.net/issues/1434)
  - 상태: 종료됨
  - repo 테이블 구조가 완전하지 않고 모든 repo 프로토콜을 지원하지 못함
    - 속도를 높이기 위한 in-memory-index 필요
  - [_마지막 의견_]
    - 누구나 index 의 특정 요구 사항을 나열할 수 있는가?
    - index 과정은 빠르게 작동하여야 하고 최소한의 메모리를 사용하여야 함
    - skiplist 를 기반으로 디자인 되어야 함

- [Definition of rightmost selector](https://redmine.named-data.net/issues/1433)
  - 상태: 종료됨
  - sqlite-handle.cpp 의 현재 함수 filterNameChild() 에서 가장 오른쪽의 child 는 name 벡터의 최대 값을 의미하지만 올바르지 않음
    - 가장 오른쪽 child 정의가 명확하지 않음
  - [_마지막 의견_]

- [Define command syntax for insertion/deletion to repo](https://redmine.named-data.net/issues/1258)
  - 상태: 종료됨
  - 모든 명령과 응답은 ControlCommand 사양을 따라야 함
    - insert 명령을 사용하면 세그먼트화 되지 않은 여러 세그먼트 항목을 insert 할 수 있음
    - delete 명령은 정확한 name 으로 데이터 객체를 삭제하고 단일 명령을 사용하여 여러 데이터를 삭제할 수 있어야 함
  - [_마지막 의견_]

# 기능개선: hashchain 고도화

## hashchain overview

- producer: ndnputchunk (ndnputfile) 실행
  - block 구성
- DIFS: producer 로 _Interest_ 를 보내고 _data_ 를 받음
  - _data_ 는 DIFS 의 directory/mongoDB 로 저장시 validation 과정을 거쳐야 함

```
+----------+                  +------+
| producer |                  | DIFS |
+----------+                  +------+
     |---------ndnputfile-------->|
 +---|                            |
 |   |                            |
block|                            |
 |   |                            |
 +---|                            |
     |<--------  200 OK  ---------|
     |<--------interest01---------|
     |---------segment01--------->|
     |                            |---+
     |                            |   |
     |                            | hash-fetcher (afterValidationSuccess)
     |                            |   |
     |                            |---+
     |<--------interest02---------|
     |---------segment02--------->|
```

### hash-chain 블록 구성

- sha256 을 사용하기 때문에 32 byte 가 되어야 함

```
# stream data (segment, block)
+----------------------+         +--------------+         +--------------+
|       block01        |         |    block02   |         |    block03   |
+----------------------+         +--------------+         +--------------+

# make hash
# create hash (each block hash)
+--------------+         +--------------+         +----------------------+
|    block03   |         |    block02   |         |       block01        |
|+------------+|         |+------------+|         |+--------------------+|
||data        ||         ||data        ||         ||data                ||
|+------------+|         |+------------+|         |+--------------------+|
||000000000000||   +------> hash value ||   +------> hash value         ||
|+------------+|   |     |+------------+|   |     |+--------------------+|
+--------------+   |     +--------------+   |     +----------------------+
       |           |             |          |
       V           |             V          |
  block03 hash ----+       block02 hash ----+

# make segment (Interest response)
# data + next hash + signature
+----------------------+         +--------------+         +--------------+
|       block01        |         |    block02   |         |    block03   |
|+--------------------+|         |+------------+|         |+------------+|
||data                ||         ||data        ||         ||data        ||
|+--------------------+|         |+------------+|         |+------------+|
||block02 hash        ||         ||block03 hash||         ||000000000000||
|+--------------------+|         |+------------+|         |+------------+|
+----------------------+         +--------------+         +--------------+
|SignatureSha256WithRsa|         |DigestSHA256  |         |DigestSHA256  |
+----------------------+         +--------------+         +--------------+

# validation (segment-fetcher)
+----------------------+         +--------------+         +--------------+
|       block01        |         |    block02   |         |    block03   |
|+--------------------+|         |+------------+|         |+------------+|
||data                ||         ||data        ||         ||data        ||
|+--------------------+|         |+------------+|         |+------------+|
||DigestSHA256        || ---+    ||DigestSHA256|| ---+    ||000000000000||
|+--------------------+|    |    |+------------+|    |    |+------------+|
+----------------------+    |    +--------------+    |    +--------------+
|SignatureSha256WithRsa|    +--> |DigestSHA256  |    +--> |DigestSHA256  |
+----------------------+         +--------------+         +--------------+
```

---

## ndnputfile

- `ndnputchunk` 내용은 추후 `ndnputfile` 로 취합되어야 함
  - block 을 만드는 부분에서
    - reverse_end (데이터 스트림의 첫부분) 는 `SignatureSha256WithRsa` 로 함
    - 그 외 `DigestSHA256` 으로 함
- 이 내용은 바로 진행
  - difs/hash-chain 의 ndnputfile 에 일부 구현되어 있기 때문에 비교적 적용이 쉬울 것으로 판단
  - 최대한 빠르게 ndnputchunk 에 해당 기능을 적용하여 hashchain 관련 테스트 진행
    - ndnputchunk 에 우선 적용하는 이유는 difs 의 ndnputfile 에 적용시 인스톨, 설정 등의 시간이 걸리기 때문
  - 테스트 후 difs 의 ndnputfile 에 hashchain 관련 전체 기능 적용

## hash-key-chain

- `key-chain` 을 상속받는 `hash-key-chain` 를 새로 만들어야 함
  - `sign` 에 next-hash 가 들어가고 current-hash 를 block type 으로 return 하여야 함
    - 현재, void 로 선언되어 있음
- 이 내용은 바로 진행

## hash-fetcher

- `segmentfetcher` 를 상속받는 `hash-fetcher` 를 새로 만들어야 함
  - first segment 처리 부분 확인 및 추가 (next-hash 가 empty 인 경우 next-hash 가 current-hash 로 됨)
  - afterValidationSuccess 에서 current-hash 와 next-hash 의 매치가 되지 않으면 exception 처리
  - DIFS 에서 segmentfetcher 를 사용중
    - [handlers/write-handler](https://github.com/uni2u/difs/blob/hash-chain/src/handles/write-handle.cpp)
    - 해당 부분이 `hash-fetcher` 를 사용할 수 있도록 write-handler 수정
- 이 내용은 바로 진행
  - difs 의 디렉토리 및 mongoDB 에 저장하기 전 검증시 사용
  - 추후, difs 에 포함되어야 함

## new signature type

- 새로운 signature type 인 (SignatureWithHashchain) 정의가 필요함
- 이 내용은 hashchain 기본 동작 확인이 끝난 후 진행

## security validation config

- difs 의 repo-ng.conf 의 validator 는 type any 로 함
  - hash chain 의 validator 는 segmentfetcher 를 사용할 수 있도록 하기 위함
  - type any 는 validator 를 사용하지 않고 by pass (The following rule disables all security in the repo)
- 이 내용은 hashchain 기본 동작 확인이 끝난 후 진행

```
  trust-anchor
    {
      type any
    }
```

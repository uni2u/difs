# 기능개선: KeyValue Store 고도화

## KeyValue Store overview

- producer 가 생산하는 데이터를 네트워크에서 저장하기 위함
- 각 DIFS 노드는 클러스터를 구성하고 있으며
- 각 노드는 자신의 name 과 함께 DIFS naming 을 가짐
- 이것은 어떠한 네트워크에 구성되어도 공통 naming 을 통해 DIFS 를 사용할 수 있게하기 위함
- producer 로 부터 공통 naming 을 통해 저장 요청을 받은 노드는
- producer 로 _Interest_ 를 보내어 _Data_ 를 받아 노드에 저장하는데
- 저장 노드는 config 를 통하여 디렉토리 형식으로 저장하거나 mongoDB 를 사용하여 저장할 수 있음
- consumer 는 저장된 데이터를 사용하기 위해서 실제 데이터를 저장한 노드의 name 을 알아야 하는데
- 복잡한 네트워크 naming 이 적용된 노드의 name 을 알지 못한다
- 이를 편하게 하고자 공통의 naming 을 사용하여 해당 데이터를 실제 저장하고 있는 노드의 full prefix 를 제공할 수 있도록
- manifest file 을 활용한다.
- manifest file 은 해당 {data_name} 에 대하여 이 데이터를 실제 저장한 노드의 {node_name} 리스트를 제공하는 파일이다.
- producer 가 DIFS 에 저장을 요청하면서 지정한 {data_name} 은 hash 계산을 통하여 일정한 값을 가진다.
- DIFS 클러스터는 각 노드가 담당할 hash range 가 구성되어 있기 때문에 위의 계산 결과를 활용하여 일정 노드를 선택할 수 있다.
- 즉, consumer 도 같은 {data_name} 으로 요청하면 이 {data_name} 을 hash 한 결과가 같기 때문에 동일한 노드를 선택할 수 있다.
- consumer 는 요청하는 {data_name} 에 대한 manifest file 을 제공받고 해당 file 을 확인하여
- 실제 데이터를 저장하고 있는 노드의 full name prefix 로 데이터를 요청한다.  

---

### github branch 정리

- github 의 master (또는 main) 활용
  - 각 브렌치에 commit 이 발생하면
  - commit 내용을 확인하고 확인한 사람이 확인 의견을 포함하여 merge request
  - commit 내용과 merge request 내용을 확인하고 master 에 merge
    - difs-cxx 의 경우 적용됨
- branch 정리
  - 우선 삭제 브렌치 리스트 업
  - 남겨진 브렌치 중 master 로 삼을 브렌치 선정
    - 현재 hash-chain 브렌치를 master 후보로 생각중
    - nosql (mongodb) 구성이 무엇을 기반으로 작성되었는지 확인 필요
  - 결정된 브렌치를 master 로 구성
    - 각 작업은 해당 브렌치에 구현
    - commit 에 대한 검토 후 merge request
    - 검토 의견 확인 후 merge

---

### ndn-cxx 0.7.1 적용

- 현재 ndn-cxx 0.7.0 기반
  - 특히, security 관련하여 미구현된 내용이 많음
- 위와 관련한 내용이 ndn-cxx 0.7.1 버전에는 상당 해소된 것으로 파악됨
  - ndn-cxx 0.7.1 적용에 있어 큰 문제는 발생하지 않는 것으로 분석함
    - 다른 파트에서 DIFS 를 사용하는데 ndn-cxx 0.7.1 기반으로 동작중임

---

### nosql 적용

- DIFS 저장 노드의 다양성을 지원하기 위함
- nosql 은 **mongoDB** 를 대상으로 함
- DIFS 의 저장 메커니즘이 모두 mongoDB 를 사용하는 것이 아님
  - .conf 를 활용하여 **기존 dir** 구성과 **mongodb** 구성을 선택할 수 있도록 함
  - [ndn-python-repo](https://github.com/UCLA-IRL/ndn-python-repo/blob/master/ndn_python_repo/ndn-python-repo.conf.sample) 를 예로 들 수 있음

```
---
repo_config:
  # the repo's routable prefix
  repo_name: 'testrepo'
  # if true, the repo registers the root prefix. If false, client needs to tell repo
  # which prefix to register/unregister
  register_root: True 


db_config:
  # choose one among sqlite3, leveldb, and mongodb
  db_type: 'sqlite3'
  
  # only the chosen db's config will be read
  sqlite3:
    'path': '~/.ndn/ndn-python-repo/sqlite3.db'   # filepath to sqlite3 database file
  leveldb:
    'dir': '~/.ndn/ndn-python-repo/leveldb/'      # directory to leveldb database files
  mongodb:
    'db': 'repo'
    'collection': 'data'


tcp_bulk_insert:
  addr: '0.0.0.0'
  port: '7376'
  # when register_root is False, whether packets inserted via TCP triggers prefix registration
  register_prefix: True 


logging_config:
  # one of 'CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG'
  level: 'INFO'
  # absolute path to log file. If not given, logs to stdout
  # file: 'repo.log'
```

#### 현재

- DIFS 기반의 mongoDB 적용이 진행되고 있으며
- `insert` 관련 사항은 확인한 것으로 파악됨
  - DIFS config 등을 사용하여 db_type 을 mongodb 로 지정한 경우
  - data/manifest 모두 mongodb 를 사용하는 것으로 함
- `del` 관련하여 진행되어야 함

---

### ForwardingHint 적용

- DIFS 에 저장되는 데이터는 renaming 되기 때문에 signature 관련 이슈 발생
  - producer 가 저장욜 요청할 때 적용한 {data_name} 은
  - 데이터를 저장하는 DIFS 노드로 인하여
  - 결국 /{node_name}/{data_name} 형태로 변환됨
- ForwardingHint 를 활용하면
  - producer 가 저장욜 요청할 때 적용한 {data_name} 은
  - 데이터를 저장한 노드의 명을 제외한 {data_name} 그대로 사용
  - ForwardingHint 로 데이터를 저장한 노드의 {node_name} 을 TLV 에 적용
  - 결과적으로 /{node-name}/{data_name} 형태로 변환됨

#### 20' 이미 적용

- 큰 이슈는 없을 것으로 판단함
- 단, 노드에 데이터 저장시 /{common_name}/{data_name} 형태로 사용
  - [이슈 내용](https://github.com/uni2u/difs/issues/7#issuecomment-699843496)
  - INC 파트에서 사용하는 ForwardingHint 체계가 적용되었기 때문에 상호 확인이 필요함
- github branchs 정리가 필요하며
  - 브렌치 정리 시점에 해당 내용이 포함될 수 있도록 주의

> difs github branch 정리에 해당 내용이 반드시 포함될 수 있도록 주의
>> ForwardingHint 도 계속 변화가 있기 때문에 변화 사항을 

---

### command 체계 개선

- [ndn-python-repo](https://github.com/UCLA-IRL/ndn-python-repo) 의 command 체계를 적용함
  - [ndn-python-repo::doc](https://ndn-python-repo.readthedocs.io/en/latest/src/readme.html)

#### command parameter 확인

- 참조 [ndn-python-repo::insert](https://ndn-python-repo.readthedocs.io/en/latest/src/specification/insert.html)
- `/{difs_name}/insert`
  - command parameter
    - name
    - start_black_id (Optional)
    - end_block_id (Optional)
    - forwarding_hint (Optional)
    - register_prefix (Optional): 신규
    - check_prefix: 신규
    - process_id
    - ~InterestLifttime~: 제거

> 내용 파악 진행중
>> 현재까지는 문제 없을 것으로 판단하지만 Segment numbers 형식이 [NDN naming conventions rev2](https://named-data.net/publications/techreports/ndn-tr-22-2-ndn-memo-naming-conventions/) 를 따르기 때문에 내용 분석 필요함

---

### 모니터링 지원

- 모니터링을 위한 정보는 크게 다음 2가지로 구분됨
  - 동적 노드 정보: 동적노드구성 정보
  - 리소스 정보

#### 동적 노드 정보

- manager node 를 통해 동적 노드 정보를 제공
- manager node 는 KeySpace 정보를 제공
- manager node 는 DIFS 클러스터의 노드 리스트 제공
- 모니터링을 담당하는 서버는 주기적으로 manager node 에 동적 노드 정보를 요청

#### 동적 노드 정보 제공 json

- hashes
  - KeySpace 정보로서 manager node 는 DIFS 클러스터 노드의 전체 KeySpace 를 관리
- nodes
  - DIFS 각 노드의 monitoring prefix 로서 모니터링 서버는 이를 활용해 각 노드의 리소스 정보 요청

```json
{
  "info": {
    "hashes": [
      {
        "name": "node_name",
        "range": {
          "start": "start_hash",
          "end": "end_hash"
        }
      },
      {
        "name": "node_name",
        "range": {
          "start": "start_hash",
          "end": "end_hash"
        }
      }
    ],
    "nodes": [
      {
        "name": "node_monitoring_prefix"
      },
      {
        "name": "node_monitoring_prefix"
      }
    ]
  }
}
```

#### 리소스 정보

- DIFS 각 노드는 자신이 관리중인 content 목록을 제공
- DIFS 각 노드는 자신의 리소스 정보를 제공
- 모니터링을 담당하는 서버는 주기적으로 DIFS 각 노드에 모니터링 정보를 요청

#### 리소스 정보 제공 json

- manifest
  - 자신이 저장하고 있는 manifest 를 제공
- data
  - 자신이 저장하고 있는 data 를 제공

```json
{
  "info": {
    "name": "node_name",
    "resource": {
      "disk": {
        "size": "total size",
        "usage": "usage"
      },
      "memory": {
        "size": "total size",
        "usage": "usage"        
      }
    },
    "contents": [
      {
        "manifests": [
          {
            "key": "manifest_name"
          },
          {
            "key": "manifest_name"
          }
        ],
        "datas": [
          {
            "data": "data_name"
          },
          {
            "data": "data_name"
          }
        ]
      }
    ]
  }
}
```

---

### user lib 지원

- DIFS 를 사용하는 user 를 위함
- user (producer/consumer) 의 application 이 DIFS 에 데이터를 저장하거나 사용하는 경우
- application 의 함수에서 insert/get/del 등을 호출할 수 있도록 함
- DIFS 는 cpp 로 작성된 관계로 cpp 기반의 라이브러리 제공
  - .so
- INC 등에서 사용할 수 있도록 기본 이미지에  DIFS lib 탑재

# 기능개선: KeyValue Store 고도화

## KeyValue Store overview

### nosql 적용

- DIFS 저장 노드의 다양성을 지원하기 위함
- nosql 은 mongoDB 를 대상으로 함
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
- github branchs 정리가 필요하며
  - 브렌치 정리 시점에 해당 내용이 포함될 수 있도록 주의

> difs github branch 정리에 해당 내용이 반드시 포함될 수 있도록 주의

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

### user lib 지원

- DIFS 를 사용하는 user 를 위함
- user (producer/consumer) 의 application 이 DIFS 에 데이터를 저장하거나 사용하는 경우
- application 의 함수에서 insert/get/del 등을 호출할 수 있도록 함
- DIFS 는 cpp 로 작성된 관계로 cpp 기반의 라이브러리 제공
  - .so
- INC 등에서 사용할 수 있도록 기본 이미지에  DIFS lib 탑재

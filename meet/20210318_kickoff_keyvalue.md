# 기능개선: KeyValue Store 고도화

## KeyValue Store overview

### nosql 적용

### ForwardingHint 적용

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

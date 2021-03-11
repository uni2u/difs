# 기능개선: 동적노드구성

## 동적구성 overview

- manager node 에 의해 관리되는 클러스터에서
- 데이터의 정보 (어떤 노드가 실제 저장하고 있는지) 를 명시한 manifest file 은
- service name (/{common_name}/{data_name}) 을 통해 제공되는데
- {data_name} 을 hash 하여 얻을 결과값을 담당하는 DIFS 클러스터 노드에서 관리한다
- consumer 는 자신이 알고 있는 {data_name} 에 DIFS {common_name} 을 조합하면
- 이를 수신한 DIFS 노드는 {data_name} 을 hash 하여 담당 노드를 찾고
- 담당 노드로 부터 수신한 manifest file 을 consumer 에게 전달한다
- consumer 는 manifest file 을 확인하고 실제 데이터가 저장된 노드의 {node_name} 을 사용하여 Interest 를 전송하고
- 이 Interest 를 수신한 노드는 자신이 저장중인 실제 데이터를 전송한다

## 동적구성 개념

### manager node

- 신규 노드 추가 및 기존 노드 제거
- hash range 할당 및 KeySpace 생성/업데이트

### 동적 노드 구성을 위한 config

- DIFS 각 노드의 config 파일 활용 (json file)
  - manager node
    - config 의 `type: manager`,`parent: null`, `manager: null`
  - node
    - config 의 `type: node`, `parent: node_name`, `manager: manager_node_name`

```json
{
  "info": {
    "type": "manager or node",
    "parent": "node_name",
    "manager": "manager_node_name"
  }
}
```

> 추후 DIFS config 파일에 통합할 수 있도록 논의 필요

### manager node 의 node 관리

- 각 노드는 실행과 동시에 manager node 로 join request 보냄
- manager node 는 parent 정보를 확인 후
- parent 노드의 KeySpace 를 나누고 추가된 노드에게 hash range 할당
- manager node 는 KeySpace file 구성

```json
{
  "keyspace": [
    {
      "node-name": "node_name",
      "start": "start hash num",
      "end": "end hash num"
    },
    {
      "node-name": "node_name",
      "start": "start hash num",
      "end": "end hash num"
    }
  ]
}
```

> mongoDB 를 사용하는 것에 대한 논의 필요
>> key(version_num), value({node_name, start, end},{node_name, start, end},...)

### Added Node 란

- DIFS 관리자가 모니터링을 하다가 리소스가 모자란 경우 물리 서버를 추가하는 모델
  - IPFS 또는 torrent 또는 Cassandra 에서 사용하는 DHT 의 모양이 아님
  - 폐쇄된 환경에서 관리자가 필요에 의해 필요한 곳에 물리적으로 증설하는 개념
- manager node 는 물리적으로 설치한 서버를 DIFS 클러스터에 포함시킴

#### range split

- 새로운 노드가 추가되면
- manager node 는 새로운 노드가 추가될 hash range 를 계산하고
- KeySpace 정보를 check KeySpace version Interest 를 통해 전송
- 모든 노드는 manager node 로 부터 check KeySpace version Interest 를 받고
- 자신의 KeySpace 버전 정보와 다른경우 manager node 로 fetch KeySpace file Interest 를 보내고
- manager node 로 부터 KeySpace 정보를 받아 테이블을 업데이트 함
- 새로운 노드 추가로 인하여 hash range 의 수정이 생긴 노드는
- 추가된 새로운 노드로 부터 Interest 를 받고 Data (manifest file) 를 전송

```
+---------------------+---------------------+---------------------+
|        node01       |        node02       |        node03       |
+---------------------+---------------------+---------------------+
|       (20, 30]      |       (30, 40]      |       (40, 50]      |
+---------------------+---------------------+---------------------+

+--------+
|NEW node|-----------------------------+
+--------+                             |
                                       V
+---------------------+----------+----------+---------------------+
|        node01       |  node02  | NEW node |        node03       |
+---------------------+----------+----------+---------------------+
|       (20, 30]      | (30, 35] | (30, 40] |       (40, 50]      |
+---------------------+----------+----------+---------------------+
```

### Leave Node 란

- DIFS 관리자가 모니터링을 하다가 리소스가 필요없는 경우 물리 서버를 제거하는 모델
  - IPFS 또는 torrent 또는 Cassandra 에서 사용하는 DHT 의 모양이 아님
  - 폐쇄된 환경에서 관리자가 필요에 의해 필요한 곳의 물리적 서버를으로 제거하는 개념
- manager node 는 물리적으로 설치한 서버를 DIFS 클러스터에서 제거함

#### range merge

- 기존 노드가 제거되면
- manager node 는 기존 노드가 삭제될 hash range 를 계산하고
- KeySpace 정보를 check KeySpace version Interest 를 통해 전송
- 모든 노드는 manager node 로 부터 check KeySpace version Interest 를 받고
- 자신의 KeySpace 버전 정보와 다른경우 manager node 로 fetch KeySpace file Interest 를 보내고
- manager node 로 부터 KeySpace 정보를 받아 테이블을 업데이트 함
- 기존 노드 제거로 인하여 hash range 의 수정이 생긴 노드는
- 삭제될 노드로 Interest 를 보내고 삭제될 노드는 Data (manifest file) 를 전송

```
+----------+
|LEAVE node|<--------------------------+
+----------+                           |
                                       |
+---------------------+----------+----------+---------------------+
|        node01       |  node02  |Leave node|        node03       |
+---------------------+----------+----------+---------------------+
|       (20, 30]      | (30, 35] | (30, 40] |       (40, 50]      |
+---------------------+----------+----------+---------------------+

+---------------------+---------------------+---------------------+
|        node01       |        node02       |        node03       |
+---------------------+---------------------+---------------------+
|       (20, 30]      |       (30, 40]      |       (40, 50]      |
+---------------------+---------------------+---------------------+
```

## 동적 노드 구성을 위한 Interest

### mangaer node 의 노드 구성

- check init node **_Interest_**
  - `/{manager_node_name}/init`
    - parent 정보 (body)
  - parent 정보
    - 참여할 노드가 나누어 가질 hash range 를 담당하는 노드를 의미함
    - Operator 가 물리적으로 대상 노드의 오버헤드를 줄이기 위하여 추가함
  - simple flow
    - manager node 는 DIFS 클러스터에 참여할 노드로 부터 parent 정보가 포함된 Interest 를 받고
    - manager node 는 parent 정보를 확인 후 parent 노드의 hash range 를 나누어
    - 참여할 노드에게 할당하여 KeySpace를 재구성

### manager node 의 KeySpace 관리

- check KeySpace version **_Interest_** (to node)
  - `/{node_name}/keyspace/ver/{version_num}`
  - manager node 가 관리하는 range allocation 의 KeySpace version 정보 공유 (version 번호만)
    - DIFS 클러스터 노드에게 manager node 가 KeySpace 버전 정보를 안내함
    - 노드는 200 OK 로 응답
  - simple flow
    - manager node 는 DIFS 클러스터의 모든 노드를 알고 있음 (added/leave node 포함)
      - manager node 는 모든 노드에 대한 hash range allocation 을 계산
      - 각 노드의 KeySpace 정보를 담은 파일을 생성하고 버전을 만들어 제공
      - 최신 KeySpace 버전을 담은 Interest 를 버전이 갱신될 때 마다 모든 노드에게 전송
    - 각 노드는 manager node 가 보낸 버전 정보가 자신이 가진 KeySpace 버전과 다른 경우
      - manager node 에게 KeySpace 정보를 얻기위한 Interest (`fetch KeySpace file` Interest) 전송

- fetch KeySpace file **_Interest_** (From namager)
  - `/{manager_node_name}/keyspace/fetch/{version_num}`
  - manager node 에 의해 관리되는 range allocation 의 KeySpace version 에 대한 최신 파일 정보
    - manager node 로 부터 받은 KeySpace 버전 정보가 자신이 가지고 있는 버전 정보와 다른 경우 KeySpace 테이블 업데이트 파일을 요청
    - manager node 는 KeySpace 정보를 담은 파일로 응답
  - simple flow
    - manager node 로 부터 받은 `check KeySpace version` 정보가 자신이 가진 KeySpace 버전과 다른 경우 KeySpace 테이블 업데이트가 있음을 확인
    - 각 노드는 manager node 로 KeySpace 테이블 업데이트를 위한 최신 버전의 파일 요청을 위한 Interest 전송
      - manager node 는 해당 버전의 KeySpace 파일 제공

### KeySpace 변화가 있는 노드간 파일 재조정

- coordination **_Interest_**
  - `/{node_name}/manifestlist/` or `/{node_name}/manifestlist/start/{start_num}/end/{end_num}`
    - start: 파일을 가지고 와야하는 노드의 KeySapce 시작
    - end: 파일을 가지고 와야하는 노드의 KeySpace 마지막
    - **이러한 형식으로 start 부터 end 까지 manifest 를 한번에 제공이 가능한가?**
      - 데이터를 나누어야 할 노드는 위 Interest 를 받고
      - 자신이 담당하는 manifest 중 범위에 해당하는 것을 골라 데이터로 제공
      - 또는 자신이 담당하는 manifest 중 범위에 해당하는 것에 대한 리스트 파일을 만들어서 제공
        - 이 리스트를 받은 노드는 manifest 요청 Interest 를 전송
          - `/{node_name}/manifest/{hash(manifest key)}`
    - **다른 방안**
      - 각 노드는 자신이 가진 manifest 리스트를 만들고
      - 위 Interest 를 통해 manifest 리스트 파일을 제공하고
      - manifest 리스트를 받은 노드는 자신이 담당할 KeySpace 에 포함되는 manifest 를 각각 호출
        - `/{node_name}/manifest/{hash(manifest key)}`
    - **manifest_list file**
      - DIFS 각 노드는 자신이 관리중인 manifest file 의 리스트를 제공하여야 함
      - 추가된 노드가 자신의 KeySpace 에 해당하는 manifest file 을 요청하여야 하는데
      - 어떤 manifest file 을 요청해야 하는지에 대한 정보가 필요함
      - manifest_list file 은 추후 모니터링에도 활용할 수 있도록 함
  - KeySpace 테이블 업데이트 정보를 제공받은 각 노드
    - 자신이 저장중인 manifest 파일을 제공할 노드를 알 수 있음 (기존 노드)
    - 자신이 저장할 manifest 파일을 제공받을 노드를 알 수 있음 (추가 노드)
  - **변화된 KeySpace 에 해당하는 두 노드간 전송되는  _Interest/Data_**
    - KeySpace 테이블 정보를 활용하여 노드가 담당하는 ID Range 를 split/merge
      - manager node 로 부터 제공받은 재조정 range allocation 에 대한 KeySpace version 업데이트 정보
    - manager node 로 부터 제공받은 KeySpace 정보를 토대로 당사자간 manifest 를 주고 받기 위한 내용

> 신규 노드가 참여하여 기존 노드의 manifest 를 나누기 위해서는 기존 노드가 가지고 있는 manifest 의 naming 을 정확히 알아야 함
> 
> coordination Interest 를 통하여 기존 노드가 가지고 있는 manifest 의 naming 이 명시된 manifest 리스트를 받아야 함
> 
> manifest 리스트에 명시된 manifest name 은 hash 또는 name 으로 제공되어 신규 노드가 담당할 hash range 의 manifest file 을 요청함
> 
> (name 이 hash 인 경우 자신의 hash range 에 해당하는 manifest 요청, name 이 name 인 경우 hash 계산을 통하여 manifest 요청)

```json
# 아래 내용은 모니터링을 위한 노드 리소스 정보를 제공하는 예제이며
# 아래 내용 중 contents/manifests 부분이 해당 노드가 관리중인 manifest 의 리스트

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

## flows

### init node (config)

```
+-------+                         +-------+
|NEW    |                         |difs   |
|node   |                         |node   |
+-------+                         +-------+
    |                                 |
    |--+                              |
    |  |                              |
check config                          |
type: manager                         |
    |  |                              |
    |--+                              |
    |                                 |
    |--+                              |
    |  |                              |
Assignment of an ID range             |
create KeySpace file                  |
    |  |                              |
    |--+                              |
    |                                 |
    ~                                 ~
    |                                 |
    |                                 |--+
    |                                 |  |
    |                              set config
    |                              type: node
    |                              parent: manager_node_name
    |                                 |  |
    |                                 |--+
    |                                 |
    |----------Interest (init)------->|
    |                                 |
    |<-------Data (config file)-------|
    |                                 |
    |--+                              |
    |  |                              |
check config                          |
type: node                            |
parent: manager_node_name             |
    |  |                              |
    |--+                              |
    |                                 |
    |--+                              |
    |  |                              |
Assignment of an ID range             |
create KeySpace file                  |
    |  |                              |
    |--+                              |
    |                                 |
```

### Added node (add)

```
+-----------+                       +-------+                         +-------+                         +-------+
|Added      |                       |manager|                         |difs   |                         |difs   |
|node       |                       |node   |                         |node   |                         |node   |
+-----------+                       +-------+                         +-------+                         +-------+
      |                                 |                                 |                                 |
      |                                 |--+                              |                                 |
      |                                 |  |                              |                                 |
      |                        Assignment of an ID range                  |                                 |
      |                        Detect Added node                          |                                 |
      |                                 |  |                              |                                 |
      |                                 |--+                              |                                 |
      |                                 |                                 |                                 |
      |<--Interest (check keyspace version)                               |                                 |
      |----------Data (200OK)---------->|                                 |                                 |
      |                               Interest (check keyspace version)-->|                                 |
      |                                 |<---------Data (200OK)-----------|                                 |
      |                                 |-----------------Interest (check keyspace version)---------------->|
      |                                 |<--------------------------Data (200OK)----------------------------|
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |-Interest (fetch KeySpace file)->|                                 |                                 |
      |<------Data (KeySpace file)------|                                 |                                 |
      |                                 |<-Interest (fetch KeySpace file)-|                                 |
      |                                 |-------Data (KeySpace file)----->|                                 |
      |                                 |------------------Interest (fetch KeySpace file)------------------>|
      |                                 |<----------------------Data (KeySpace file)------------------------|
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |-------------------Interest (Coordination Request)---------------->|                                 |
      |                                 |                                 |                                 |
      |<---------------------Data (manifest list file)--------------------|                                 |
      |                                 |                                 |                                 |
      |--------------Interest (Copy of KeyValue of ID Range)------------->|                                 |
      |                                 |                                 |--+                              |
      |                                 |                                 |  |                              |
      |                                 |                           ID range split                          |
      |                                 |                                 |  |                              |
      |                                 |                                 |--+                              |
      |<---------------Data (Copy of KeyValue of ID Range)----------------|                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
                                                 ## replica set ##
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |-------------------Interest (Coordinate request)------------------>|
      |                                 |                                 |                                 |
      |                                 |<-----------------Data (200OK; Coordinate reply)-------------------|
      |                                 |                                 |                                 |
```

### Leave node (del)

```
+-----------+                       +-------+                         +-------+                         +-------+
|Leave      |                       |manager|                         |difs   |                         |difs   |
|node       |                       |node   |                         |node   |                         |node   |
+-----------+                       +-------+                         +-------+                         +-------+
      |                                 |                                 |                                 |
      |                                 |--+                              |                                 |
      |                                 |  |                              |                                 |
      |                        Assignment of an ID range                  |                                 |
      |                        Detect Leave node                          |                                 |
      |                                 |  |                              |                                 |
      |                                 |--+                              |                                 |
      |                                 |                                 |                                 |
      |                               Interest (check keyspace version)-->|                                 |
      |                                 |<---------Data (200OK)-----------|                                 |
      |                                 |-----------------Interest (check keyspace version)---------------->|
      |                                 |<--------------------------Data (200OK)----------------------------|
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |<-Interest (fetch KeySpace file)-|                                 |
      |                                 |-------Data (KeySpace file)----->|                                 |
      |                                 |------------------Interest (fetch KeySpace file)------------------>|
      |                                 |<----------------------Data (KeySpace file)------------------------|
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |<------------------Interest (Coordination Request)-----------------|                                 |
      |                                 |                                 |                                 |
      |----------------------Data (manifest list file)------------------->|                                 |
      |                                 |                                 |                                 |
      |<-------------Interest (Copy of KeyValue of ID Range)--------------|                                 |
      |                                 |                                 |--+                              |
      |                                 |                                 |  |                              |
      |                                 |                           ID range merge                          |
      |                                 |                                 |  |                              |
      |                                 |                                 |--+                              |
      |----------------Data (Copy of KeyValue of ID Range)--------------->|                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
                                                 ## replica set ##
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |                                 |                                 |
      |                                 |-------------------Interest (Coordinate request)------------------>|
      |                                 |                                 |                                 |
      |                                 |<-----------------Data (200OK; Coordinate reply)-------------------|
      |                                 |                                 |                                 |
```

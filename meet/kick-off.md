## 중앙집중형 방식

- 모든 노드는 **manager node** 에 의해 제어

### manager node

-

## 동적 노드 구성을 위한 Interest

- coordination __Interest__
  -  `/{node_name}/range/vid/{view_num}/%DA/{data_name}/%TA/{target_node_id}`
      - {data_name}: /{node_id}/{sequence_num}
      - %DA: data_nameSeparator=name::Component::fromEscapedString("%DA")
      - %TA: target_data_nameSeparator=name::Component::fromEscapedString("%DA")
  - 최종버전의 KeySpace 테이블 정보 제공
      - KeySpace 테이블 정보를 활용하여 노드가 담당하는 ID Range 를 split/merge
      - 각 노드는 KeySpace 조정
  - 재조정 range allocation 에 대한 information view 업데이트 정보

## flows

### Added node (add)

- DIFS 관리자가 모니터링을 하다가 리소스가 모자란 경우 물리 서버를 추가하는 모델
  - IPFS 또는 torrent 에서 사용하는 DHT 의 모양이 아님
  - 폐쇄된 환경에서 관리자가 필요에 의해 필요한 곳에 물리적으로 증설하는 개념
- manager node 는 물리적으로 설치한 서버를 DIFS 클러스터에 포함시킴

```
+-----------+                       +-------+                         +-------+                         +-------+
|Added/Leave|                       |manager|                         |difs   |                         |difs   |
|node       |                       |node   |                         |node   |                         |node   |
+-----------+                       +-------+                         +-------+                         +-------+
      |                                 |                                 |                                 |
      |<-Interest (Coordinate request)--|                                 |                                 |
      |                                 |--+                              |                                 |
      |                                 |  |                              |                                 |
      |                        Assignment of an ID range                  |                                 |
      |                        Detect Added node                          |                                 |
      |                                 |  |                              |                                 |
      |                                 |--+                              |                                 |
      |<-Data (200OK; Coordinate reply)-|                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |--Interest (Coordinate request)->|                                 |
      |                                 |                                 |                                 |
      |                                 |<-Data (200OK; Coordinate reply)-|                                 |
      |                                 |                                 |                                 |
      |--------------Interest (Copy of KeyValue of ID Range)------------->|                                 |
      |                                 |                                 |--+                              |
      |                                 |                                 |  |                              |
      |                                 |                           ID range split                          |
      |                                 |                                 |  |                              |
      |                                 |                                 |--+                              |
      |<---------------Data (Copy of KeyValue of ID Range)----------------|                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |-------------------Interest (Coordinate request)------------------>|
      |                                 |                                 |                                 |
      |                                 |<-----------------Data (200OK; Coordinate reply)-------------------|
      |                                 |                                 |                                 |
```

### Leave node (del)

- DIFS 관리자가 모니터링을 하다가 리소스가 필요없는 경우 물리 서버를 제거하는 모델
  - IPFS 또는 torrent 에서 사용하는 DHT 의 모양이 아님
  - 폐쇄된 환경에서 관리자가 필요에 의해 필요한 곳의 물리적 서버를으로 제거하는 개념
- manager node 는 물리적으로 설치한 서버를 DIFS 클러스터에서 제거함

```
+-----------+                       +-------+                         +-------+                         +-------+
|Added/Leave|                       |manager|                         |difs   |                         |difs   |
|node       |                       |node   |                         |node   |                         |node   |
+-----------+                       +-------+                         +-------+                         +-------+
      |                                 |                                 |                                 |
      |<-Interest (Coordinate request)--|                                 |                                 |
      |                                 |--+                              |                                 |
      |                                 |  |                              |                                 |
      |                        Assignment of an ID range                  |                                 |
      |                        Detect Leave node                          |                                 |
      |                                 |  |                              |                                 |
      |                                 |--+                              |                                 |
      |<-Data (200OK; Coordinate reply)-|                                 |                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |--Interest (Coordinate request)->|                                 |
      |                                 |                                 |                                 |
      |                                 |<-Data (200OK; Coordinate reply)-|                                 |
      |                                 |                                 |                                 |
      |<-------------Interest (Copy of KeyValue of ID Range)--------------|                                 |
      |                                 |                                 |--+                              |
      |                                 |                                 |  |                              |
      |                                 |                           ID range merge                          |
      |                                 |                                 |  |                              |
      |                                 |                                 |--+                              |
      |----------------Data (Copy of KeyValue of ID Range)--------------->|                                 |
      |                                 |                                 |                                 |
      ~                                 ~                                 ~                                 ~
      |                                 |                                 |                                 |
      |                                 |-------------------Interest (Coordinate request)------------------>|
      |                                 |                                 |                                 |
      |                                 |<-----------------Data (200OK; Coordinate reply)-------------------|
      |                                 |                                 |                                 |
```

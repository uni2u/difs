### dd


### 동적 노드 구성을 위한 Interest

- coordination __Interest__
  -  `/{node_name}/range/vid/{view_num}/%DA/{data_name}/%TA/{target_node_id}`
      - {data_name}: /{node_id}/{sequence_num}
      - %DA: data_nameSeparator=name::Component::fromEscapedString("%DA")
      - %TA: target_data_nameSeparator=name::Component::fromEscapedString("%DA")
  - 최종버전의 KeySpace 테이블 정보 제공
      - KeySpace 테이블 정보를 활용하여 노드가 담당하는 ID Range 를 split/merge
      - 각 노드는 KeySpace 조정
  - 재조정 range allocation 에 대한 information view 업데이트 정보

### flows

#### Added node (add)

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

#### Leave node (del)

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

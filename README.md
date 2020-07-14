## DIFS (Data Infra File System)

### Abstraction
DIFS 시스템은 NDN 기반 환경에서 분산 데이터 저장소의 저장 및 관리를 위한 Object Storage 로서 [repo-ng](https://github.com/named-data/repo-ng) 를 기반으로 작성되었다.
특히, 저장 능력이 없는 센서 또는 대용량 데이터를 생성하는 장치등에서 발생한 데이터들을 안전하게 분산 저장할 수 있다.
DIFS 는 NDN 기반 Object Storage 로서 데이터가 생성된 위치와 상관없이 데이터가 저장된 저장소의 이름 및 데이터 구성이 포함된 메타데이터 정보를 제공한다.

```
          +-----------------------------+-----------------------------+
          |                             |                             |
          V                             V                             V
+---[DIFS node1]---+          +---[DIFS node2]---+          +---[DIFS node3]---+
| +--------------+ |          | +--------------+ |          | +--------------+ |
| | file storage | |          | | file storage | |          | | file storage | |
| | ------------ | |          | +--------------+ |          | +--------------+ |
| | |_ dir       | |          | | key/value    | |          | | key/value    | |
| |   |_ segment | |          | +--------------+ |          | +--------------+ |
| +--------------+ |          +------------------+          +------------------+
| | key/value    | |
| | ------------ | |
| |  (hash,meta) | |
| |  (hash,meta) | |
| +--------------+ |
+------------------+
```

### Introduction
전통적으로 Object Storage 는 고유 식별자 (ID) 를 사용하여 Object 와 메타데이터를 매핑하여 관리한다.
DIFS 는 NDN 네트워크를 위한 대규모 Object Storage 로서 다음과 같이 구성된다.

- 데이터는 노드에 구성된 파일 시스템에 저장
- 각 데이터는 이름이 있으며 data-name 은 해시 엔진을 통해 고유 ID 를 가짐
- 해시 값을 사용하여 디렉토리를 구성하고 메타데이터를 저장할 위치 결정

#### _File Storage_
파일 저장소는 inux 파일 시스템인 ext4 파일 시스템을 기반으로 하고 있다.
데이터를 체계적으로 저장하기 위하여 Git 에서와 같이 디렉토리에 저장된다.
데이터 세그먼트는 데이터 이름의 처음 2 바이트 해시 값을 사용하여 디렉토리를 구성한 후 저장된다.
다른 응용 프로그램에서 생성 된 다양한 유형과 크기의 데이터에 대처하기 위해 BLOB 를 지원한다.

```
/data
 |_ /dir(hash 2 bytes)
 |  |_ segment
 |  |_ segment
 |
 |_ /dir
    |_ segment
```

#### _Metadata Store_
Metadata Store 는 데이터 이름 해시값을 Key 로 메타데이터를 제공하기 위한 Key/Value Store 형태이다.
메타데이터 정보는 실제로 데이터를 저장하는 노드에 대한 정보 제공한다.
특히, 일관된 해싱 알고리즘을 사용하여 데이터를 분산시키기 때문에 분산 해시 링으로 표시할 수 있다.
DIFS 를 구성하는 노드는 유일한 이름을 가지고 있으며 노드 이름의 해싱 결과에 따라 각 노드가 저장하는 데이터 범위를 결정하는 노드 범위 지정한다.
해시 알고리즘을 활용하면 동일한 데이터 이름을 해시하는 경우 항상 같은 결과를 얻을 수 있으며 이것은 Metadata Store 를 구성하는 핵심이다.
메타데이터에는 데이터 이름, 데이터 이름의 해시 값, 데이터를 저장하는 노드에 대한 정보 및 세그먼트 번호를 포함한 데이터 정보 포함한다.

```
Key: data_name hash result / Value: metadata

{
  "info": {
    "name": "content_name",
    "hash": "hash_value",
    "version_info": "integer"
  },
  "storages": [
    {
      "storage_name": "storage prefix",
      "segment": {
        "start_num": "integer",
        "end_num": "integer"
      }
    },
    {
      "storage_name": "storage prefix",
      "segment": {
        "start_num": "integer",
        "end_num": "integer"
      }
    }
  ]
}
```

#### _Hash Engine_
NDN 에서 모든 Data 는 고유의 이름을 가지고 있으며 이름은 중복되지 않는다.
즉, 사용자는 Data 의 고유 이름을 알고 있다면 데이터에 접근할 수 있는 것이다.
하지만 Data 의 이름은 네트워크까지 포함하여 구성되어 있기 때문에 매우 복잡한 이름으로 구성된다 (예: '/network/domain/data').
사용자가 network 을 포함한 Data 의 full name 을 알고 있어야 하였으며 이 이름은 네트워크를 지나면서 절대로 변형되지 않아야 한다.
DIFS 는 이러한 문제를 'hash engine' 을 사용하여 해소한다.
'같은 이름을 해싱한 결과는 동일하다.' 는 특성을 활용하여 Data 이름을 활용하여 Data 의 full name 을 제공한다.
데이터 생산자가 데이터를 저장하면서 제시한 `data name` 은 _hash engine_ 을 통해 결과값을 가지게 된다.
이 결과값을 사용하여 DIFS 클러스터를 구성하고 있는 노드 중 _metadata_ 를 구성할 Key/Value Store 를 지정한다.
지정된 노드의 Key/Value Store 에 _metadata_ 를 생성하고 _hash 결과값_ 과 매치한다.
즉, _hash result_ 를 Key 로 하는 _metadata_ Value 쌍이 구성된다.

```
[Data Insert]
                               +------select key/value store node------+
                               |                using hash result      |
                               |                                       V
producer --'data_name'--> DIFS node1 --+           ......     --> DIFS nodeX --+
                               ^       |                               ^       |
                               |       |                               | set Key/Value store
                               | data_name hash                        | create metadata
                               |       |                               |       |
                               +-------+                               +-------+

---

[Data Retrive]
                               +------search key/value store node------+
                               |                using hash result      |
                               |                                       V
consumer --'data_name'--> DIFS node3 --+           ......     --> DIFS nodeX --+
                               ^       |                               ^       |
                               |       |                               | get Key/Value store
                               | data_name hash                        | response metadata
                               |       |                               |       |
                               +-------+                               +-------+
```


### Our Goal

> NDN 기반 Object Storage

- '_DIFS = DIFS file storage + DIFS Key/Value store_'

If you're curious about DIFS, visit our [WiKi page](https://github.com/uni2u/difs/wiki).

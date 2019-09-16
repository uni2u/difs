## DIFS (Data Infra File System)

** Our Goal **

> Object Storage

- using command (repo-ng)
  - https://redmine.named-data.net/projects/repo-ng/wiki

** What's Different (DIFS/repo-ng) **

|  | repo-ng | DIFS |
|---|:---|:---|
|stored data|sqlite3 (DB)|file storage|
|stored type|segment|segment|

### Data Storage

#### File Storage

- Store segments (segment = data chunk)

```
                                                          + contents
                                                            - segment01
                                                            - segment02
                                                            - ...
                                                            - segmentXX
  (source)        (source)        (source)               (source)
      |               |               |                      |
      R ------------- R ------------- R -------------------- R
      |               |               |                      |
      |               |               |                      |
(/storage01)    (/storage02)    (/storage03)    ...    (/storageXX)
                                                         + stored segments
                                                           - segment01
                                                           - segment02
                                                           - ...
                                                           - segmentXX
                                                         + contents_name hashing
                                                           - find key-value store
                                                           - create key-value request
```

### Data Key-Value Store

```
      R ------------- R ------------- R -------------------- R
      |               |               |                      |
      |               |               |                      |
(/storage01)    (/storage02)    (/storage03)    ...    (/storageXX)
                  + selected key-value storage
                    - create key-value table
                    - key: contents_name hashing
                    - value: who stored this segments
                  + create manifest
                    - contents_info
                    - servers
                    - segments
```


- manifest file example 

``` json
{
  "info": {
    "parents_info": "hash_value",
    "root_info": "hash_value",
    "vershin_info": "integer"
  },
  "storages": [
    {
      "sotrage_name": "storage name",
      "segment": {
        "start_num": "integer",
        "end_num": "integer"
      }
    },
    {
      "sotrage_name": "storage name",
      "segment": {
        "start_num": "integer",
        "end_num": "integer"
      }
    }
  ]
}
```

### Sequence

- insert
  - producer send interest to DIFS
    - interest: /repo/{insert}/a.com/izone.mp4
  - producer: create data
  - storage
    - each storage have a name (/storage01)
    - store segments
    - ex) /storage01/#1 -> stored segment prefix

```
+----------+                                +------------+
| producer |                                | /storage01 |
+----------+                                +------------+
      |  interest: /repo/{insert}/a.com/izone.mp4  |
      |------------------------------------------->|
      |                                            |
      |               state: 200 OK                |
      |<-------------------------------------------|
      |                                            |
      |         interest: /a.com/izone.mp4         |
      |<-------------------------------------------|
      |      response data: manifest or segment    |
      |------------------------------------------->|
      |       interest: /a.com/izone.mp4/#01       |
      |<-------------------------------------------|
      |          response data: segment#01         |
      |------------------------------------------->|
      |                                            |
      ~                                            ~
      |  interest: /repo/{watch}/{check}/a.com/izone.mp4
      |------------------------------------------->|
      |               state: 200 OK                |
      |<-------------------------------------------|
```

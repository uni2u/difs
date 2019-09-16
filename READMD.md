## DIFS (Data Infra File System)

#### Our Goal

> Object Storage

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

```sequence
Alice->Bob: Hello Bob, how are you?
Note right of Bob: Bob thinks
Bob-->Alice: I am good thanks!
```

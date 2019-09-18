## DIFS (Data Infra File System)

** Our Goal **

> Object Storage

- using command (repo-ng)
  - https://redmine.named-data.net/projects/repo-ng/wiki

** What's Different (DIFS/repo-ng) **

|  | repo-ng | DIFS |
|---|:---|:---|
|storage|sqlite3 (DB)|object storage|
|stored type|segment|segment|
|...|...|...|

### DIFS

> NDN based Object Storage
>> segments are stored on file storage

>> content is retrieved through key-value storage which file storage has been stored

>> DIFS = DIFS file storage + DIFS Key/Value storage

#### DIFS File Storage

- store segments (segment = data chunk)

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
                                                         + create info
                                                           - content_name
                                                           - content_name hashing
                                                           - segments
                                                           - rename
```

- info file example (like manifest)

```json
{
  "name": "content name",
  "hash": "content name hashing result",
  "segment": {
    "start_num": "integer",
    "end_num": "integer"
  },
  "rename": "stored storage name"
}
```

#### DIFS Key-Value Storage

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
  - producer send interest to DIFS storage
    - interest: /repo/{insert}/a.com/izone.mp4
    - producer: create data
    - using repo-ng command
  - DIFS file storage
    - each storage have a name (/storage01)
    - store segments
      - ex) /storage01/#1 -> stored segment prefix
    - create info (like manifest) file
      - name: /a.com/izone.mp4
      - hash: hashing result (/a.com/izone.mp4)
      - segments: start/end num
      - stored: /storage01/contents

```
+----------+                                +------------+
| producer |                                | /storage01 |
+----------+                                +------------+
      |                                            |
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
      ~                                            ~
      |       interest: /a.com/izone.mp4/#end      |
      |<-------------------------------------------|
      |          response data: segment#end        |
      |------------------------------------------->|
      ~                                            ~
      |                                            |---+
      |                                            |   |
      |                                  create info (manifest)
      |                                    name: /a.com/izone.mp4
      |                                    hash: hashing result (/a.com/izone.mp4)
      |                                    segments: start/end num
      |                                    rename: /storage01/contents
      |                                            |   |
      |                                            |<--+
      |                                            |
```

- who is key-value
  - stored DIFS file storage find DIFS K/V storage
  - each DIFS storage has its own hash value range
  - DIFS file storage that stores data, hashes content names
    - compare with storage hash value range and content name hash result
  - DIFS file storage send interest to DIFS K/V storage
    - interest: /{find storage}/{create}/{table}/a.com/izone.mp4
    - DIFS command: new command
  - DIFS K/V storage
    - create K/V table
    - send interest to data storage
      - interest: /{data storage}/{info}/a.com/izone.mp4
      - response: producer created info file
      - DIFS command: new command
    - create manifest file
      - store info: who stored data (/storage01/contents)
      - segments info: segment start/end number
      - you can see: manifest file example (SECTION: DIFS Key-Value Storage)
    - insert K/V
      - Key: hashing result (/a.com/izone.mp4)
      - Value: manifest file

```
+------------+                              +------------+
| /storage01 |                              | /storage04 |
+------------+                              +------------+
      |                                            |
  +---|                                            |
  |   |                                            |
hashing: /a.com/izone.mp4                          |
  |   |                                            |
  +-->|                                            |
      |                                            |
  +---|                                            |
  |   |                                            |
find K/V store: compare hashing result with the storage range
  |   |                                            |
  +-->|                                            |
      |                                            |
      |  interest: /storage04/{create}/{table}/a.com/izone.mp4
      |------------------------------------------->|
      |                                            |---+
      |                                            |   |
      |                                  create Key/Value Table
      |                                            |   |
      |                                            |<--+
      |                   200 OK                   |
      |<-------------------------------------------|
      |                                            |
      |  interest: /storage01/{info}/a.com/izone.mp4
      |<-------------------------------------------|
      |            response data: info             |
      |------------------------------------------->|
      |                                            |
      ~                                            ~
      |                                            |---+
      |                                            |   |
      |                             create manifest file
      |                     store info: who stored data (/storage01/contents)
      |                     segments info: segment start/end number
      |                                            |   |
      |                                            |<--+
      |                                            |
      ~                                            ~
      |                                            |---+
      |                                            |   |
      |                                 insert Key/Value
      |                     Key: hashing result (/a.com/izone.mp4)
      |                     Value: manifest file (already created)
      |                                            |   |
      |                                            |<--+
      |                                            |
```

- get
  - consumer send interest to DIFS storage
    - interest: /repo/{get}/a.com/izone.mp4
    - DIFS command: new command
    - based repo-ng command
  - requested DIFS storage hashes the content name
    - hashing: content name (/a.com/izone.mp4)
    - find DIFS K/V storage: hashing result (who has hash range)
    - send interest to K/V storage
      - interest: /{find storage name}/{fetch}/{hashing result}
      - DIFS command: new command
      - response: manifest (Key: hashing result, Value: manifest)
    - delivers manifest files to consumer
  - consumer received manifest file
    - who stored data (rename prefix)
    - number of segments
  - consumer send interest to DIFS file storage
    - interest: /storage01/contents/a.com/izone.mp4/#01
    
```
+----------+     +------------+     +------------+     +------------+
| consumer |     | /storage02 |     | /storage04 |     | /storage01 |
|          |     |            |     | K/VStorage |     |File Storage|
+----------+     +------------+     +------------+     +------------+
     |                  |                  |                  |
intestst: /repo/{get}/a.com/izone.mp4      |                  |
     |----------------->|                  |                  |
     |                  |---+              |                  |
     |                  |   |              |                  |
     |       hasing: /a.com/izone.mp4      |                  |
     |                  |   |              |                  |
     |                  |<--+              |                  |
     |                  |                  |                  |
     |    interest: /{find storage}/{fetch}/{hashing result}  |
     |                  |----------------->|                  |
     |                  |                  |                  |
     |           response: manifest (key: hashing result)     |
     |                  |<-----------------|                  |
    response: manifest (delivery)          |                  |
     |<-----------------|                  |                  |
     |                  |                  |                  |
     ~                  ~                  ~                  ~
     |                  |                  |                  |
 +---|                  |                  |                  |
 |   |                  |                  |                  |
receive manifest file                                         |
  know who stored data (rename prefix)                        |
  know number of segment                                      |
 |   |                  |                  |                  |
 +-->|                  |                  |                  |
     |                  |                  |                  |
     ~                  ~                  ~                  ~
     |                  |                  |                  |
intestst: /storage01/contents/a.com/izone.mp4/#01             |
     |------------------------------------------------------->|
     |                response data: segment#01               |
     |<-------------------------------------------------------|
     |                  |                  |                  |
     ~                  ~                  ~                  ~
     |                  |                  |                  |
intestst: /storage01/contents/a.com/izone.mp4/#end            |
     |------------------------------------------------------->|
     |                response data: segment#end              |
     |<-------------------------------------------------------|
     |                  |                  |                  |
```

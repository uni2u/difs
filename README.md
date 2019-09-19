## DIFS (Data Infra File System)

### Our Goal

> NDN based Object Storage

- segments are stored on file storage
- content is retrieved through key-value storage which file storage has been stored
- **DIFS = DIFS file storage + DIFS Key/Value storage**

#### Environment

- repo-ng base
  - https://redmine.named-data.net/projects/repo-ng/wiki

#### What's Different (DIFS/repo-ng)

|  | repo-ng | DIFS |
|---|:---|:---|
|storage|sqlite3 (DB)|object storage|
|stored type|segment|segment|
|...|...|...|

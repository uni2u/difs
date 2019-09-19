## DIFS (Data Infra File System)

### Our Goal

> NDN based Object Storage

- segments are stored on file storage
- content is retrieved through key-value storage which file storage has been stored
- '_DIFS = DIFS file storage + DIFS Key/Value storage_'

#### Environment

DIFS based on [repo-ng](https://github.com/named-data/repo-ng).

Unlike repo-ng using sqlite3, DIFS stores the manufacturer's data segments in the file system.

- repo-ng base
  - https://redmine.named-data.net/projects/repo-ng/wiki

#### What's Different (DIFS/repo-ng)

|  | repo-ng | DIFS |
|---|:---|:---|
|storage|sqlite3 (DB)|object storage|
|stored type|segment|segment|
|...|...|...|

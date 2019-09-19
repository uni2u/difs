## DIFS (Data Infra File System)

DIFS based on [repo-ng](https://github.com/named-data/repo-ng).

- repo-ng wiki
  - https://redmine.named-data.net/projects/repo-ng/wiki

Unlike repo-ng using sqlite3, DIFS stores the manufacturer's data segments in the file system.

**What's Different (DIFS/repo-ng)**

|  | repo-ng | DIFS |
|---|:---|:---|
|storage|sqlite3 (DB)|object storage|
|stored type|segment|segment|
|...|...|...|

### Our Goal

> NDN based Object Storage

- segments are stored on file storage
- content is retrieved through key-value storage which file storage has been stored
- '_DIFS = DIFS file storage + DIFS Key/Value storage_'

### Environment

Unlike repo-ng using sqlite3, DIFS stores the manufacturer's data segments in the file system.
DIFS can be divided into file storage and Key-Value storage.

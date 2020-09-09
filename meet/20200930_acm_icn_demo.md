---
title: Chipmunk: Distributed Object Storage for NDN
subtitle: ACM ICN 2020 DEMO
author: 
presentedAt: 2020-09-30
---

# CHIPMUNK GOAL
## easy to use
- producers wants persistant data store
  - they don't know exact store prefix
  - store the data using simple prefix
- consumers wants storage that is simple to use
  - they don't not know exactly who is storing data
## object storage
- store data in network
- name and data should be mapped
- it should be available regardless of network location
## object storage for NDN
- file storage
  - storing actual data segment
- key/value store
  - manage name and data mapping information

---

# CHIPMUNK DESIGN
## design
- object storage based on **_repo-ng_**
- node consists of file storage and key/value store
  - file storage stores segments of data
  - key/value store stores metadata

![node](https://github.com/uni2u/difs/blob/master/img/node.png?raw=true)

## common prefix
- inherits the command-set of **_repo-ng_**
- each node has a common prefix
  - `data prefix <ndn:/chipmunk/data/>`
  - `cluster prefix <ndn:/chipmunk>`

---

# FILE STORAGE
## directory
- using ext4 file system
  - improved performance through
    - multi-block allocation, extent-based block mapping, delayed allocation, stripe-aware allocation
  - unlimited number of sub-directories
- sub directory (like git)
  - store NDN-packets received from producers
  - objects are placed over 256 subdirectories using the first two characters of the SHA1 `data-name` to keep the number of directory entries in objects itself to a manageable number
  - directory stores files that store many objects in compressed form, along with index files to allow them to be randomly accessed

## file storage structure

![file storage](https://github.com/uni2u/difs/blob/master/img/directory.png?raw=true)

---

# KEY/VALUE STORE
## key and value
- key is the hash result of `ndn-name`
  - same `ndn-name` has the same hashing result
- value is the metadata that mapping to the hash result of `ndn-name`
  - metadata includes data information and node information that stores actual data

## metadata
- large scale network support
  - FIB table capacity issue
  - NDNS sync issue (all NDNS servers must be synchronized)
- network flexibility, scalability

## metadata store structure

![key/value store](https://github.com/uni2u/difs/blob/master/img/metadata.png?raw=true)

---

# HASH RING
## consistent hashing
- find the node where the key is stored without looking up metadata
- use hash results to find keys, find nodes
- hash results are aligned in the virtual ring, and each node handles only its own scope in the ring
- phase2) if a node is added, divide the scope that a particular node (a node with a lot of data) was responsible for and assign it to a new node
- phase2) if a node is deleted, assign the range it was responsible for to the adjacent node
- phase2) minimize the number of nodes affected by add/delete of nodes during service

## ring topology
![hash ring](https://github.com/uni2u/difs/blob/master/img/hashring02.png?raw=true)

---

# ENVIRONMENT
## object storage network
- each node connected by NDN
- using repo-ng tools
  - `ndnputfile <common name> <ndn-name> <file>`
  - `ndngetfile <ndn-name>`
  - `ndndelfile <common name> <ndn-name>`

![topology](https://github.com/uni2u/difs/blob/master/img/topology.png?raw=true)

---

# DEMO1
## insert

![insert](https://github.com/uni2u/difs/blob/master/img/insert02.png?raw=true)

## retrieve/delete

![retrieve](https://github.com/uni2u/difs/blob/master/img/retrieve02.png?raw=true)

---

# DEMO2
## insert

![insert](https://github.com/uni2u/difs/blob/master/img/insert03.png?raw=true)

## retrieve/delete

![retrieve](https://github.com/uni2u/difs/blob/master/img/retrieve03.png?raw=true)

---

# NOW CHIPMUNK
## phase1
- _basic model (this demo)_
## ▶ phase2 (progress)
### more like NDN
- forwarding hint
### new signature model
- hash-chain
### use container & NFN
- storing function data
- sharing host volume
### self config (consistent hashing)
- auto configuration (all nodes)
- dynamic storage node _add/delete_
## phase3
### performance
- increases performance in indicators such as speed and capacity
- apply to NDN testbed
### opensource contribute

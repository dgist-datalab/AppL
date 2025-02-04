# AppL


## What is AppL?

AppL is s a novel approximate index structure for flash based SSD, which combines memory-efficient approximate indices
and an LSM-tree.

The original paper that introduced AppL is currently in the revision stage of [Eurosys 25](https://2025.eurosys.org/).


## Real SSD prototype

To measure I/O performance, you need special hardware (Xilinx VCU108+Customized NAND Flash).

However, you can evaluate memory efficiency (i.e., cache efficiency) and WAF of AppL and other FTLs
without the specific hardware using memory (RAM drive).

## Directories and files
This section introduces the main directories of AppL and configuration files.

### Main directories

* `./algorithm/`: This direcotry includes FTL algorithms including AppL.
  
  * `./algorithm/Page_ftl/`: It contains source codes for **OFTL** which keeps all indices into its DRAM.
    
  * `./algorithm/DFTL/`: It contains source code for **DFTL** and DFTL-like algorithms, such as **SFTL**.
    
    * `./algorithm/DFTL/caching/coarse`: Original **DFTL** cache algorithm
      
    * `./algorithm/DFTL/caching/sftl`: **SFTL** cache algorithm
      
  * `./algorithm/leaFTl/`: It contains source codes for **LeaFTL**
    
  * `./algorithm/layeredLSM/`: It contains source codes for **AppL**
    
    * `./algorithm/layeredLSM/translation_functions/bf_guard_mapping*`: It contains source codes for fingerpring (FP) indexing
      
    * `./algorithm/layeredLSM/translation_functions/plr*` : It contains source codes for piece-wise linear regression (PLR) indexing


* `./lower/`: This direcotry includes source codes for storage media, such as RAM drive (emulation) and Real SSD prototype
 
  * `./lower/AMF/`: It contains source codes for **Real SSD prototype**

  * `./lower/posix_memory/`: It contains source codes for **RAM drive**
 
  * `./lower/linux_aio/` : It contains source codes for **Commercial SSDs**
  
### Configuration files

## Testing guidelines

### Prerequisites

The hardware/software requirements for executing MiDAS are as follows.


* `DRAM`: Larger than the device size of trace files + an extra 10% of the device size for the data structures and OP region to test the trace files. For example, you need 140GB size of DRAM to run the trace file with a 128GB device size.


### Installation

* Clone the required repository (MiDAS SSD prototype) into your host machine.

```
$ git clone https://github.com/dgist-datalab/MiDAS.git
$ cd MiDAS
```

### Execution

When you want to test MiDAS, please follow the instructions below.
In our testing environment, the following steps finished in 650 ~ 680 seconds.
If you want to test MiDAS on the simulation environment for fast evaluation, go to the following link: https://github.com/sungkyun123/MiDAS-Simulation

```
$ sudo apt install git build-essential
$ git clone https://github.com/dgist-datalab/MiDAS.git
$ cd MiDAS
$ git submodule update --init --recursive
$ wget https://zenodo.org/record/10409599/files/test-fio-small # trace file
$ make clean; make GIGAUNIT=8L _PPS=128 -j # Device side = 8GB, PPS = Pages Per Segment
$ ./midas test-fio-small
``` 

### Results

During the experiment, you can see that MiDAS adaptably changes the group configuration.


* UID information : Upon running MiDAS, the parameters of UID will be displayed at the beginning.
We sample a subset of LBA for timestamp monitoring with a sampling rate of 0.01.
We use a coarse-grained update interval unit of 16K blocks and epoch lengths of 4x of the storage capacity (8GB).

The following result is from an example run:

```
Storage Capacity: 8GiB  (LBA NUMBER: 1953125)
*** Update Interval Distribution SETTINGS ***
- LBA sampling rate: 0.01
- UID interval unit size: 1 segment (1024 pages)
- Epoch size: 32.00GB (2048 units)
```


* Throughput information

Throughput is calculated per 100GB write requests.

```
[THROUGHPUT] 347 MB/sec (current progress: 300GB)
[THROUGHPUT] 246 MB/sec (current progress: 400GB)
```


* You can see the group configuration and valid ratio of the groups per 1x write request of the storage capacity.

```
[progress: 896GB]
TOTAL WAF:	1.852, TMP WAF:	1.640
  GROUP 0[4]: 0.0000 (ERASE:287)
  GROUP 1[51]: 0.7653 (ERASE:224)
  GROUP 2[222]: 0.5629 (ERASE:102)
  GROUP 3[28]: 0.4636 (ERASE:30)
  GROUP 4[12]: 0.6019 (ERASE:2)
  GROUP 5[190]: 0.4284 (ERASE:195)

```


* When an epoch is over, the GCS algorithm finds the best group configuration using UID and MCAM. The group configuration is shown as follows.

```

*****MODEL PREDICTION RESULTS*****
group number: 6
*group 0: size 4, valid ratio 0.000000
*group 1: size 51, valid ratio 0.734829
*group 2: size 222, valid ratio 0.350166
*group 3: size 28, valid ratio 0.413656
*group 4: size 12, valid ratio 0.734003
*group 5: size 191, valid ratio 0.781421
calculated WAF: 1.626148
calculated Traffic: 0.570
************************************
```


* MiDAS periodically checks the irregular pattern of the workload. If there is a group whose valid ratio prediction is wrong, MiDAS gives up on adjusting group sizes for all groups beyond the group and simply merges the groups.

```
==========ERR check==========
[GROUP 0] calc vr: 0.000, real vr: 0.000	(O)
[GROUP 1] calc vr: 0.735, real vr: 0.765	(O)
[GROUP 2] calc vr: 0.350, real vr: 0.559	(X)
!!!IRREGULAR GROUP: 2!!!
=> MERGE GROUP : G2 ~ G5
=> NAIVE ON!!!!
===============================
```


* When the execution is over, You can check the total runtime, read traffic, write traffic, and WAF of the MiDAS.
```
runtime: 950 sec
Total Read Traffic : 23531283
Total Write Traffic: 31225469

Total WAF: 2.52

TRIM 239862
DATAR 0
DATAW 12384205
GCDR 23531283
GCDW 18841264
```

# AppL


## What is AppL?

AppL is a novel approximate index structure for flash based SSD, which combines memory-efficient approximate indices
and an LSM-tree.

The original paper that introduced AppL is currently in the revision stage of [ACM/SIGOPS EuroSys 2025](https://2025.eurosys.org/).


## Real SSD prototype

To measure I/O performance, you need special hardware (Xilinx VCU108+Customized NAND Flash).

However, you can evaluate memory efficiency (i.e., cache efficiency) and WAF of AppL and other FTLs
without the specific hardware using memory (RAM drive).

## Directories and files
This section introduces the main directories of AppL and configuration files.

### Main directories

* `./algorithm/`: This directory includes FTL algorithms including AppL.
  
  * `./algorithm/Page_ftl/`: It contains source codes for **OFTL** which keeps all indices into its DRAM.
    
  * `./algorithm/DFTL/`: It contains source code for **DFTL** and DFTL-like algorithms, such as **SFTL**.
    
    * `./algorithm/DFTL/caching/coarse`: Original **DFTL** cache algorithm
      
    * `./algorithm/DFTL/caching/sftl`: **SFTL** cache algorithm
      
  * `./algorithm/leaFTl/`: It contains source codes for **LeaFTL**
    
  * `./algorithm/layeredLSM/`: It contains source codes for **AppL**
    
    * `./algorithm/layeredLSM/translation_functions/bf_guard_mapping*`: It contains source codes for fingerprint (FP) indexing
      
    * `./algorithm/layeredLSM/translation_functions/plr*` : It contains source codes for piece-wise linear regression (PLR) indexing


* `./lower/`: This directory includes source codes for storage media, such as RAM drive (emulation) and Real SSD prototype
 
  * `./lower/AMF/`: It contains source codes for **Real SSD prototype**

  * `./lower/posix_memory/`: It contains source codes for **RAM drive**
 
  * `./lower/linux_aio/` : It contains source codes for **Commercial SSDs**
  
### Configuration files

* `./include/settings.h`: It contains configuration for storage media such as the storage space and the emulation mode

```
...
#define GIGAUNIT 32LL // 32GB storage space
...
#define COPYMETA_ONLY 10 // low-memory mode, if you comment out this line, the emulation becomes the normal mode.
...
```

* `Makefile`: You can select which algorithms and lowers to run in Makefile.
```
...
export TARGET_LOWER=posix_memory // Emulation mode. For others, replace 'posix_memory' to directory name of under './lower/' 
export TARGET_ALGO=layeredLSM // AppL. For others, replace 'posix_memory' to directory name of under './algorithm/'
...
```

* `interface/vectored_main.c`: It is the main file for the emulation

## Testing guidelines

The hardware/software requirements for executing AppL are as follows along with execution guidelines.

### Prerequisites

* `DRAM`:
  * Normal emulation: Requires DRAM larger than the device size plus additional memory for index caching. For example, to emulate a device with 128GB storage and allocate 20% of the total index size for caching, you would need 128GB + 256MB of DRAM.
  * Low-memory emulation (default): Requires DRAM larger than 10% of the device size plus additional memory for index caching. For example, to emulate a device with 128GB storage under low-memory conditions while allocating 20% of the total index size for caching, you would need 12.8GB + 256MB of DRAM.


### Installation

Clone this repository into your host machine.

```
$ git clone https://github.com/dgist-datalab/AppL.git
$ cd AppL
```

### Execution


When you want to test AppL, please follow the instructions below.

```
$ sudo apt install git build-essential
$ git clone https://github.com/dgist-datalab/AppL.git
$ cd AppL
$ make driver -j 4
```

To compile all FTLs, we provide a bash script, compile_all_ftls.sh.
```
$ ./compile_all_ftls.sh
...
$ ls ./result/
appl_driver  dftl_driver  leaftl_driver  oftl_driver  sftl_driver
```

To run a simple benchmark (random write and random read), we provide a bash script, test_all_ftls.sh
```
$ ./test_all_ftls.sh 
Drivers found: ./result/appl_driver ./result/dftl_driver ./result/leaftl_driver ./result/oftl_driver ./result/sftl_driver
Running: ./result/appl_driver -T 0 -p 20 > ./result/appl_driver_res.txt 2>&1
Done: ./result/appl_driver
Running: ./result/dftl_driver -T 0 -p 20 -t 0 > ./result/dftl_driver_res.txt 2>&1
Done: ./result/dftl_driver
Running: ./result/leaftl_driver -T 0 -p 20 > ./result/leaftl_driver_res.txt 2>&1
Done: ./result/leaftl_driver
Running: ./result/oftl_driver -T 0 > ./result/oftl_driver_res.txt 2>&1
Done: ./result/oftl_driver
Running: ./result/sftl_driver -T 0 -p 20 -t 2 > ./result/sftl_driver_res.txt 2>&1
Done: ./result/sftl_driver
Please check the results:
./result/appl_driver_res.txt  ./result/dftl_driver_res.txt  ./result/leaftl_driver_res.txt  ./result/oftl_driver_res.txt  ./result/sftl_driver_res.txt

```
You can adjust the cache size using the `-p` parameter. For example, `-p 20` sets the cache size to 20% of the total index size.

The `-T` parameter is an option for target benchmarks.
* 0: random write and random read
* 1: sequential write and sequential read

### Results
For the emulation, you can check the two main results, one is the cache miss rate in read requests and the other is I/O traffic.

#### Cache miss rate
The section '[Read page information]' shows the detailed information of read requests
```
[Read page information]
a_type	BH	l_type	max	min	avg	cnt	percentage
```
  - `a_type` : Represents algorithm-specific types, with values varying depending on the FTL scheme (details to follow).
  - `BH`: Indicates the number of requests that hit the buffer.
  - `l_type`: Describes the state of the storage media when requests are processed in flash memory (0 is ok).
  - `max` : Represents the maximum time (in microseconds) for each category.
  - `min` : Represents the minimum time (in microseconds) for each category.
  - `cnt` : Denotes the number of requests belonging to each category.
  - `percentage`: Indicates the proportion of each category relative to the total number of requests.

##### Details of a_type

**`AppL`**
```
[Read page information]
a_type	BH	l_type	max	min	avg	cnt	percentage
10	0	0	17	0	0.526	48556	0.57884%
20	0	0	20	0	1.114	119413	1.42352%
21	0	0	18	0	1.438	12221	0.14569%
22	0	0	8	1	1.741	826	0.00985%
23	0	0	3	1	2.119	42	0.00050%
30	0	0	38	0	1.227	6406802	76.37562%
31	0	0	34	0	1.430	803400	9.57735%
220	0	0	20	2	3.207	12097	0.14421%
221	0	0	19	2	3.645	1257	0.01498%
222	0	0	7	3	3.817	82	0.00098%
223	0	0	9	3	5.000	4	0.00005%
230	0	0	38	1	2.713	874522	10.42519%
231	0	0	33	1	2.933	109322	1.30323%
```
Ones place represents the number of approximation errors occurring in the read path.

Tens place in a_type indicates the type of indexing scheme used
  * `1` : RB-tree
  * `2` : Fingerprint (FP)
  * `3` : Piece-wise linear regression (PLR)
    
Hundreds place in a_type specifies the type of cache miss
  * `1` : Shortcut table miss
  * `2` : Index miss
  * `3` : Both

For example, a_type 231 represents the following:

- Hundreds place (2): The request encountered an FP index miss, meaning a cache miss occurred at the fingerprint index level.

- Tens place (3): The request uses Piece-wise Linear Regression (PLR) as the indexing scheme.

- Ones place (1): There is one approximation error, requiring an additional flash read.

As a result, the request requires:

- One flash read to access the index (due to the PLR index miss).

- One flash read to correct the approximation error.

- One flash read to retrieve the desired data.

Thus, a total of three I/O operations are needed.

The hit ratio—**defined as the percentage of requests requiring only one flash access**—is 78.38%. 

This value is obtained by summing the requests for a_types 10, 20, and 30.

**`DFTL` and `SFTL`**
```
[Read page information]
a_type	BH	l_type	max	min	avg	cnt	percentage
0	0	0	19	0	0.461	1128120	13.44834%
0	1	0	17	0	0.315	542655	6.46900%
1	0	0	59	0	1.382	4540687	54.12962%
1	1	0	49	0	1.237	2176674	25.94817%
3	0	0	16	1	3.376	380	0.00453%
3	1	0	17	2	3.500	28	0.00033%
```

- a_type 0 : No cache miss.
- a_type 1 : Cache miss with clean eviction.
- a_type 3 : Cache miss with dirty eviction.
- a_type 7 : Cache miss with dirty eviction, where garbage collection (GC) occurs for the dirty eviction.

The hit ratio for this result is 19.91734%, which is calculated as the sum of the requests with a_type '0'.

**`LeaFTL`**
```
[Read page information]
a_type	BH	l_type	max	min	avg	cnt	percentage
0	0	0	23	0	1.560	897979	10.70483%
0	1	0	17	1	1.352	5123	0.06107%
1	0	0	74	1	2.619	7442819	88.72600%
1	1	0	37	1	2.665	37463	0.44660%
10	0	0	19	1	2.481	2492	0.02971%
10	1	0	3	1	2.179	28	0.00033%
11	0	0	20	2	3.906	2634	0.03140%
11	1	0	5	3	4.000	6	0.00007%
```
Ones place in a_type indicates the cache miss

Tens place represents the number of approximation errors occurring in the read path.

The hit ratio for this result is 10.7659%, calculated as the sum of the requests with a_type '0'.


**`OFTL`**
```
[Read page information]
a_type	BH	l_type	max	min	avg	cnt	percentage
0	0	0	17	0	0.306	5669187	67.58250%
0	1	0	16	0	0.159	2719357	32.41751%
```
a_type is always 0

The hit ratio of this result is always 100% because OFTL contains all indices in its DRAM memory.

#### Traffic results

```
ERASE 494
MAPPINGR 1015560
MAPPINGW 12321
GCMR 0
GCMW 0
DATAR 8388544
DATAW 2097136
GCDR 0
GCDW 0
GCMR_DGC 0
GCMW_DGC 0
COMPACTIONDATA-R 16276751
COMPACTIONDATA-W 4201763
MISSDATAR 928154
...
WAF: 3.009447
```

- Erase: Number of flash block erase requests.
- MAPPINGR: Number of flash read requests for retrieving indexing data.
- MAPPINGW: Number of flash write requests for storing indexing data.
- GCMR: Number of flash read requests for GC (garbage collection) in indexing data.
- GCMW: Number of flash write requests for GC in indexing data.
- DATAR: Number of flash read requests for retrieving the desired user data.
- DATAW: Number of flash write requests for storing user data.
- GCDR: Number of flash read requests for GC in user data.
- GCDW: Number of flash write requests for GC in user data.
- GCMR_DGC: Number of flash read requests for indexing data during GC of user data.
- GCMW_DGC: Number of flash write requests for indexing data during GC of user data.
- COMPACTIONDATA-R: Number of flash read requests for compaction.
- COMPACTIONDATA-W: Number of flash write requests for compaction.
- MISSDATAR: Number of flash read requests for collecting approximation errors.

WAF means the write amplification factor.


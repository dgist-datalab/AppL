#override export CC=g++-9
#override export CXX=g++-9
#override export AR=gcc-ar
#override export NM=gcc-nm

export CC=g++
export CXX=g++

TARGET_INF=interface
export TARGET_LOWER=posix_memory
export TARGET_ALGO=leaFTL
export TARGET_BM=sequential
JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export USER_DEF


DEBUGFLAGS=\
			-Wno-pointer-arith\
			-g\
			-export-dynamic\


export COMMONFLAGS+=\
			-Wno-write-strings\
			-Wno-sign-compare\
			-Wno-unused-function\
			-DLARGEFILE64_SOURCE\
			-D_GNU_SOURCE\
			-DSLC\
			-D$(TARGET_BM)\
			-Wno-unused-but-set-variable\
			-DPROGRESS\
			-DAPPL_DESIGN_PATH="\"$(PWD)/algorithm/layeredLSM/design_knob/\""\
			-DNO_MEMCPY_DATA\
			-O3\




COMMONFLAGS+=$(DEBUGFLAGS)\

COMMONFLAGS+=$(USER_DEF)\

export CFLAGS_ALGO=\
			 -fPIC\
			 -Wall\
			 -D$(TARGET_LOWER)\

export CFLAGS_LOWER=\
		     -fPIC\
			 -lpthread\
			 -Wall\
			 -D_FILE_OFFSET_BITS=64\

export ORIGINAL_PATH=$(PPWD)


#CFLAGS_ALGO+=-DCOMPACTIONLOG\
	
CFLAGS_ALGO+=$(COMMONFLAGS)\
			 -D$(TARGET_ALGO)\

CFLAGS_LOWER+=$(COMMONFLAGS)\
			  -D$(TARGET_ALGO)\

ifeq ($(TARGET_ALGO), lsmtree)
 CFLAGS_ALGO+=-DLSM_SKIP
endif

ifeq ($(CC), gcc)
 CFLAGS_ALGO+=-Wno-discarded-qualifiers -std=c99
 CFLAGS_LOWER+=-Wno-discarded-qualifiers -std=c99
else
 CFLAGS_ALGO+= -std=c++11
 CFLAGS_LOWER+= -std=c++11
endif

CFLAGS +=\
		 $(CFLAGS_ALGO)\
		 -D$(TARGET_LOWER)\
		 -D$(TARGET_ALGO)\
		 -D$(TARGET_INF)\
		 -D_DEFAULT_SOURCE\
		 -D_BSD_SOURCE\
-DBENCH\
-DCDF\

SRCS +=\
	./interface/queue.c\
	./interface/interface.c\
	./interface/vectored_interface.c\
	./interface/global_write_buffer.c\
	./include/FS.c\
	./include/slab.c\
	./include/utils/debug_tools.c\
	./include/utils/dl_sync.c\
	./include/utils/rwlock.c\
	./include/utils/cond_lock.c\
	./include/utils/data_checker.c\
	./include/utils/crc32.c\
	./include/data_struct/hash_kv.c\
	./include/data_struct/list.c\
	./include/data_struct/redblack.c\
	./include/data_struct/heap.c\
	./include/data_struct/lru_list.c\
	./include/data_struct/partitioned_slab.c\
	./bench/measurement.c\
	./bench/bench.c\
	./bench/bench_demand.c\
	./bench/bench_transaction.c\
	./bench/bench_localized_workload.c\
	./bench/bench_parsing.c\
	./include/utils/thpool.c\
	./include/utils/kvssd.c\
	./include/utils/sha256.c\
	./include/utils/tag_q.c\
	./blockmanager/bb_checker.c\
	./blockmanager/block_manager_master.c\
	./parallel_unit_manager/pu_manager.c\


TARGETOBJ =\
			$(patsubst %.c,%.o,$(SRCS))\

MEMORYOBJ =\
		   	$(patsubst %.c,%_mem.o,$(SRCS))\

DEBUGOBJ =\
		   	$(patsubst %.c,%_d.o,$(SRCS))\


ifeq ($(TARGET_LOWER),AMF)
	ARCH +=./object/libAmfManager.a
endif

LIBS +=\
		-lpthread\
		-lm\

all: driver

driver: ./interface/vectored_main.c libdriver.a libart.o
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)

cheeze_block_driver: ./interface/cheeze_hg_block.c ./interface/mainfiles/cheeze_block_main.c libdriver.a libart.o
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)

libart.o:
	make -C ./include/data_struct/libart/
	mv ./include/data_struct/libart/src/libart.o ./


libdriver.a: $(TARGETOBJ)
	mkdir -p object && mkdir -p data
	cd ./blockmanager/$(TARGET_BM) && $(MAKE) && cd ../../
	cd ./algorithm/$(TARGET_ALGO) && $(MAKE) clean && $(MAKE) && cd ../../
	cd ./lower/$(TARGET_LOWER) && $(MAKE) && cd ../../
	mv ./parallel_unit_manager/*.o ./object/
	mv ./include/data_struct/*.o ./object/
	mv -f ./blockmanager/*.o ./object/
	mv ./include/utils/*.o ./object/
	mv ./interface/*.o ./object/ && mv ./bench/*.o ./object/ && mv ./include/*.o ./object/
	$(AR) r $(@) ./object/*

%_mem.o: %.c
	$(CC) $(CFLAGS) -DLEAKCHECK -c $< -o $@ $(LIBS)

%_d.o: %.c
	$(CC) $(CFLAGS) -DDEBUG -c $< -o $@ $(LIBS)

.c.o :
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

submodule:libdriver.a
	mv libdriver.a ../objects/

clean :
	cd ./algorithm/$(TARGET_ALGO) && $(MAKE) clean && cd ../../
	cd ./lower/$(TARGET_LOWER) && $(MAKE) clean && cd ../../
	@$(RM) ./data/*
	@$(RM) ./object/*.o
	@$(RM) ./object/*.a
	@$(RM) ./libart.o
	@$(RM) *.a
	@$(RM) driver_memory_check
	@$(RM) debug_driver
	@$(RM) duma_driver
	@$(RM) range_driver
	@$(RM) *driver
	@$(RM) bd_testcase
	@$(RM) libdriver.so


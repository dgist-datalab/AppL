#include "page_cache.h"
#include "../../interface/interface.h"
#include "cache_layer.h"
#include <list>
#include <pthread.h>
//pthread_mutex_t idx_map_lock=PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t req_wait_lock=PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t lru_lock=PTHREAD_MUTEX_INITIALIZER;
static inline void print_error(uint64_t line){
    printf("%s error :%u\n", __FILE__, line);
}

void pc_set_init(pc_set *target, uint32_t max_cached_size, uint32_t lba_num, lower_info *li, bool sc_pinning){
    lru_init(&target->lru, NULL, NULL);
    target->now_cached_size=0;
    target->max_cached_size=max_cached_size;
    //target->pinned_size=0;
    target->cached_sc_num=0;
    target->cached_idx_num=0;

    target->cached_idx.clear();
    target->cached_sc.clear();
    target->cached_sc.reserve(lba_num/RIDXINPAGE+1);
    page_cache temp_pc;
    temp_pc.type=SHORTCUT;
    temp_pc.flag=EMPTY;
    temp_pc.ppa=UINT32_MAX;
    temp_pc.size=PAGESIZE;
 //  temp_pc.ispinned=false;
 //  temp_pc.refer_cnt=0;
    temp_pc.data=NULL;
    temp_pc.node=NULL;
    temp_pc.pba=UINT32_MAX;

    for(uint32_t i=0; i<lba_num/RIDXINPAGE+1; i++){
        temp_pc.scidx=i;
        target->cached_sc.push_back(temp_pc);
    }

    target->li=li;
    target->pending_req_map.clear();
    target->pending_sc_req_map.clear();
    target->sc_pinning=sc_pinning;
}

void lru_checker(LRU *lru){
    return ;
    lru_node *now;
    for_each_lru_list(lru, now){
        page_cache *pc=(page_cache*)now->data;
        if(pc->type!=SHORTCUT && pc->type!=IDX){
            print_error(__LINE__);
            abort();
        }
    }
}

void pc_set_free(pc_set *target){
    lru_free(target->lru);

    std::unordered_map<uint32_t, page_cache*>::iterator miter;
    for(miter=target->cached_idx.begin(); miter!=target->cached_idx.end(); miter++){
        free(miter->second->data);
        delete(miter->second);
    }
    target->cached_sc.clear();
    target->cached_idx.clear();
}

page_cache* pc_is_cached(pc_set *target, cache_type type, uint32_t ppa_or_scidx, bool pinned){
    page_cache *pc=NULL;
    //pthread_mutex_lock(&lru_lock);
    
    if(type==SHORTCUT){
        pc=&target->cached_sc[ppa_or_scidx];
        if(pc->flag==EMPTY){
            pc=NULL;
        }
    }
    else if(type==IDX){
        std::unordered_map<uint32_t, page_cache*>::iterator miter;
        miter=target->cached_idx.find(ppa_or_scidx);
        if(miter==target->cached_idx.end()){
            pc=NULL;
        }
        else{
            pc=miter->second;
            if(pc->flag==EMPTY){
                pc=NULL;
            }
        }
    }
    else{
        print_error(__LINE__);
        abort();
    }

    if(pc && pc->flag!=FLYING && pc->flag!=OCCUPIED){
        lru_update(target->lru, pc->node);
        lru_checker(target->lru);
        //if(pinned){
        //    pthread_mutex_lock(&pinning_lock);
        //    pc->ispinned=true;
        //    if(pc->refer_cnt==0){
        //        target->pinned_size+=pc->size;
        //    }
        //    pc->refer_cnt++;
        //    pthread_mutex_unlock(&pinning_lock);
        //}
    }
    //pthread_mutex_unlock(&lru_lock);
    return pc;
}

bool pc_has_space(pc_set *target, uint32_t need_size){
    //pthread_mutex_lock(&lru_lock);
    bool res=target->now_cached_size + need_size < target->max_cached_size;
    //pthread_mutex_unlock(&lru_lock);

    return res;
}


page_cache* pc_occupy(pc_set *target, cache_type type, uint32_t ppa_or_scidx, uint32_t size){
    page_cache *pc=NULL;
    //pthread_mutex_lock(&lru_lock);
    if(target->now_cached_size + size > target->max_cached_size){
        //print_error(__LINE__);
        //abort();
    }


    if(type==SHORTCUT){
        pc = &target->cached_sc[ppa_or_scidx];
        if(pc->flag!=EMPTY){
            print_error(__LINE__);
            abort();
        }
        pc->flag=EMPTY;
        pc->type=type;
		pc->size=size;
        pc->waiting_req.clear();
        target->cached_sc_num++;
    }
    else if(type==IDX){

        if(target->cached_idx_num!=target->cached_idx.size()){
            printf("errror %s:%u\n",__FILE__, __LINE__);
            abort();
        }
        std::unordered_map<uint32_t, page_cache*>::iterator miter;
        miter=target->cached_idx.find(ppa_or_scidx);
        if(miter!=target->cached_idx.end()){
            pc=miter->second; 
        }
        else{
            pc = new page_cache();
            pc->waiting_req.clear();
            target->cached_idx.insert(std::make_pair(ppa_or_scidx, pc));
            pc->flag = EMPTY;
        }
        pc->type = type;
        pc->size = size;
        pc->ppa=ppa_or_scidx;

        target->cached_idx_num=target->cached_idx.size();

        if(target->cached_idx_num!=target->cached_idx.size()){
            printf("errror %s:%u\n",__FILE__, __LINE__);
            abort();
        }
    }
    else{

        print_error(__LINE__);
        abort();
    }

    pc->flag=OCCUPIED;
    //pc->data=(char*)malloc(size);
    //pc->ispinned = true;
    //pc->refer_cnt=1;
    pc->node=NULL;

    //target->pinned_size+=size;
    //pthread_mutex_unlock(&lru_lock);
    return pc;
}

void pc_reclaim(pc_set *pcs, page_cache *pc){
    //pthread_mutex_lock(&lru_lock);
    if(pc->type!=SHORTCUT){
        print_error(__LINE__);
        abort();
    }

    pc->flag=EMPTY;
    free(pc->data);
    //pc->ispinned=false;
    //pc->refer_cnt=0;

    //pcs->pinned_size-=pc->size;
    pcs->now_cached_size-=pc->size;
    pcs->cached_sc_num--;
    //pthread_mutex_unlock(&lru_lock);
}

void pc_set_insert(pc_set *target, cache_type type, uint32_t ppa_or_scidx, void *data, void (*converter)(void *data, page_cache *pc), uint32_t (*get_ppa)(uint32_t sc_idx, page_cache *pc)){
    page_cache *pc=NULL;

    if(type==SHORTCUT){
        pc=&target->cached_sc[ppa_or_scidx];
    }
    else if(type==IDX){
        std::unordered_map<uint32_t, page_cache*>::iterator miter;
        miter=target->cached_idx.find(ppa_or_scidx);
        if(miter==target->cached_idx.end()){
            print_error(__LINE__);
            abort();
        }
        pc=miter->second;
        pc->pba=ppa_or_scidx;
        pc->ppa=ppa_or_scidx;
    }
    else{
        print_error(__LINE__);
        abort();
    }

    if(converter){
        converter(data, pc);
    }
    else{
        //memcpy(pc->data, data, pc->size);
    }

    target->now_cached_size+=pc->size;

    //pthread_mutex_lock(&lru_lock);
    pc->node=lru_push_special(target->lru, pc, type, ppa_or_scidx, pc->size);
    if(type==IDX && (ppa_or_scidx==16223878 || ppa_or_scidx==16238486)){
        static int cnt=0;
        printf("tttt %u\n", ++cnt);
    }
    lru_checker(target->lru);
    //pthread_mutex_unlock(&lru_lock);
    pc->waiting_req.clear();
    pc->flag=CLEAN;

    if(target->now_cached_size > target->max_cached_size){
        static int cache_max_size=0;
        if(cache_max_size < target->now_cached_size){
            cache_max_size=target->now_cached_size;
        }
      //  printf("over cached!\n %u:%u  -- (cached_max_size: %u)\n" ,target->now_cached_size, target->max_cached_size, cache_max_size);
        pc_evict(target, true, 0, get_ppa);
    }
}

void pc_set_update(pc_set *target, uint32_t ppa_or_scidx){
    page_cache *pc=&target->cached_sc[ppa_or_scidx];
    /*update data*/
    pc->flag=DIRTY;
}

void pc_unpin_target(pc_set *pcs, page_cache *pc){
    //if(!pc->ispinned){
    //    printf("error!\n");
    //    abort();
    //}
    //if(pc->ispinned){
    //    pthread_mutex_lock(&pinning_lock);
    //    pc->refer_cnt--;
    //    if (pc->refer_cnt == 0){
    //        pc->ispinned = false;
    //        pcs->pinned_size-= pc->size;
    //    }
    //    pthread_mutex_unlock(&pinning_lock);
    //}
}

void pc_unpin(pc_set *target, cache_type type, uint32_t ppa_or_scidx){
    //page_cache *pc;
    //pthread_mutex_lock(&idx_map_lock);
    //if(type==SHORTCUT){
    //    pc=&target->cached_sc[ppa_or_scidx];
    //}
    //else if(type==IDX){
    //    pc=target->cached_idx[ppa_or_scidx];
    //}
    //else{
    //    printf("error!\n");
    //    abort();
    //}
    //pthread_mutex_unlock(&idx_map_lock);
    //pc_unpin_target(target, pc);
}

static void *__write_end_req(algo_req *req){
    inf_free_valueset(req->value, FS_MALLOC_W);
    free(req);
    return NULL;
}

static void __send_write_reqeust(pc_set *target, page_cache *pc, uint32_t new_ppa){
    algo_req *res=(algo_req*)calloc(1, sizeof(algo_req));
    res->parents=NULL;
    res->ppa=new_ppa;
    res->type=MAPPINGW;//??
    res->end_req=__write_end_req;
    res->value=inf_get_valueset(NULL, FS_MALLOC_W, PAGESIZE);
    //memcpy(res->value->value, pc->data, PAGESIZE);
    res->value->ppa=new_ppa;
    target->li->write(new_ppa, PAGESIZE, res->value, res);
}

void pc_evict(pc_set *target, bool internal, int32_t need_size, uint32_t (*get_ppa)(uint32_t sc_idx, page_cache *pc)){
    int32_t remain_size=target->max_cached_size-target->now_cached_size;
    if(remain_size > need_size) return;
    //pthread_mutex_lock(&lru_lock);
    int32_t target_size=need_size-remain_size;
    page_cache *pc=NULL;
    uint32_t type;
    uint32_t id;
    uint32_t size;
    while(target_size > 0){
        pc=NULL;
        //page_cache *pc=(page_cache*)lru_pop(target->lru);
        page_cache *pc=(page_cache*)lru_pop_special(target->lru, &type, &id, &size);
        lru_checker(target->lru);
        if(pc==NULL){
            printf("all cached entries are pinned %s:%d\n", __FUNCTION__, __LINE__);
            //abort();
            uint32_t flying_cnt=0;
            uint32_t empty_cnt=0;
            uint32_t remain_cnt=0;
            std::vector<page_cache>::iterator iter;
            for(iter=target->cached_sc.begin(); iter!=target->cached_sc.end(); iter++){
                if(iter->flag==FLYING){
                    flying_cnt++;
                }
                else if(iter->flag==EMPTY){
                    empty_cnt++;
                }
                else{
                    remain_cnt++;
                }
            }

            target->now_cached_size=(flying_cnt+remain_cnt)*PAGESIZE;
            //target->cached_idx.clear();
            remain_size=target->max_cached_size-target->now_cached_size;
            if(target_size <remain_size){
                break;
            }
        }

        target->now_cached_size-=size;
        target_size-=size;

        if(type==SHORTCUT){
            target->cached_sc_num--;
            if(pc->flag==DIRTY){
                uint32_t new_ppa=get_ppa(id, pc);
                __send_write_reqeust(target, pc, new_ppa);
                pc->ppa=new_ppa;
            }
            else if(pc->flag==CLEAN){
                //do nothing
            }
            else{
                printf("invalidate type in cache!\n");
                abort();
            }
            //free(pc->data);
            pc->data=NULL;
            pc->node = NULL;
            pc->flag = EMPTY;
        }
        else if(type==IDX){
            if (target->cached_idx_num != target->cached_idx.size())
            {
                printf("errror %s:%u\n",__FILE__, __LINE__);
                abort();
            }
            target->cached_idx_num--;
            std::unordered_map<uint32_t, page_cache*>::iterator miter;
            miter=target->cached_idx.find(id);
            if(miter!=target->cached_idx.end()){
                delete miter->second;
                target->cached_idx.erase(miter);
            }
            else{
                abort();
            }
            if (target->cached_idx_num != target->cached_idx.size())
            {

            printf("errror %s:%u\n",__FILE__, __LINE__);
                abort();
            }
            //free(pc->data);
            //delete pc;
        }
        else{
            printf("type %u, id: %u error!\n", type, id);
            abort();
        }
    }
    lru_checker(target->lru);
    //pthread_mutex_unlock(&lru_lock);
    return;
}


void pc_force_evict_idx(pc_set *pcs, uint32_t pba){

    //pthread_mutex_lock(&lru_lock);
    std::unordered_map<uint32_t, page_cache*>::iterator miter;
    miter=pcs->cached_idx.find(pba);
    if(miter==pcs->cached_idx.end()){
        //pthread_mutex_unlock(&lru_lock);
        return;
    }
    if (pcs->cached_idx_num != pcs->cached_idx.size())
    {
        printf("errror %s:%u\n",__FILE__, __LINE__);
        abort();
    }
    lru_delete(pcs->lru, miter->second->node);
    pcs->now_cached_size-=miter->second->size;
    pcs->cached_idx_num--;
    delete miter->second;
    pcs->cached_idx.erase(miter);
    if (pcs->cached_idx_num != pcs->cached_idx.size())
    {
        printf("errror %s:%u\n",__FILE__, __LINE__);
        abort();
    }
    //lru_move_last(pcs->lru, miter->second->node);

    lru_checker(pcs->lru);

    //pthread_mutex_unlock(&lru_lock);
}

algo_req* pc_send_get_request(pc_set *target, cache_type type, request *parents, uint32_t ppa_or_scidx, value_set *value, void *param, void* (*end_req)(algo_req*)){
    algo_req *res=(algo_req*)calloc(1,sizeof(algo_req));
    res->parents=parents;
    res->type=MAPPINGR;
    res->end_req=end_req;
    res->value = value;
    res->param = (void *)param;

    page_cache *pc=NULL;
    if(type==SHORTCUT){
        pc=&target->cached_sc[ppa_or_scidx];
        res->ppa=pc->ppa;
    }
    else if(type==IDX){
        std::unordered_map<uint32_t, page_cache*>::iterator miter;
        miter=target->cached_idx.find(ppa_or_scidx);
        if(miter==target->cached_idx.end()){
            print_error(__LINE__);
            abort();
        }
        pc=miter->second;
        res->ppa=pc->ppa/L2PGAP;
    }
    else{
        print_error(__LINE__);
        abort();
    }

    if(!(pc->flag==OCCUPIED || pc->flag==FLYING)){
        print_error(__LINE__);
        abort();
    }
    pc->flag=FLYING;
    ((cache_read_param*)param)->pc=pc;
    /*add waiting request*/
    //pthread_mutex_lock(&req_wait_lock);
    if(type==SHORTCUT){
        if(parents==NULL){
            target->li->read(res->ppa, PAGESIZE, value, res);
        }
        else{
        std::map<uint32_t, std::vector<algo_req*>* >::iterator pm_iter;
        pm_iter=target->pending_sc_req_map.find(ppa_or_scidx);
        if(pm_iter==target->pending_sc_req_map.end()){
            std::vector<algo_req*>* new_pending_req=new std::vector<algo_req*>();
            target->pending_sc_req_map.insert(std::make_pair(ppa_or_scidx, new_pending_req));
            target->li->read(res->ppa, PAGESIZE, value, res);
        }
        else{
            pm_iter->second->push_back(res);
        }
        }
    }
    else{
        std::map<uint32_t, std::vector<algo_req*>* >::iterator pm_iter;
        pm_iter=target->pending_req_map.find(ppa_or_scidx);
        if(pm_iter==target->pending_req_map.end()){
            std::vector<algo_req*>* new_pending_req=new std::vector<algo_req*>();
            target->pending_req_map.insert(std::make_pair(ppa_or_scidx, new_pending_req));
            target->li->read(res->ppa, PAGESIZE, value, res);
        }
        else{
            pm_iter->second->push_back(res);
        }
    }
    //pthread_mutex_unlock(&req_wait_lock);

    return res;
}

page_cache *pc_set_pick(pc_set *pcs, cache_type type, uint32_t ppa_or_scidx, bool lock_flag){
    page_cache *pc=NULL;
    if(lock_flag){
        //pthread_mutex_lock(&lru_lock);  
    }
    if(type==SHORTCUT){
        pc=&pcs->cached_sc[ppa_or_scidx];
    }
    else if(type==IDX){
        std::unordered_map<uint32_t, page_cache*>::iterator miter;
        miter=pcs->cached_idx.find(ppa_or_scidx);
        if(miter==pcs->cached_idx.end()){
           if(lock_flag){
                //pthread_mutex_unlock(&lru_lock);  
            }
            return NULL;
        }
        pc=miter->second;
    }
    else{
        print_error(__LINE__);
        abort();
    }
    if(lock_flag){
        //pthread_mutex_unlock(&lru_lock);
    }

    return pc;
}


void pc_increase_refer_cnt(pc_set *target, page_cache *pc){
    //pthread_mutex_lock(&pinning_lock);
    //pc->refer_cnt++;
    //if(pc->refer_cnt==1){
    //    pc->ispinned=true;
    //    target->pinned_size+=pc->size;
    //}
    //pthread_mutex_unlock(&pinning_lock);
}

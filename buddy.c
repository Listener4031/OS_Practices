#include "buddy.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
//#define NULL ((void *)0)

static ArrayUnit** list_array_ = NULL; // 0 - 15
static AddrToPageUnit* addr_to_page_unit_ = NULL; // occupied page X --> all page
static void* global_start_addr_;

__attribute((constructor)) void Init(){
    list_array_ = (ArrayUnit**) malloc(17 * sizeof(ArrayUnit*));
    int i;
    for(i = 1; i <= 16; ++i){
        list_array_[i] = (ArrayUnit*) malloc(sizeof(ArrayUnit));
        list_array_[i] -> rank_ = i;
        list_array_[i] -> head_ = NULL;
        list_array_[i] -> tail_ = NULL;
        list_array_[i] -> cnt_page_unit_ = 0;
        list_array_[i] -> cnt_valid_page_unit_ = 0;
    }
    addr_to_page_unit_ = (AddrToPageUnit*) malloc(sizeof(AddrToPageUnit));
    addr_to_page_unit_ -> head_ = NULL;
    addr_to_page_unit_ -> tail_ = NULL;
    addr_to_page_unit_ -> cnt_page_unit_ = 0;
    addr_to_page_unit_ -> cnt_occupied_unit_ = 0;
}

int init_page(void *p, int pgcount){ // only support pgcount is power of two
    global_start_addr_ = p;
    // pgcount to binary
    int tmp_cnt_ = pgcount;
    int i;
    for(i = 16; i >= 1; --i){
        if(tmp_cnt_ >= (1 << (i - 1))){
            // create new page
            PageUnit* new_page_ptr = (PageUnit*) malloc(sizeof(PageUnit));
            new_page_ptr -> rank_pre_ = NULL;
            new_page_ptr -> rank_next_ = NULL;
            new_page_ptr -> addr_pre_ = NULL;
            new_page_ptr -> addr_next_ = NULL;
            new_page_ptr -> rank_ = i;
            new_page_ptr -> start_addr_ = global_start_addr_;
            new_page_ptr -> occupied_judger_ = False;
            // link to array unit
            list_array_[i] -> head_ = new_page_ptr;
            list_array_[i] -> tail_ = new_page_ptr;
            list_array_[i] -> cnt_page_unit_ = 1;
            list_array_[i] -> cnt_valid_page_unit_ = 1;
            // link to list
            addr_to_page_unit_ -> head_ = new_page_ptr;
            addr_to_page_unit_ -> tail_ = new_page_ptr;
            addr_to_page_unit_ -> cnt_page_unit_ = 1;
            addr_to_page_unit_ -> cnt_occupied_unit_ = 0;
            break;
        }
    }
    // check_list(0);
    return OK;
}

void delete_page_in_array(int _rank, PageUnit* _targeted_ptr){
    if(_targeted_ptr == NULL) exit(-1);
    if(_targeted_ptr -> rank_pre_ == NULL){
        if(_targeted_ptr -> rank_next_ == NULL){
            // nothing left
            list_array_[_rank] -> head_ = NULL;
            list_array_[_rank] -> tail_ = NULL;
        }
        else{
            // delete head
            _targeted_ptr -> rank_next_ -> rank_pre_ = NULL;
            list_array_[_rank] -> head_ = _targeted_ptr -> rank_next_;
        }
    }
    else{
        if(_targeted_ptr -> rank_next_ == NULL){
            // delete tail
            _targeted_ptr -> rank_pre_ -> rank_next_ = NULL;
            list_array_[_rank] -> tail_ = _targeted_ptr -> rank_pre_;
        }
        else{
            // delete mid
            _targeted_ptr -> rank_pre_ -> rank_next_ = _targeted_ptr -> rank_next_;
            _targeted_ptr -> rank_next_ -> rank_pre_ = _targeted_ptr -> rank_pre_;
        }
    }
    // update cnt
    list_array_[_rank] -> cnt_page_unit_ = list_array_[_rank] -> cnt_page_unit_ - 1;
    if(_targeted_ptr -> occupied_judger_ == False) list_array_[_rank] -> cnt_valid_page_unit_ = list_array_[_rank] -> cnt_valid_page_unit_ - 1;
    // check_list(1);
}

void delete_page_in_list(PageUnit* _targeted_ptr){
    if(_targeted_ptr == NULL) exit(-1);
    if(_targeted_ptr -> addr_pre_ == NULL){
        if(_targeted_ptr -> addr_next_ == NULL){
            // nothing left
            addr_to_page_unit_ -> head_ = NULL;
            addr_to_page_unit_ -> tail_ = NULL;
        }
        else{
            // delete head
            _targeted_ptr -> addr_next_ -> addr_pre_ = NULL;
            addr_to_page_unit_ -> head_ = _targeted_ptr -> addr_next_;
        }
    }
    else{
        if(_targeted_ptr -> addr_next_ == NULL){
            // delete tail
            _targeted_ptr -> addr_pre_ -> addr_next_ = NULL;
            addr_to_page_unit_ -> tail_ = _targeted_ptr -> addr_pre_;
        }
        else{
            // delete mid
            _targeted_ptr -> addr_pre_ -> addr_next_ = _targeted_ptr -> addr_next_;
            _targeted_ptr -> addr_next_ -> addr_pre_ = _targeted_ptr -> addr_pre_;
        }
    }
    // update cnt
    addr_to_page_unit_ -> cnt_page_unit_ = addr_to_page_unit_ -> cnt_page_unit_ - 1;
    if(_targeted_ptr -> occupied_judger_ == True) addr_to_page_unit_ -> cnt_occupied_unit_ = addr_to_page_unit_ -> cnt_occupied_unit_ - 1;
    // check_list(2);
}

void add_page_to_list(PageUnit* _lead_ptr, PageUnit* _new_ptr){
    if(_lead_ptr == NULL) exit(-1);
    if(_lead_ptr -> addr_next_ == NULL){
        // add to tail
        _lead_ptr -> addr_next_ = _new_ptr;
        _new_ptr -> addr_pre_ = _lead_ptr;
        _new_ptr -> addr_next_ = NULL;
        addr_to_page_unit_ -> tail_ = _new_ptr;
    }
    else{
        // add to mid
        _lead_ptr -> addr_next_ -> addr_pre_ = _new_ptr;
        _new_ptr -> addr_next_ = _lead_ptr -> addr_next_;
        _lead_ptr -> addr_next_ = _new_ptr;
        _new_ptr -> addr_pre_ = _lead_ptr;
    }
    addr_to_page_unit_ -> cnt_page_unit_ = addr_to_page_unit_ -> cnt_page_unit_ + 1;
    if(_new_ptr -> occupied_judger_ == True) addr_to_page_unit_ -> cnt_occupied_unit_ = addr_to_page_unit_ -> cnt_occupied_unit_ + 1;
    // check_list(3);
}

void add_page_to_array(int _rank, PageUnit* _new_ptr){
    if(_rank < 1 || _rank > 16) exit(-1);
    if(list_array_[_rank] -> cnt_page_unit_ == 0){
        // add first page
        list_array_[_rank] -> head_ = _new_ptr;
        list_array_[_rank] -> tail_ = _new_ptr;
        _new_ptr -> rank_pre_ = NULL;
        _new_ptr -> rank_next_ = NULL;
    }
    else{
        // add to tail
        list_array_[_rank] -> tail_ -> rank_next_ = _new_ptr;
        _new_ptr -> rank_pre_ = list_array_[_rank] -> tail_;
        _new_ptr -> rank_next_ = NULL;
        list_array_[_rank] -> tail_ = _new_ptr;
    }
    list_array_[_rank] -> cnt_page_unit_ = list_array_[_rank] -> cnt_page_unit_ + 1;
    if(_new_ptr -> occupied_judger_ == False) list_array_[_rank] -> cnt_valid_page_unit_ = list_array_[_rank] -> cnt_valid_page_unit_ + 1;
    // check_list(4);
}

void update_ptr_in_list(PageUnit* _targeted_ptr, PageUnit* _new_ptr){
    if(_targeted_ptr == NULL) exit(-1);
    if(_targeted_ptr -> addr_pre_ != NULL) _targeted_ptr -> addr_pre_ -> addr_next_ = _new_ptr;
    else addr_to_page_unit_ -> head_ = _new_ptr;
    if(_targeted_ptr -> addr_next_ != NULL) _targeted_ptr -> addr_next_ -> addr_pre_ = _new_ptr;
    else addr_to_page_unit_ -> tail_ = _new_ptr;
    if(_targeted_ptr -> occupied_judger_ == False 
    && _new_ptr -> occupied_judger_ == True) addr_to_page_unit_ -> cnt_occupied_unit_ = addr_to_page_unit_ -> cnt_occupied_unit_ + 1;
    // check_list(5);
}

void print_list(int _min_rank){
    PageUnit* tmp_ptr = NULL;
    printf("debug: list: ````````````\n");
    for(tmp_ptr = addr_to_page_unit_ -> head_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> addr_next_){
        if(tmp_ptr -> rank_ >= _min_rank){
            printf("debug: tmp_ptr -> rank_ : %d\n", tmp_ptr -> rank_);
            printf("debug: tmp_ptr -> start_addr_ : %ld\n", tmp_ptr -> start_addr_ - global_start_addr_);
            printf("debug: tmp_ptr -> occupied_judger_ : %d\n", tmp_ptr -> occupied_judger_);
            printf("debug: ______________\n");
        }
        //if(tmp_ptr -> start_addr_ - global_start_addr_ >= 16384) break;
    }
    printf("debug: end of print, cnt : %d\n", addr_to_page_unit_ -> cnt_page_unit_);
    printf("debug: end of print, cnt_occupied : %d\n", addr_to_page_unit_ -> cnt_occupied_unit_);
    printf("\n");
}

void *alloc_pages(int rank){
    if(rank < 1 || rank > 16) return -EINVAL;
    int i, locate_ = -1;
    // find a locate_
    for(i = rank; i <= 16; ++i){
        if(list_array_[i] -> cnt_valid_page_unit_ >= 1){
            locate_ = i;
            break;
        }
    }
    if(locate_ == -1) return -ENOSPC;
    // departure large to small
    if(locate_ == rank){
        // just occupy the page, do not add page, for the pointer point to the same addr, but add cnt
        PageUnit* tmp_ptr = NULL;
        for(tmp_ptr = list_array_[locate_] -> head_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> rank_next_){
            if(tmp_ptr -> occupied_judger_ == False){ // must happen
                tmp_ptr -> occupied_judger_ = True;
                // update cnt of array unit
                list_array_[locate_] -> cnt_valid_page_unit_ = list_array_[locate_] -> cnt_valid_page_unit_ - 1;
                // update cnt of list
                addr_to_page_unit_ -> cnt_occupied_unit_ = addr_to_page_unit_ -> cnt_occupied_unit_ + 1;
                // check_list(-1);
                return tmp_ptr -> start_addr_;
            }
        }
    }
    else{
        // depature located page
        PageUnit* tmp_ptr = NULL;
        void* tmp_start_addr = ((void *)0);
        void* tmp_end_addr = ((void *)0);
        void* ret_addr = ((void *)0);
        for(tmp_ptr = list_array_[locate_] -> head_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> rank_next_){
            if(tmp_ptr -> occupied_judger_ == False){ // must happen
                ret_addr = tmp_ptr -> start_addr_;
                // delete located page in array
                delete_page_in_array(locate_, tmp_ptr);
                break;
            }
        }
        // tmp_ptr points to the largest one, but from addr its rank becomes the min.
        // example1 --- (p, 128(#8)) get 32 : (p, 32(#6)), (p + 32m, 32(#6)), (p + 64m, 64(#7))
        // example2 --- (p, 128(#8)) get 64 : (p, 64(#7)), (p + 64m, 64(#7))
        // update rank in list
        tmp_ptr -> rank_ = rank;
        tmp_start_addr = tmp_ptr -> start_addr_;
        tmp_end_addr = tmp_ptr -> start_addr_ + (1 << (tmp_ptr -> rank_ - 1)) * MINUNIT - MINUNIT;
        // create new page
        PageUnit* new_page_ptr = (PageUnit*) malloc(sizeof(PageUnit));
        new_page_ptr -> rank_pre_ = tmp_ptr -> rank_pre_;
        new_page_ptr -> rank_next_ = tmp_ptr -> rank_next_;
        new_page_ptr -> addr_pre_ = tmp_ptr -> addr_pre_;
        new_page_ptr -> addr_next_ = tmp_ptr -> addr_next_;
        new_page_ptr -> rank_ = tmp_ptr -> rank_;
        new_page_ptr -> start_addr_ = tmp_ptr -> start_addr_;
        new_page_ptr -> occupied_judger_ = True;
        // update ptr, because tmp_ptr actually will be deleted. 
        update_ptr_in_list(tmp_ptr, new_page_ptr);
        // add page (occupied) to array unit
        add_page_to_array(rank, new_page_ptr);
        int i;
        PageUnit* lead_ptr = new_page_ptr;
        for(i = rank; i <= locate_ - 1; ++i){
            // update tmp addr
            tmp_start_addr = tmp_end_addr + MINUNIT;
            tmp_end_addr = tmp_start_addr + (1 << (i - 1)) * MINUNIT - MINUNIT;
            // create new page
            new_page_ptr = (PageUnit*) malloc(sizeof(PageUnit));
            new_page_ptr -> rank_ = i;
            new_page_ptr -> start_addr_ = tmp_start_addr;
            new_page_ptr -> occupied_judger_ = False;
            // add page
            add_page_to_array(i, new_page_ptr);
            add_page_to_list(lead_ptr, new_page_ptr);
            lead_ptr = new_page_ptr;
        }
        // check_list(-2);
        return ret_addr;
    }
    return ((void *)0);
}

void try_merge(PageUnit* _targeted_ptr){
    if(_targeted_ptr == NULL) exit(-1);
    if((_targeted_ptr -> addr_pre_ != NULL) 
    && (_targeted_ptr -> addr_pre_ -> rank_ == _targeted_ptr -> rank_) 
    && (_targeted_ptr -> addr_pre_ -> occupied_judger_ == False) 
    && (((_targeted_ptr -> addr_pre_ -> start_addr_ - global_start_addr_) / MINUNIT) % (2 * (1 << (_targeted_ptr -> addr_pre_ -> rank_ - 1))) == 0) ){
        // create new large page 
        PageUnit* new_page_ptr = (PageUnit*) malloc(sizeof(PageUnit));
        new_page_ptr -> rank_ = _targeted_ptr -> rank_ + 1;
        new_page_ptr -> start_addr_ = _targeted_ptr -> addr_pre_ -> start_addr_;
        new_page_ptr -> occupied_judger_ = False;
        // update array 
        add_page_to_array(new_page_ptr -> rank_, new_page_ptr);
        delete_page_in_array(_targeted_ptr -> addr_pre_ -> rank_, _targeted_ptr -> addr_pre_);
        delete_page_in_array(_targeted_ptr -> rank_, _targeted_ptr);
        // update list 
        add_page_to_list(_targeted_ptr, new_page_ptr);
        delete_page_in_list(_targeted_ptr -> addr_pre_);
        delete_page_in_list(_targeted_ptr);
        // try merge more 
        try_merge(new_page_ptr);
    }
    else if((_targeted_ptr -> addr_next_ != NULL) 
    && (_targeted_ptr -> addr_next_ -> rank_ == _targeted_ptr -> rank_) 
    && (_targeted_ptr -> addr_next_ -> occupied_judger_ == False) 
    && (((_targeted_ptr -> start_addr_ - global_start_addr_) / MINUNIT) % (2 * (1 << (_targeted_ptr -> rank_ - 1))) == 0) ){
        // create new large page 
        PageUnit* new_page_ptr = (PageUnit*) malloc(sizeof(PageUnit));
        new_page_ptr -> rank_ = _targeted_ptr -> rank_ + 1;
        new_page_ptr -> start_addr_ = _targeted_ptr -> start_addr_;
        new_page_ptr -> occupied_judger_ = False;
        // update array 
        add_page_to_array(new_page_ptr -> rank_, new_page_ptr);
        delete_page_in_array(_targeted_ptr -> addr_next_ -> rank_, _targeted_ptr -> addr_next_);
        delete_page_in_array(_targeted_ptr -> rank_, _targeted_ptr);
        // update list 
        delete_page_in_list(_targeted_ptr -> addr_next_);
        add_page_to_list(_targeted_ptr, new_page_ptr);
        delete_page_in_list(_targeted_ptr);
        // try merge more 
        try_merge(new_page_ptr);
    }
}

void check_list(int tag){
    printf("debug: in check.\n");
    if(addr_to_page_unit_ -> head_ == NULL && addr_to_page_unit_ -> cnt_page_unit_ != 0){
        printf("debug: check failed1 at %d.\n", tag);
        exit(-1);
    }
    if(addr_to_page_unit_ -> head_ -> addr_pre_ != NULL || addr_to_page_unit_ -> tail_ -> addr_next_ != NULL){
        printf("debug: check failed2 at %d.\n", tag);
        exit(-1);
    }
    printf("debug: pass check.\n");
}

int return_pages(void *p){ 
    if(addr_to_page_unit_ -> cnt_occupied_unit_ == 0) return -EINVAL;
    // check_list(6);
    PageUnit* targeted_ptr = NULL;
    int i = 0;
    for(targeted_ptr = addr_to_page_unit_ -> head_; targeted_ptr != NULL; targeted_ptr = targeted_ptr -> addr_next_){
        if(targeted_ptr -> start_addr_ == p && targeted_ptr -> occupied_judger_ == True){
            targeted_ptr -> occupied_judger_ = False;
            // update cnt
            addr_to_page_unit_ -> cnt_occupied_unit_ = addr_to_page_unit_ -> cnt_occupied_unit_ - 1;
            list_array_[targeted_ptr -> rank_] -> cnt_valid_page_unit_ = list_array_[targeted_ptr -> rank_] -> cnt_valid_page_unit_ + 1;
            // try merge
            try_merge(targeted_ptr);
            return OK;
        }
    }
    return -EINVAL;
}

int query_ranks(void *p){
    if(addr_to_page_unit_ -> cnt_page_unit_ == 0) return -EINVAL;
    PageUnit* tmp_ptr = NULL;
    for(tmp_ptr = addr_to_page_unit_ -> head_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> addr_next_){
        if(tmp_ptr -> start_addr_ == p) return tmp_ptr -> rank_;
    }
    return -EINVAL;
}

int query_page_counts(int rank){
    if(rank < 1 || rank > 16) return -EINVAL;
    return list_array_[rank] -> cnt_valid_page_unit_;
}


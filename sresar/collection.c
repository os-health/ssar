/*
 *
 *  Copyright (c) 2021 Alibaba Inc.
 *  SRESAR is licensed under Mulan PSL v2.
 *  You can use this software according to the terms and conditions of the Mulan PSL v2.
 *  You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 *  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 *  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 *  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *  See the Mulan PSL v2 for more details.
 *
 *  Part of SRESAR (Site Reliability Engineering System Activity Reporter)
 *  
 *  Author:  Miles Wen <mileswen@linux.alibaba.com>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "collection.h"

void circular_linkedlist_init(Node *plist, int list_size){
    int i;
    Node* first_node   = plist;
    Node* current_node = plist;
    Node* prev_node    = NULL;
    Node* next_node    = current_node + 1;
    current_node->next = next_node;
    current_node->load1 = (double)0;
    for(i = 0; i < list_size - 2; i++){
        prev_node    = current_node;
        current_node = next_node;
        next_node    = current_node + 1;
        
        current_node->prev = prev_node; 
        current_node->next = next_node;
        current_node->load1 = (double)0;
    }
    prev_node    = current_node;
    current_node = next_node;
    next_node    = first_node;
    current_node->prev = prev_node; 
    current_node->next = next_node;
    current_node->load1 = (double)0;
    
    first_node->prev   = current_node;
}


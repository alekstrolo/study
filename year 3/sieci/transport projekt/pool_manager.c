//Konrad Kasprzyk
//299243

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/ip_icmp.h>
#include <stdbool.h>
#include <string.h>
#include "pool_manager.h"

struct pool_pointers init_pool(int pool_size, int file_size){
    struct pool_data *head = malloc(sizeof(struct pool_data));
    head->ready = false;
    head->start = 0;
    head->length = (file_size >= 1000)? 1000 : file_size;
    head->next = head;
    file_size -= 1000;
    struct pool_data *curr = head;
    struct pool_data *next;
    int next_start = 1000;

    for(int i=0; i<pool_size; i++){
        if(file_size <= 0)
            break;
        next = malloc(sizeof(struct pool_data));
        next->ready = false;
        next->start = next_start;
        next->length = (file_size >= 1000)? 1000 : file_size;
        file_size -= 1000;
        next_start += 1000;
        next->next=head;
        curr->next = next;
        curr = next;
    }
    struct pool_pointers ret_pointers;
    ret_pointers.head = head;
    ret_pointers.tail = curr;
    return ret_pointers;
}

int write_to_file(struct pool_pointers* pool_ptr, int bytes_written_to_file, int file_size, FILE *file){
    int bytes_written = bytes_written_to_file;
    struct pool_data *head = pool_ptr->head;
    struct pool_data *tail = pool_ptr->tail;
    struct pool_data *next;
    struct pool_data *new_tail;
    if(head == tail){
        //printf("head==tail\n");
        if(head->ready){
            //printf("write_to_file head->ready\n");
            if (fwrite(head->data, sizeof(char), head->length, file) < head->length)
	        {
    	        fprintf(stderr, "fwrite error: %s\n", strerror(errno)); 
		        exit(EXIT_FAILURE);
	        }
            bytes_written += head->length;
            head->ready = false;
            if(bytes_written<file_size){
                head->start += 1000;
                head->length = (file_size - bytes_written >= 1000)? 1000 : file_size - bytes_written;
            }
        }
        return bytes_written;
    }

    // free pool datagram, move head, add tail with bigger start value
    while(head->ready && head->next != head){
        //printf("wewnatrz while w write to file. head->start= %d\n",head->start);
        if (fwrite(head->data, sizeof(char), head->length, file) < head->length)
	    {
    	    fprintf(stderr, "fwrite error: %s\n", strerror(errno)); 
		    exit(EXIT_FAILURE);
	    }
        bytes_written += head->length;
        next = head -> next;
        free(head);
        head = next;
        tail->next = head;

        // make tail
        if(tail->start + 1000 < file_size){
            new_tail = malloc(sizeof(struct pool_data));
            new_tail->next = head;
            new_tail->start = tail->start + 1000;
            new_tail->length = (file_size - bytes_written >= 1000)? 1000 : file_size - bytes_written;
            new_tail->ready = false;
            tail->next = new_tail;
            tail = new_tail;
        }
    }
    pool_ptr->head = head;
    pool_ptr->tail = tail;
    return bytes_written;
}

void insert_to_pool(struct pool_data* head, struct rec_data* rec_data){
    struct pool_data* curr = head;
    do{
        //printf("curr head start= %d, curr head ready= %d\n", curr->start, curr->ready);
        //printf("rec_data->start= %d\n",rec_data->start);
        if(curr->start == rec_data->start)
        {
            if(curr->ready == false)
            {
                //printf("curr->start= %d ready\n",curr->start);
                curr->ready = true;
                memcpy(curr->data, rec_data->data + rec_data->data_prefix, rec_data->length);
            }
            break;
        }

        curr = curr->next;
    }while(curr != head);
}
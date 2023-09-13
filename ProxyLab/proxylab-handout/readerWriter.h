#ifndef READER_WRITER_H
#define READER_WRITER_H

#include "csapp.h"
#include "semaphore.h"

void init_reader_writer();
void readerLock();
void readerUnlock();
void writerLock();
void writerUnlock();
void readerToWriter();

#endif

// sem_t mtx;
// sem_t w;
// // 读者不断到达，就会造成写者饥饿
// void reader(){
//     while(1){
//         P(&mtx);
//         read_cnt++;
//         if(read_cnt == 1){
//             P(&w);
//         }
//         V(&mtx);
//         // doRead();
//         P(&mtx);
//         read_cnt--;
//         if(read_cnt == 0){
//             V(&w);
//         }
//         V(&mtx);
//     }
// }
// // 同理 写者不断到达，就会让读者饥饿
// void writer(){
//     while(1){
//         P(&w);
//         // doWrite();
//         V(&w);
//     }
// }
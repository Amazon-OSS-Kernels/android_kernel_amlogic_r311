/*
 * Copyright (c) 2023 Amazon.com, Inc. or its affiliates.  All rights reserved.
 *
 * PROPRIETARY/CONFIDENTIAL.  USE IS SUBJECT TO LICENSE TERMS.
 */

/*
 * mmapleak shows, mmap calls with
 * no munmap or partial/full munmap calls
 */

#include <unistd.h>
#include <sys/mman.h>

#include <iostream>

int kill( pid_t pid, int sig );

#define PAGE_SIZE 4096

int curidx;
char *alloc[6];

enum munmap_t {
    FULL,
    PARTIAL,
    MORE
};

int munmap_pages(char *p, int size, munmap_t type) {
    int chunks = 0;
    int chunk_size = 0;
    int nchunks = size / PAGE_SIZE;

    if (p == NULL)
        return -1;

    switch (type) {
    case PARTIAL:
        chunks = nchunks - 1;
        chunk_size = size / nchunks;
        break;
    case FULL:
        chunks = 1;
        chunk_size = size;
        break;
    case MORE:
        chunks = 1;
        chunk_size = size + PAGE_SIZE;
        break;
    }

    std::cout << std::endl << "Trying to Unmap " << chunk_size * chunks << " bytes" << std::endl;

    for (int i = 0; i < chunks; i++) {
        if (munmap(p, chunk_size) < 0) {
            std::cerr << "munmap failed" << std::endl;
            return -1;
        }
        p += chunk_size;
    }

    std::cout << "Unmapped " << chunk_size * chunks << " bytes from mapped " << size << " bytes" << std::endl;

    return 0;
}

int mmap_pages(int bytesToMap)
{
    std::cout << std::endl << "Trying to Mmap " << bytesToMap << " bytes" << std::endl;

    char *p = (char *)mmap(NULL, bytesToMap,
          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
    if (p == NULL) {
        std::cerr << "mmap failed, errno " << errno << std::endl;
        return -1;
    }
    for (int j = 0; j < bytesToMap; j++) {
        /* Prevent KSM from de-duping by writing random data */
        p[j] = (j + random()) & 0xFF;
    }

    alloc[curidx++] = p;

    std::cout << "Mmapped " << bytesToMap << " bytes" << std::endl;

    return 0;
}

int main()
{
    int bytesToMap = 3 * PAGE_SIZE;

    std::cout << std::endl << "Send Signal -45 to process" << std::endl;
    kill(getpid(), 45);
    sleep(5);

    if (mmap_pages(bytesToMap) < 0)
        return -1;
    if (mmap_pages(bytesToMap) < 0)
        return -1;
    if (mmap_pages(bytesToMap) < 0)
        return -1;

    if (curidx) {
        if (munmap_pages(alloc[--curidx], bytesToMap, PARTIAL) < 0)
            std::cerr << "Test 1 failed: errno " << errno << std::endl;
        if (munmap_pages(alloc[--curidx], bytesToMap, MORE) == 0)
            std::cerr << "Test 2 failed: errno " << errno << std::endl;
        if (munmap_pages(alloc[--curidx], bytesToMap, FULL) < 0)
            std::cerr << "Test 3 failed: errno " << errno << std::endl;
    }

    sleep(5);

    std::cout << std::endl << "Send Signal -45 to process" << std::endl;
    kill(getpid(), 45);

    std::cout << "Test Done...." << std::endl;
    std::cout << "Exiting...." << std::endl;
    sleep(10);
    kill(getpid(), 9);

    return 0;
}

#include <stdio.h>
#include <string.h>
#include "libTinyFS.h"

int main()
{

    // 1. create new file system
    printf("1. create new tinyFS\n");
    int result = tfs_mkfs("tinyFSDisk", 8192);
    if (result < 0)
    {
        printf("test 1: failed\n");
        return 1;
    }
    printf("test 1: file system created\n");
    printf("\n");

    // 2. mount file system
    printf("2. mount tinyFS\n");
    result = tfs_mount("tinyFSDisk");
    if (result < 0)
    {
        printf("test 2: failed\n");
        return 1;
    }
    printf("test 2: file system mounted\n");
    printf("\n");

    // 3. open/create a file
    printf("3. open/create file 'testfile'\n");
    fileDescriptor fd1 = tfs_openFile("testfile");
    if (fd1 < 0)
    {
        printf("test 3: failed\n");
        return 1;
    }
    printf("test 3: file opened with fd = %d\n", fd1);
    printf("\n");

    // 4. write data to file
    printf("4. write data to 'testfile'\n");
    char writeData[] = "CSC453 is an awesome class";
    result = tfs_writeFile(fd1, writeData, strlen(writeData));
    if (result < 0)
    {
        printf("test 4: failed\n");
        return 1;
    }
    printf("test 4: \"%s\"\n", writeData);
    printf("\n");

    // 5. seek to a specific position
    printf("5. seek to offset 7\n");
    result = tfs_seek(fd1, 7);
    if (result < 0)
    {
        printf("test 5: failed\n");
    }
    else
    {
        char readByte;
        result = tfs_readByte(fd1, &readByte);
        if (result >= 0)
        {
            printf("test 5: byte at position 7: '%c'\n", readByte);
        }
    }
    printf("\n");
    
    // 6. create a second file
    printf("6. create second file 'data'\n");
    fileDescriptor fd2 = tfs_openFile("data");
    if (fd2 < 0)
    {
        printf("test 6: failed\n");
    }
    else
    {
        printf("test 6: second file opened with fd = %d\n", fd2);
        // 7. write to second file
        printf("7. write data to 'data' file\n");
        char data2[] = "I cannot wait to graduate soon";
        result = tfs_writeFile(fd2, data2, strlen(data2));
        if (result < 0)
        {
            printf("test 7: failed\n");
        }
        else
        {
            printf("test 7: \"%s\"\n", data2);
        }
    }
    printf("\n");
    
    // 8. reopen first file to verify persistence
    printf("8. reopen 'testfile' to verify it still exists\n");
    fileDescriptor fd3 = tfs_openFile("testfile");
    if (fd3 < 0)
    {
        printf("test 8: failed\n");
    }
    else
    {
        printf("test 8: file reopened with fd = %d\n", fd3);
    }
    printf("\n");

    // 9. close files
    printf("9. close files\n");
    result = tfs_closeFile(fd1);
    if (result >= 0)
    {
        printf("test 9: closed FD %d\n", fd1);
    }
    else
    {
        printf("test 9: could not close FD %d\n", fd1);
    }

    if (fd2 >= 0)
    {
        result = tfs_closeFile(fd2);
        if (result >= 0)
        {
            printf("test 9: closed FD %d\n", fd2);
        }
        else
        {
            printf("test 9: failed");
        }
    }

    if (fd3 >= 0)
    {
        result = tfs_closeFile(fd3);
        if (result >= 0)
        {
            printf("test 9: closed FD %d\n", fd3);
        }
        else
        {
            printf("test 9: failed");
        }
    }
    printf("\n");

    // 10. delete a file
    printf("10. reopen and delete 'data' file\n");
    fd2 = tfs_openFile("data");
    if (fd2 >= 0)
    {
        result = tfs_deleteFile(fd2);
        if (result < 0)
        {
            printf("test 10: failed\n");
        }
        else
        {
            printf("test 10: file 'data' deleted\n");
        }
    }
    printf("\n");

    // 11. unmount file system
    printf("11. unmount file system\n");
    result = tfs_unmount();
    if (result < 0)
    {
        printf("test 11: failed\n");
    }
    else
    {
        printf("test 11: file system unmounted\n");
    }
    printf("\n");

    return 0;
}

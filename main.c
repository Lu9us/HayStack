#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

struct processingData
{
    int haystackBufferLength;
    char *buffer;
    char **lineBuffer;
    int uuidLength;
    int bufferSize;
    int *fileHandle;
    int threadID;
};

struct BackBuffer
{

    char *p_buff;
    int *fileHandle;
    int haystackBufferLength;
    int uuidLength;
    int used;
    int ready;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int totalProcessed = 0;
int totalRead = 0;
size_t haySize = 0;
size_t chunks = 0;
int *matched;

void *getBackBuffer(void *backupBuffer)
{
    struct BackBuffer *data = (struct BackBuffer *)backupBuffer;
    if ((chunks < haySize) && data->used == 1)
    {
        int readSize = read(*data->fileHandle, data->p_buff, data->haystackBufferLength * data->uuidLength * sizeof(char));
        pthread_mutex_lock(&mutex);
        chunks += readSize;
        pthread_mutex_unlock(&mutex);
        data->used = 0;
        data->ready = 0;
    }
}


void *processBuffer(void *processingDat)
{
    struct processingData *data = (struct processingData *)processingDat;
    int matchedBuffer[data->haystackBufferLength];
    pthread_t backBufferThread;
    struct BackBuffer buffer;
    buffer.fileHandle = data->fileHandle;
    buffer.haystackBufferLength = data->haystackBufferLength;
    buffer.uuidLength = data->uuidLength;
    buffer.used = 1;
    buffer.ready = 1;
    buffer.p_buff = malloc(data->haystackBufferLength * data->uuidLength * sizeof(char));
    char * line = malloc(data->uuidLength * sizeof(char));
    while (chunks < haySize)
    {

        size_t readSize = 0;
        /* memset(matchedBuffer, 0x00, sizeof(int) * data->haystackBufferLength); */

        if (buffer.used == 1)
        {
            readSize = read(*data->fileHandle, data->buffer, data->haystackBufferLength * data->uuidLength * sizeof(char));
            pthread_mutex_lock(&mutex);
            /*  printf("thread: ");
            printf("%d",data->threadID);
            printf(" creating backbuffer\n"); */
            chunks += readSize;
            pthread_mutex_unlock(&mutex);
        }
        else
        {

            data->buffer = buffer.p_buff;
            buffer.p_buff = malloc(data->haystackBufferLength * data->uuidLength * sizeof(char));
            buffer.used = 1;
            pthread_create(&backBufferThread, NULL, &getBackBuffer, (void *)&buffer);
            /*    pthread_mutex_lock(&mutex);
             printf("thread: ");
            printf("%d",data->threadID);
            printf(" using backbuffer\n");
               pthread_mutex_unlock(&mutex); */
        }

        int matches = 0;

        for (int y = 0; y < data->bufferSize; y++)
        {
            if (matched[y] == 1)
            {
                continue;
            }
            for (int x = 0; x < data->haystackBufferLength; x++)
            {

                if (  memcmp(data->buffer + (x * data->uuidLength), data->lineBuffer[y], data->uuidLength) == 0 )
                {

                    printf(data->lineBuffer[y]);

                    matches++;
                    matchedBuffer[x] = 1;
                    pthread_mutex_lock(&mutex);
                    totalProcessed++;
                    matched[y] = 1;
                    pthread_mutex_unlock(&mutex);

                    break;
                }
              
            }
        }
    }
}

int main(int argc, char *argv[])
{
    pthread_t *threadArray;
    struct stat charStat;
    errno = 0;
    FILE *needleFile;
    int hayFile;
    int needles = 0;
    char cwd[1024];
    int uuidLength = 38;
    int haystackBufferLength = 10083;

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        //printf(cwd);
        //printf("\n");
    }

    for (int i = 0; i < argc; i++)
    {

        if (strcmp("--needles", argv[i]) == 0)
        {
            needleFile = fopen(argv[i + 1], "r");
            if (needleFile == NULL)
            {
                printf("file load failed for: ");
                printf(argv[i + 1]);
                //printf(errno);
                printf("\n");
            }
        }
        if (strcmp("--haystack", argv[i]) == 0)
        {
            stat(argv[i + 1], &charStat);
            haySize = charStat.st_size;
            hayFile = open(argv[i + 1], O_RDONLY);

            if (hayFile == -1)
            {
                printf("file load failed for: ");
                printf(argv[i + 1]);
                printf("\n");
            }
        }
    }

    int bufferSize = 32;
    char **lineBuffer = malloc(bufferSize * sizeof(*lineBuffer));
    char *line;
    line = malloc(uuidLength * sizeof(char));
    int i = 0;

    while (fgets(line, uuidLength, needleFile))
    {
        if (i < bufferSize)
        {

            lineBuffer[i] = malloc(256);

            strcpy(lineBuffer[i], line);

            i++;
        }
        if (i >= bufferSize)
        {
            char **tmp = realloc(lineBuffer, bufferSize * 2 * sizeof(*lineBuffer));
            if (tmp != NULL)
            {
                bufferSize *= 2;
                lineBuffer = tmp;
            }
        }

        // printf("%d",i);
    }

    bufferSize = i;
    int initalThreads = 12;

    matched = malloc(bufferSize * sizeof(int));
    //char *buffer = malloc(haystackBufferLength * uuidLength * sizeof(char));
    int ch;

    int matches = 0;
    int readSize = 0;

    uuidLength = strlen(lineBuffer[0]);
    threadArray = malloc(sizeof(pthread_t) * initalThreads);
    char threadBuffers[initalThreads][haystackBufferLength * uuidLength * sizeof(char)];
    struct processingData *buffData;
    int threads = 0;

    if (totalProcessed >= bufferSize)
    {
        return 0;
    }

    // printf("chunks:");
    // printf("%zu",chunk);
    // printf(" ");
    // printf("%zu",haySize);
    // printf("\n");

    if (bufferSize > 100)
    {

        while (threads < initalThreads)
        {

            buffData = malloc(sizeof(struct processingData));

            buffData->buffer = threadBuffers[threads];
            buffData->bufferSize = bufferSize;
            buffData->uuidLength = uuidLength;
            buffData->lineBuffer = lineBuffer;
            buffData->haystackBufferLength = haystackBufferLength;
            buffData->fileHandle = &hayFile;
            buffData->threadID = threads;
            //threadArray[threads];
            pthread_create(&threadArray[threads], NULL, processBuffer, (void *)buffData);
            threads++;
        }
    }
    else
    {
        buffData = malloc(sizeof(struct processingData));

        buffData->buffer = threadBuffers[threads];
        buffData->bufferSize = bufferSize;
        buffData->uuidLength = uuidLength;
        buffData->lineBuffer = lineBuffer;
        buffData->haystackBufferLength = haystackBufferLength;
        buffData->fileHandle = &hayFile;
        buffData->threadID = 0;
        processBuffer((void *)buffData);
    }

    for (i = 0; i <= threads; i++)
        pthread_join(threadArray[i], NULL);

    //  printf("%d",totalProcessed);
    // printf("%d",totalRead);
    return 0;
}

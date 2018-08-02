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
};



pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int totalProcessed = 0;
int totalRead = 0;
size_t haySize = 0;
size_t chunks = 0;
int* matched;

void *processBuffer(void *processingDat)
{
     struct processingData *data = (struct processingData *)processingDat;
    int  matchedBuffer [data->haystackBufferLength];

    while (chunks < haySize)
    {
        
        size_t readSize = 0;
        memset(matchedBuffer,0x00,sizeof(int)*data->haystackBufferLength);
        pthread_mutex_lock(&mutex);
        readSize = read(*data->fileHandle, data->buffer, data->haystackBufferLength * data->uuidLength * sizeof(char));
        chunks += readSize;
        pthread_mutex_unlock(&mutex);
        int matches = 0;

        for (int y = 0; y < data->bufferSize; y++)
        {
            if(matched[y]==1)
            {
                continue;
            }
            for (int x = 0; x < data->haystackBufferLength; x++)
            {
                if(matchedBuffer[x]==1)
            {
                continue;
            }

                if (memcmp(data->buffer + (x * data->uuidLength), data->lineBuffer[y], data->uuidLength) == 0)
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
                else
                {
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
    int haystackBufferLength = 1024;
    
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
    matched = malloc(bufferSize* sizeof(int));
    char *buffer = malloc(haystackBufferLength * uuidLength * sizeof(char));
    int ch;

    int matches = 0;
    int readSize = 0;
    int initalThreads = 32;
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
        processBuffer((void *)buffData);
    }

    for (i = 0; i <= threads; i++)
        pthread_join(threadArray[i], NULL);

    //  printf("%d",totalProcessed);
    // printf("%d",totalRead);
    return 0;
}

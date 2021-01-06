#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <sys/resource.h>

void PrintFunc(int **mat, int len);                                          // 출력함수
void makeMatrix(int **matrix, int X, int Y);                                 // matrix에 랜덤수 삽입
void CheckXY(int num, int *ch_X, int *ch_Y);                                 // input숫자 판별기
void MemFree(int **mat, int len);                                            // 할당받은 메모리 영역 반환
int **Convolution(int *x, int *y, int **mat);                                // Convolution filtering
int ConvolLayer(int (*fun)[3]);                                              // Convolution core filter
int **MaxPooling(int *x, int *y, int **mat);                                 // max-pooling filtering
int PoolingLayer(int (*fun)[2]);                                             // max-pooling core filter
int **ResultMat(int x, int y, int (*func)(void));                            // 함수로 인자를 받아 matrix만든후 그 matrix주소 반환
mqd_t ku_mq_open(char *mqname, int msg_size);                                // message queue wrapper funtion, 권한값, 특징 들을 반복을 피하기 위해
void ku_mq_send(char *mqname, char *data, int msg_size, unsigned int id);    // ku_mq_open을 이용하는 wrapper function
void ku_mq_receive(char *mqname, char *data, int msg_size, unsigned int id); // ku_mq_open을 이용하는 wrapper function
void cworker_func(int id);                                                   // Convolution 계산을 위한 각각으 worker 생성
void pworker_func(int id);                                                   // MaxPooling 계산을 위한 각각으 worker 생성

volatile int size = 0;
int **worker_pids;

struct msg_matrix_3x3_st  // 3x3 matrix 전달(main->worker로)
{
    long id;
    int value[3][3];
};

struct msg_matrix_2x2_st // 2x2 matrix 전달(main->worker로)
{
    long id;
    int value[2][2];
};

struct msg_ret_st // 결과값 받아오기 (worker->main으로 )
{
    long id;
    int value;
};

void signalhandler()  // 단순 signal_interrupt 발생용
{
}

/************************************************MAIN************************************************/

int main(int argc, char *argv[])
{
    int colLen0 = 0, colLen1 = 0, colLen2 = 0;
    int X = 0, Y = 0;
    int **midMat, **endMat;

    if (argc != 2) // 인자가  2개가 아닌 경우 종료
    {
        printf("Please input 2 arguments!!\n");
        return 0;
    }

    size = atoi(argv[1]); // 정수변환

    CheckXY(size, &X, &Y); // 옳바른 input인지 확인, 이후 X, Y값 지정

    int **oriMat = (int **)malloc(sizeof(int *) * X); // X(행) * Y(열) matrix 메모리 할당, 메모리 할당 함수를 따로 만들까도
    for (int i = 0; i < X; i++)                       // 생각했는대 혹시몰라 수업ppt방식 예시대로 할당
        oriMat[i] = (int *)malloc(sizeof(int) * Y);

    makeMatrix(oriMat, X, Y); // num차원 matrix 데이터 생성
    PrintFunc(oriMat, X);  // 생성된 matrix 확인용 
    // worker_pid를 저장할 matix 생성
    worker_pids = (int **)malloc(sizeof(int *) * (X - 2));
    for (int i = 0; i < X - 2; i++)
    {
        worker_pids[i] = (int *)malloc(sizeof(int) * (X - 2));
    }

    signal(SIGUSR1, signalhandler);  // 시그널 등록

    for (int i = 0; i < X - 2; i++)
    {
        for (int j = 0; j < X - 2; j++)
        {
            int child_pid;
            if ((child_pid = fork()) == 0) // child process 미리 생성
            {
                cworker_func(i * X + j); // convollutiion worker생성
                if (i < ((X / 2) - 1) && j < ((X / 2) - 1))
                    pworker_func(i * X + j);
                return 0;
            }
            else
            {
                worker_pids[i][j] = child_pid; // 자식의 pid를 matrix로 저장해둠
            }
        }
    }

    colLen0 = X;  // 나중에 메모리 반환을 위해 기리 저장

    midMat = Convolution(&X, &Y, oriMat); // convolution된후 생성된 matrix의 주소 반환
    colLen1 = X;

    PrintFunc(midMat, X);

    endMat = MaxPooling(&X, &Y, midMat); // maxpooling된후 생성된 matrix의 주소 반환
    colLen2 = X;

    PrintFunc(endMat, X); // 최종 결과 출력

    mq_unlink("/mq_mat_3x3"); // 해당 Queue 삭제
    mq_unlink("/mq_mat_2x2");
    mq_unlink("/mq_ret");
    mq_unlink("/mq_check");

    MemFree(oriMat, colLen0); // num차원 matrix 저장공간 free
    MemFree(midMat, colLen1);
    MemFree(endMat, colLen2);

    return 0;
}

/*******************************************************************************************************/

mqd_t ku_mq_open(char *mqname, int msg_size) // message queue wrapper funtion
{
    struct mq_attr attr;
    mqd_t mq_des;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = msg_size;
    mq_des = mq_open(mqname, O_CREAT | O_RDWR, 0600, &attr);
    if (mq_des < 0)
    {
        perror("mq_open()");
        exit(0);
    }
    return mq_des;
}

void ku_mq_send(char *mqname, char *data, int msg_size, unsigned int id) // send용 wrapper 함수
{
    mqd_t mq_des = ku_mq_open(mqname, msg_size);
    if (mq_send(mq_des, (char *)&data, msg_size, id) == -1)
    {
        perror("mq_send");
        exit(0);
    }
    mq_close(mq_des);
}

void ku_mq_receive(char *mqname, char *data, int msg_size, unsigned int id) // receive용 wrapper 함수
{
    unsigned int msgid;
    mqd_t mq_des = ku_mq_open(mqname, msg_size);

    if (mq_receive(mq_des, (char *)&data, msg_size, &msgid) == -1)
    {
        perror("mq_receive");
        exit(0);
    }

    mq_close(mq_des);
}

void cworker_func(int id)  // worker
{
    struct msg_matrix_3x3_st msg_matrix_3x3;
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0); // id값을 보냄
    pause(); // 정지상태로 대기 
    ku_mq_receive("/mq_mat_3x3", (char *)&msg_matrix_3x3, sizeof(struct msg_matrix_3x3_st), 0); // interrupt 발생으로 깨어남

    int convl_ret = ConvolLayer(msg_matrix_3x3.value);  // 핵심 연산후 결과값 반환

    struct msg_ret_st msg_ret = {msg_matrix_3x3.id, convl_ret};  // 결과값 생성
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0); // 결과값 재전송
    pause(); // 대기
    ku_mq_send("/mq_ret", (char *)&msg_ret, sizeof(struct msg_ret_st), 0);
}
// cworker_func과 같은 논리 
void pworker_func(int id)
{
    struct msg_matrix_2x2_st msg_matrix_2x2;
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0);
    pause();
    ku_mq_receive("/mq_mat_2x2", (char *)&msg_matrix_2x2, sizeof(struct msg_matrix_2x2_st), 0);

    int mpl_ret = PoolingLayer(msg_matrix_2x2.value);

    struct msg_ret_st msg_ret = {msg_matrix_2x2.id, mpl_ret};
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0);
    pause();
    ku_mq_send("/mq_ret", (char *)&msg_ret, sizeof(struct msg_ret_st), 0);
}

void CheckXY(int num, int *ch_X, int *ch_Y)
{
    if ((num & 1) || num <= 0 || num == 2)
    { // 홀수이거나, 2거나, 0이하라면 종료 ex) a = 2i + 2 (i = 1, 2, 3...)
        printf("4 이상의 짝수만 입력가능\n");
        exit(0);
    }
    else
    {
        *ch_X = num; // 수업에서 X, Y가 같은 정방행렬이라 가정하였음.
        *ch_Y = num;
    }
}

int **Convolution(int *x, int *y, int **mat) // convolution 곱을 수업때 지정한 filter를 이용하여 진행후 queue보내기
{
    int m = (*x);
    int n = (*y);
    int newLen = n - 2;
    int check = 0;

    int **resultMat = (int **)malloc(sizeof(int *) * (newLen)); // x(행) * y(열) 차원 행렬 저장공간 생성
    for (int c = 0; c < (newLen); c++)
        resultMat[c] = (int *)malloc(sizeof(int) * (newLen));

    for (int i = 0; i < newLen; i++)
    {
        for (int j = 0; j < newLen; j++)
        {  // worker들이 생성되었음을 확인 (동기화 과정 )
            ku_mq_receive("/mq_check", (char *)&check, sizeof(int), 0);
            printf("come %d", check);
        }
    }

    for (int i = 0; i < newLen; i++) // newLen * newLen 만큼 반복
    {
        for (int j = 0; j < newLen; j++)
        {
            struct msg_matrix_3x3_st msg_matrix_3x3; // 정보를 보낼 구조체 생성
            msg_matrix_3x3.id = i * m + j;  // id값 저장 
            for (int a = 0; a < 3; a++)
            {
                for (int b = 0; b < 3; b++)
                {
                    msg_matrix_3x3.value[a][b] = mat[i + a][j + b];  // value값 저장 
                }
            }
            ku_mq_send("/mq_mat_3x3", (char *)&msg_matrix_3x3, sizeof(struct msg_matrix_3x3_st), 0); // 구조체 전송
            kill(worker_pids[i][j], SIGUSR1); // signal 전송 -> worker가 pause()에서 깨어남
        }
    }
    // 다시 동기화 과정
    for (int i = 0; i < newLen; i++)
    {
        for (int j = 0; j < newLen; j++)
        {
            ku_mq_receive("/mq_check", (char *)&check, sizeof(int), 0);
        }
    }
    // 결과를 받는
    for (int i = 0; i < newLen; i++)
    {
        for (int j = 0; j < newLen; j++)
        {
            struct msg_ret_st msg_ret;
            kill(worker_pids[i][j], SIGUSR1); // signal을 먼저 보내 깨운후에 
            ku_mq_receive("/mq_ret", (char *)&msg_ret, sizeof(struct msg_ret_st), 0); // receiver 호출 
            resultMat[msg_ret.id / size][msg_ret.id % size] = msg_ret.value;
        }
    }
    *x = (newLen);
    *y = (newLen);

    return (int **)resultMat; // 생성된 result matrix의 포인터 반환
}

int ConvolLayer(int (*fun)[3]) // 필터
{
    int sum = 0;

    for (int i = 0; i < 3; i++)
    { // 왼쪽 위 첫 모퉁이 시작점
        for (int j = 0; j < 3; j++)
        {
            if (i == 1 && j == 1)
            {
                sum += (fun[i][j] * 8); // filter의 정중앙일 경우 X 8
            }
            else
            {
                sum += (fun[i][j] * -1); // filter의 정중앙이 아닐경우 X -1
            }
        }
    }

    return sum;
}

int **MaxPooling(int *x, int *y, int **mat)
{
    int m = (*x);
    int n = (*y);
    int newLen = (m / 2);
    int check = 0;

    int **resultMat = (int **)malloc(sizeof(int *) * newLen); // x(행) * y(열) 차원 행렬 저장공간 생성
    for (int c = 0; c < newLen; c++)
        resultMat[c] = (int *)malloc(sizeof(int) * newLen);

    for (int i = 0; i < newLen - 1; i++)
    {
        for (int j = 0; j < newLen - 1; j++)
        {
            ku_mq_receive("/mq_check", (char *)&check, sizeof(int), 0);
        }
    }

    for (int i = 0; i < newLen - 1; i++)
    {
        for (int j = 0; j < newLen - 1; j++)
        {
            struct msg_matrix_2x2_st msg_matrix_2x2;
            msg_matrix_2x2.id = i * m + j;
            for (int a = 0; a < 2; a++)
            {
                for (int b = 0; b < 2; b++)
                {
                    msg_matrix_2x2.value[a][b] = mat[2 * i + a][2 * j + b];
                }
            }
            ku_mq_send("/mq_mat_2x2", (char *)&msg_matrix_2x2, sizeof(struct msg_matrix_2x2_st), 0);
            kill(worker_pids[i][j], SIGUSR1);
        }
    }

    for (int i = 0; i < newLen - 1; i++)
    {
        for (int j = 0; j < newLen - 1; j++)
        {
            ku_mq_receive("/mq_check", (char *)&check, sizeof(int), 0);
        }
    }

    for (int i = 0; i < newLen - 1; i++)
    {
        for (int j = 0; j < newLen - 1; j++)
        {
            struct msg_ret_st msg_ret;
            kill(worker_pids[i][j], SIGUSR1);
            ku_mq_receive("/mq_ret", (char *)&msg_ret, sizeof(struct msg_ret_st), 0);
            resultMat[msg_ret.id / size][msg_ret.id % size] = msg_ret.value;
        }
    }

    *x = newLen;
    *y = newLen;

    return (int **)resultMat; // 행의 길이를 반환
}

int PoolingLayer(int (*fun)[2])
{
    int max = fun[0][0];

    for (int i = 0; i < 2; i++) // 2 X 2 size의 pooling Layer
    {
        for (int j = 0; j < 2; j++)
        {
            if (fun[i][j] > max)
                max = fun[i][j];
        }
    }
    return max;
}

int **ResultMat(int x, int y, int (*func)(void))
{
    int **mat = (int **)malloc(sizeof(int *) * (x)); // x(행) * y(열) 차원 행렬 저장공간 생성
    for (int i = 0; i < (x); i++)
        mat[i] = (int *)malloc(sizeof(int) * (y));

    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < y; j++)
        {
            mat[i][j] = func();
        }
    }

    return mat; // matrix 시작 주소 반환
}

void PrintFunc(int **mat, int len) // 출력함수
{
    int i, j;
    for (i = 0; i < len; i++)
    {
        for (j = 0; j < len; j++)
        {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

void MemFree(int **mat, int len)
{
    for (int i = 0; i < len; i++) // num차원 행렬 저장공간 free
        free(mat[i]);
    free(mat);
}
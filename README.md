# -Parallel-CNN
## 컴퓨터공학 포토폴리오 김지우
#### 진현욱 교수님의 시스템프로그래밍을 신청하여 한학기 동안 수강하였습니다.
![KakaoTalk_20201224_122503724](https://user-images.githubusercontent.com/60593969/103407755-23f5dd00-4ba3-11eb-9cd0-96cb4f1c90d7.jpg)   
리눅스를 너무 공부해보고 싶었는대 아직 다전공을 성공하지 못하였기에 우선 일선으로 수강

> Blog 후기 : https://blog.naver.com/zbqmgldjfh/222123067831

## Parallel CNN 설계
> Parallel CNN중 일부만 구현, 코드 설명은 후반부
### - Convolution layer 구현
### - Pooling layer 구현
<img src = "https://user-images.githubusercontent.com/60593969/103480651-c5399900-4e18-11eb-8659-4f44440d67bc.jpg" width="700px">

## 과정
<img src = "https://user-images.githubusercontent.com/60593969/103772342-b814e800-506c-11eb-83ee-c4cc023a6d23.png" width="700px">

#### 1) 필요한 worker의 수를 파악하고 먼저 worker_pool을 생성한다.    
#### 2) Parent process에서 worker가 준비되었는지 확인후(동기화), 각 worker들에게 id값을 전달한다.   
#### 3) 각 worker들은 id값을 받고 Filter르 사용하여 연산한다. 이후 대기   
#### 4) Parent process에서 각 worker들에게 signal 전달   
#### 5) worker들이 Parent process에게 결과값 전송   
#### 6) 이를 Pooling layer에서도 반복   
#### 7) 최종 결과를 Parent process에서 받은후 output 출력   

## Convolutional layer
<img src = "https://user-images.githubusercontent.com/60593969/103774701-5787aa00-5070-11eb-9a7c-a30e6460ef27.jpg" width="650px">

#### 1) NxN matrix에서 3x3의 지정된 filter를 이용하여 Convolution연산을 진행함 
#### 2) result_matrix는 (n-2)x(n-2) 가 된다. 이는 필요한 Cworker의 수와 동일.
             
## Pooling layer
<img src = "https://user-images.githubusercontent.com/60593969/103774695-56ef1380-5070-11eb-9e91-dc8629a76928.jpg" width="650px">

#### 1) max-pooling 연산을 해줌. 각 사이즈는 2x2로 고정
#### 2) input_matrix가 nxn이면 output으로는 (n/2)x(n/2)가 필요. 이는 필요한 Pworker의 수와 동일.

## ID값
<img src = "https://user-images.githubusercontent.com/60593969/103899247-7736d500-5139-11eb-94c8-522326cbc2b9.png" width="650px">

**ID값은 각 원소의 행과 열의 index를 이용하여 고유한 값을 부여하였습니다.**

## Message Structure

**데이터 송수신을 위한 구조체 선언**

```C
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
```
## Message Queue API
   -> POSIX Message Queue 이용    
   -> 이러한 IPC 사용을 위해서 multi_process로 구성

```C
// 매번 mq_open을 하려면 attr값을 넘겨야 하는대 이에대한 반복을 줄이기 위해 wrapper함수를 만듬
mqd_t ku_mq_open(char *mqname, int msg_size) // message queue wrapper funtion
{
    struct mq_attr attr;
    mqd_t mq_des;
    attr.mq_maxmsg = 10; // 메시지큐의 최대수는 10개
    attr.mq_msgsize = msg_size;
    mq_des = mq_open(mqname, O_CREAT | O_RDWR, 0600, &attr);
    if (mq_des < 0)
    {
        perror("mq_open()");
        exit(0);
    }
    return mq_des; // 메시지큐 id값 반환
}
// 위에서 만든 ku_mq_open을 내장한 sned함수
void ku_mq_send(char *mqname, char *data, int msg_size, unsigned int id) // send용 wrapper 함수
{
    mqd_t mq_des = ku_mq_open(mqname, msg_size); // 큐를 열고
    if (mq_send(mq_des, (char *)&data, msg_size, id) == -1) // 메시지를 보내는
    {
        perror("mq_send");
        exit(0);
    }
    mq_close(mq_des);
}

void ku_mq_receive(char *mqname, char *data, int msg_size, unsigned int id) // receive용 wrapper 함수
{
    unsigned int msgid;
    mqd_t mq_des = ku_mq_open(mqname, msg_size); // 큐를 열고

    if (mq_receive(mq_des, (char *)&data, msg_size, &msgid) == -1) // 메시지를 받는
    {
        perror("mq_receive");
        exit(0);
    }

    mq_close(mq_des);
}
```

## Signal handler
```C
void signalhandler()  // 단순 signal_interrupt 발생용
{
}
```

각 process가 동기화를 마친후 pause()를 통해 각 worker를 정지시켜둔다.   
data 통신 준비를 위해 signal을 전송하여 pause()에서 벗어나 send 또는 receive를 하도록 호출한다.    
handler는 단순히 pause()에서 벗어나기위한 용도이다.

## Worker Function
```c
void cworker_func(int id)  // worker
{ // data를 먼저 받고
    struct msg_matrix_3x3_st msg_matrix_3x3;
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0); // id값을 보냄
    pause(); // 정지상태로 대기 
    ku_mq_receive("/mq_mat_3x3", (char *)&msg_matrix_3x3, sizeof(struct msg_matrix_3x3_st), 0); // interrupt 발생으로 깨어남

    int convl_ret = ConvolLayer(msg_matrix_3x3.value);  // 핵심 연산후 결과값 반환
  // data 다시 전송
    struct msg_ret_st msg_ret = {msg_matrix_3x3.id, convl_ret};  // 결과값 생성
    ku_mq_send("/mq_check", (char *)&id, sizeof(int), 0); // 결과값 재전송
    pause(); // 대기
    ku_mq_send("/mq_ret", (char *)&msg_ret, sizeof(struct msg_ret_st), 0);
}
```

mq_check라는 message queue에 메시지를 넣음으로써 모든 worker가 준비되었다는 것 을 확인한 후에 main에서 다음 step을 진행하도록.  

동기화가 끝난후 각 worker들은 data받을 준비가 완료되었다. main이 어떤 data를 어디에 전송해야할지는 모름으로 main은 signal과 함께 data를 전송한다.    

따라서 pause()를 통해서 기다리고 있던 worker는 signal을 통해 깨어나고, 그 pause()에서 깨어난 worker가 message queue를 읽어들여 data를 처리

```C
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
```
이는 pworker_func도 위의 cworker_func와 같은 방식으로 작동한다.

## Create Workers
```C
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
```
main process의 거의 앞부분에 구현되어야 할것이다.   

부모 Process가 fork를 통해 자식 process의 pid를 알수있는데, signal을 사용해야 하기 떄문에 해당 worker의 pid를 저장할 matrix를 생성한다.

## Convolutional Layer
```C
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
```

(동기화 과정) 앞서 설명했던 cworker는 /mq_check에 data를 보낸다. 모든 worker들이 mq_check에 data를 넣은경우 모든 worker가 준비됨을 확인할 수 있고, 그 이후 convol_layer에서 matirx 연산이 진행된다.  

(3x3 행렬 전송)
미리 새성해논 행렬에 값을 전달한후, send를 요청한다. worker들이 아직 자고있기때문에 worker에 signal을 보낸다.   

(다시 동기화 과정)   

(결과를 전달받는)  먼저 kill로 signal을 보낸후 receive를 호출한다. data를 전송받고 수신을 행야하기 때문이다.

## Max Polling Layer
```C
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
```
 
이 과정은 이전의 convolutional layer의 과정과 동일다.

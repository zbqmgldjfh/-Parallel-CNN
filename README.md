# -Parallel-CNN
## 컴퓨터공학 포토폴리오 김지우
#### 진현욱 교수님의 시스템프로그래밍을 신청하여 한학기 동안 수강하였습니다.
![KakaoTalk_20201224_122503724](https://user-images.githubusercontent.com/60593969/103407755-23f5dd00-4ba3-11eb-9cd0-96cb4f1c90d7.jpg)   
아직 다전공을 성공하지 못하였기에 일선으로 수강

## Parallel CNN 설계
> Parallel CNN중 일부만 구현
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

## The Implement Plans
<img src = "https://user-images.githubusercontent.com/60593969/103774201-89e4d780-506f-11eb-8dc7-7dca770e106c.jpg" width="650px">

#### 병렬성을 위해 2-2 방식으로 진행 

>사전에 worker_process를 미리생성후 이를 활용

## Convolutional layer
<img src = "https://user-images.githubusercontent.com/60593969/103774701-5787aa00-5070-11eb-9a7c-a30e6460ef27.jpg" width="650px">

#### 1) NxN matrix에서 3x3의 지정된 filter를 이용하여 Convolution연산을 진행함 
#### 2) result_matrix는 (n-2)x(n-2) 가 된다. 이는 필요한 Cworker의 수와 동일.
             
## Pooling layer
<img src = "https://user-images.githubusercontent.com/60593969/103774695-56ef1380-5070-11eb-9e91-dc8629a76928.jpg" width="650px">

#### 1) max-pooling 연산을 해줌. 각 사이즈는 2x2로 고정
#### 2) input_matrix가 nxn이면 output으로는 (n/2)x(n/2)가 필요. 이는 필요한 Pworker의 수와 동일.



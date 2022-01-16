# mlnsdk
C++ 서버 프레임워크. header-only library.   

[mlnserver](https://github.com/lazychase/mlnserver) : Sample Server Project

openssl


# Required
* c++20
  * msvc : visual studio 2019 16.11 이상
  * linux : gcc11
* CMake [>=3.12 <= 3.18]
* Boost 1.78
* [cpprestsdk](https://github.com/microsoft/cpprestsdk)
* openSSL
* AWS SDK for C++
* MySQL Connector/C++ 8.x

### CMake
[AWS SDK for C++ 를 빌드](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup-linux.html)하려면 3.18 이하 버젼이어야 합니다.
```
 (minimum version 3.2; maximum version 3.18)
```

### Boost
예전 버젼(minimum 1.72)도 현재는 가능합니다. 


### cpprestsdk
압도적으로 빠른 [simdjson](https://github.com/simdjson/simdjson) 이 있지만 simdjson 은 읽기 기능만 되고 json 문자열을 생성하는 기능은 제공하지 않기 때문에 선택에서 제외하였습니다.

json 파서의 종류는 cmake 의 컴파일 옵션을 수정하여 다른 json 라이브러리를 선택하는것도 가능합니다. 하지만 cpprestsdk 에 포함된 병렬프로세스 라이브러리인 ppltask 를 비동기 호출수단으로 사용할 것이기 때문에 cpprestsdk 는 필요합니다.


# Dependancies
* [spdlog](https://github.com/gabime/spdlog)  
* [struct_mapping](https://github.com/bk192077/struct_mapping)  
  

## 빠르게 서버 개발을 시작하기 위해서
json 문자열을 기반으로 패킷을 추가할 수 있도록 패킷 프로토콜과 파서가 준비되어 있습니다.


## 커스텀 패킷 디자인
사용자는 자신의 패킷프로토콜을 정의할 수 있습니다.
* 프로토콜 구조체
* 패킷 구조체 파싱 함수
* 패킷 핸들러 함수

## 교체가 가능한 Json 라이브러리
기본 제공하는 PacketJsonHandler 에서는 Json 타입을 템플릿 파라미터로 사용하고 있어서 사용자가 파상함수를 제공하여 원하는 라이브러리로 교체사용이 가능합니다.

## 웹소켓 지원(작성중)
한개의 서비스(서버)에서 TCP 소켓 클라이언트와 웹소켓 클라이언트를 동등한 세션으로 처리할 수 있습니다. 
* 웹소켓 세션을 바이너리 모드로 생성 중 => (TODO) 문자열 기반 세션으로 변경
* (TODO) 기본 프로토콜 및 파서에 대응하는 웹소켓세션용 파서 제작

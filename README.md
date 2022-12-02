# mlnsdk
C++ 서버 프레임워크. header-only library.   

[mlnserver](https://github.com/lazychase/mlnserver) : Sample Server Project

openssl


# Required
* c++20
  * msvc : visual studio 2019 16.11 이상
  * linux : gcc11
* CMake [>=3.12 <= 3.18]
  * [cmake 3.18 release](https://github.com/Kitware/CMake/releases/tag/v3.18.6)
* Boost 1.78
* Boost Json
* openSSL
* AWS SDK for C++
* MySQL Connector/C++ 8.x

### CMake
[AWS SDK for C++ 를 빌드](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup-linux.html)하려면 3.18 이하 버젼이어야 합니다.
```
 (minimum version 3.2; maximum version 3.18)
```

### Boost
1.78 버젼을 기준으로 작성되고 있습니다. 


### Boost Json
boost 1.75 버젼 부터 json 라이브러리가 추가되었습니다. ptree 를 사용했던 기존의 json 파싱작업은 속도가 매우 느린편이었는데, [추가된 json 라이브러리는 rapid json 에 밀리지 않는 성능을 보여줍니다.](https://www.boost.org/doc/libs/1_75_0/libs/json/doc/html/json/benchmarks.html)

# Windows
## vcpkg
* https://github.com/microsoft/vcpkg

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

## 웹소켓 지원
한개의 서비스(서버)에서 TCP 소켓 클라이언트와 웹소켓 클라이언트를 동등한 세션으로 처리할 수 있습니다. 

## Doing
기본 json 라이브러리를 boost json 으로 교체중

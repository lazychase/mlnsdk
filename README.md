# mlnsdk
C++ 서버 프레임워크. header-only library.   

[mlnserver](https://github.com/lazychase/mlnserver) : Sample Server Project




# Required
* CMake [>=3.12 <= 3.18]
* Boost 1.78
* [cpprestsdk](https://github.com/microsoft/cpprestsdk)

### CMake
개인 개발은 무조건 최신버젼을 써서 남들보다 먼저 새로운 경험(Good or Bad)을 해보자는 생각인데  [AWS SDK for C++ 를 빌드](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup-linux.html)하려면 3.18 이하 버젼이어야만 된다고 합니다.
```
 (minimum version 3.2; maximum version 3.18)
```

### Boost
예전 버젼(minimux 1.72)도 현재는 가능합니다. beast를 써서 websocket 을 지원하는 sesison 을 구현할 예정이라 앞으로는 모릅니다.


### cpprestsdk
json 라이브러리의 선택은 언제나 고민입니다. 압도적으로 빠른 [simdjson](https://github.com/simdjson/simdjson) 이 있지만 simdjson 은 읽기 기능만 되고 json 문자열을 생성하는 기능은 제공하지 않기 때문에 선택에서 제외하였습니다.

json 파서의 종류는 cmake 의 컴파일 옵션을 수정하여 다른 json 라이브러리를 선택하는것도 가능합니다. 하지만 cpprestsdk 에 포함된 병렬프로세스 라이브러리인 ppltask 를 비동기 호출수단으로 사용할 것이기 때문에 cpprestsdk 는 필요합니다.


# Dependancies
* [spdlog](https://github.com/gabime/spdlog)
### 




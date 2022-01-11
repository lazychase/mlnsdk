# mlnsdk
C++ ���� �����ӿ�ũ. header-only library.   

[mlnserver](https://github.com/lazychase/mlnserver) : Sample Server Project




# Required
* CMake [>=3.12 <= 3.18]
* Boost 1.78
* [cpprestsdk](https://github.com/microsoft/cpprestsdk)

### CMake
���� ������ ������ �ֽŹ����� �Ἥ ���麸�� ���� ���ο� ����(Good or Bad)�� �غ��ڴ� �����ε�  [AWS SDK for C++ �� ����](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup-linux.html)�Ϸ��� 3.18 ���� �����̾�߸� �ȴٰ� �մϴ�.
```
 (minimum version 3.2; maximum version 3.18)
```

### Boost
���� ����(minimux 1.72)�� ����� �����մϴ�. beast�� �Ἥ websocket �� �����ϴ� sesison �� ������ �����̶� �����δ� �𸨴ϴ�.


### cpprestsdk
json ���̺귯���� ������ ������ ����Դϴ�. �е������� ���� [simdjson](https://github.com/simdjson/simdjson) �� ������ simdjson �� �б� ��ɸ� �ǰ� json ���ڿ��� �����ϴ� ����� �������� �ʱ� ������ ���ÿ��� �����Ͽ����ϴ�.

json �ļ��� ������ cmake �� ������ �ɼ��� �����Ͽ� �ٸ� json ���̺귯���� �����ϴ°͵� �����մϴ�. ������ cpprestsdk �� ���Ե� �������μ��� ���̺귯���� ppltask �� �񵿱� ȣ��������� ����� ���̱� ������ cpprestsdk �� �ʿ��մϴ�.


# Dependancies
* [spdlog](https://github.com/gabime/spdlog)
### 




# mlnsdk
C++ ���� �����ӿ�ũ. header-only library.   

[mlnserver](https://github.com/lazychase/mlnserver) : Sample Server Project

openssl


# Required
* c++20
  * msvc : visual studio 2019 16.11 �̻�
  * linux : gcc11
* CMake [>=3.12 <= 3.18]
* Boost 1.78
* [cpprestsdk](https://github.com/microsoft/cpprestsdk)
* openSSL
* AWS SDK for C++
* MySQL Connector/C++ 8.x

### CMake
[AWS SDK for C++ �� ����](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup-linux.html)�Ϸ��� 3.18 ���� �����̾�� �մϴ�.
```
 (minimum version 3.2; maximum version 3.18)
```

### Boost
1.78 ������ �������� �ۼ��ǰ� �ֽ��ϴ�. 


### cpprestsdk
�е������� ���� [simdjson](https://github.com/simdjson/simdjson) �� ������ simdjson �� �б� ��ɸ� �ǰ� json ���ڿ��� �����ϴ� ����� �������� �ʱ� ������ ���ÿ��� �����Ͽ����ϴ�.�ؽ�Ʈ ���̴� ������ �뷮�� json ���ڿ� �Ľ��� �ʿ��� ��쿡�� simdjson ������ �߰��� �� �ֽ��ϴ�.

~~json �ļ��� ������ cmake �� ������ �ɼ��� �����Ͽ� �ٸ� json ���̺귯���� �����ϴ°͵� �����մϴ�. ������ cpprestsdk �� ���Ե� �������μ��� ���̺귯���� ppltask �� �񵿱� ȣ��������� ����� ���̱� ������ cpprestsdk �� �ʿ��մϴ�.~~  

boost 1.75 ���� ���� json ���̺귯���� �߰��Ǿ����ϴ�. ptree �� ����ߴ� ������ json �Ľ��۾��� �ӵ��� �ſ� �������̾��µ�, [�߰��� json ���̺귯���� rapid json �� �и��� �ʴ� ������ �����ݴϴ�.](https://www.boost.org/doc/libs/1_75_0/libs/json/doc/html/json/benchmarks.html)


# Dependancies
* [spdlog](https://github.com/gabime/spdlog)  
* [struct_mapping](https://github.com/bk192077/struct_mapping)  
  

## ������ ���� ������ �����ϱ� ���ؼ�
json ���ڿ��� ������� ��Ŷ�� �߰��� �� �ֵ��� ��Ŷ �������ݰ� �ļ��� �غ�Ǿ� �ֽ��ϴ�.


## Ŀ���� ��Ŷ ������
����ڴ� �ڽ��� ��Ŷ���������� ������ �� �ֽ��ϴ�.
* �������� ����ü
* ��Ŷ ����ü �Ľ� �Լ�
* ��Ŷ �ڵ鷯 �Լ�

## ��ü�� ������ Json ���̺귯��
�⺻ �����ϴ� PacketJsonHandler ������ Json Ÿ���� ���ø� �Ķ���ͷ� ����ϰ� �־ ����ڰ� �Ļ��Լ��� �����Ͽ� ���ϴ� ���̺귯���� ��ü����� �����մϴ�.

## ������ ����
�Ѱ��� ����(����)���� TCP ���� Ŭ���̾�Ʈ�� ������ Ŭ���̾�Ʈ�� ������ �������� ó���� �� �ֽ��ϴ�. 

## Doing
�⺻ json ���̺귯���� boost json ���� ��ü��

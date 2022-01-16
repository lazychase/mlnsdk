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
���� ����(minimum 1.72)�� ����� �����մϴ�. 


### cpprestsdk
�е������� ���� [simdjson](https://github.com/simdjson/simdjson) �� ������ simdjson �� �б� ��ɸ� �ǰ� json ���ڿ��� �����ϴ� ����� �������� �ʱ� ������ ���ÿ��� �����Ͽ����ϴ�.

json �ļ��� ������ cmake �� ������ �ɼ��� �����Ͽ� �ٸ� json ���̺귯���� �����ϴ°͵� �����մϴ�. ������ cpprestsdk �� ���Ե� �������μ��� ���̺귯���� ppltask �� �񵿱� ȣ��������� ����� ���̱� ������ cpprestsdk �� �ʿ��մϴ�.


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

## ������ ����(�ۼ���)
�Ѱ��� ����(����)���� TCP ���� Ŭ���̾�Ʈ�� ������ Ŭ���̾�Ʈ�� ������ �������� ó���� �� �ֽ��ϴ�. 
* ������ ������ ���̳ʸ� ���� ���� �� => (TODO) ���ڿ� ��� �������� ����
* (TODO) �⺻ �������� �� �ļ��� �����ϴ� �����ϼ��ǿ� �ļ� ����

## 1. cppSummaryGenerator安装步骤

```
sudo apt-get install zlib1g-dev unzip cmake gcc g++ libtinfo5 nodejs 
npm i --silent svf-lib --prefix ${HOME}
git clone https://github.com/shuangxiangkan/CallerSensitive.git
source ./env.sh
cmake . && make
```

## 1. 输入以下命令，后两个参数第一个为根据java生成的caller的json文件，第二个为调用的cpp文件编译后的.ll文件，将会生成一个cppSummary.json的文件
```
./bin/summary -ander caller.json  callee.ll
```

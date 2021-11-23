## 计网传输层实验-1

### 1. 代码说明
- 源目录：
  - 初始停等协议
  - Go Back N 实现
  - 选择重传实现
```
src
├── altBit.c
├── goBackN.c
└── selectiveRepeat.c
```

### 2. 测试脚本
```
test
└── script.py
```

```
#config
config = {
    'Message_num': 5,
    'Loss_Prob': 0,
    'Corrupt_Prob': 0,
    'Interval': 10,
    'Debug_Level': 0
}
```

### 3. 编译&运行方式
```
cmake CMakeLists.txt
make
cd test
python script.py
```

即可看到测试结果
```
[altBit]: 76.03789500000002ms
[goBackN]: 72.15546399999998ms
[selectiveRepeat]: 70.86132033333332ms
```
> 为在同一环境下测试，将随机数种子设为 1 `srand(1)`

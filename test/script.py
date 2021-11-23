import os
import numpy as np
import subprocess
import re


protocol_list = ['altBit', 'goBackN', 'selectiveRepeat']
Compile_PATH = "../Compile/"

config = {
    'Message_num': 10,
    'Loss_Prob': 0,
    'Corrupt_Prob': 0,
    'Interval': 10,
    'Debug_Level': 0
}

args = list(config.values())
args = [str(i) for i in args]
prog = re.compile(r'(?<=at time )[0-9|.]*')

def test_time(protocol, args):
    file_path = os.path.join(Compile_PATH, protocol)
    command_list = [file_path]
    command_list.extend(args)
    proc = subprocess.Popen( command_list, stdout=subprocess.PIPE )
    stdout, _ = proc.communicate()
    stdout = stdout.decode("utf-8")
    time = prog.findall(stdout)[0]
    # print(f'「{protocol}」: {time}')
    return time

def multi_test(N):
    time_list = []
    for protocol in protocol_list:
        for i in range(N):
            time = test_time(protocol, args)
            time_list.append(float(time))
        avg_time = np.mean(time_list)
        print(f'[{protocol}]: {avg_time}ms')
        
        
if __name__ == "__main__":
    multi_test(10)
            
    

import os
import unittest
from queue import Queue
from interpreter_gui import Action, interpreter
from sparser import DSLSparser

class AutoTest(unittest.TestCase):
    def setUp(self):
        # 设置测试目录和输入输出目录
        self.test_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../test')
        self.input_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../input')
        self.output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../output')
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

    def test_scripts(self):
        for filename in os.listdir(self.test_dir):
            if filename.endswith('.dsl'):
                # 获取脚本文件路径、对应的输入和输出文件路径
                script_file = os.path.join(self.test_dir, filename)
                input_file = os.path.join(self.input_dir, filename.replace('.dsl', '.in'))
                output_file = os.path.join(self.output_dir, filename.replace('.dsl', '.out'))

                if not os.path.exists(input_file):
                    print(f"Input file {input_file} does not exist.")
                    continue

                print(f"Processing script: {script_file}")
                print(f"Using input file: {input_file}")
                print(f"Generating output file: {output_file}")

                # 初始化解析器和Action
                parser = DSLSparser()
                input_queue = Queue()
                output_queue = Queue()
                action = Action(parser, input_queue, output_queue, True)

                # 加载脚本文件，初始化步骤
                action.get_script([script_file])
                action.fill_stepDic()
                action.initialize_step()

                # 读取输入文件并写入输入队列
                with open(input_file, 'r', encoding='utf-8') as f:
                    for line in f:
                        input_queue.put(line.strip())

                # 执行脚本并捕获输出
                outputs = []
                while not action.isOver:
                    action.execute_script()
                    while not output_queue.empty():
                        outputs.append(output_queue.get())
                        
                # 清空输出文件内容
                open(output_file, 'w').close()
                
                # 将输出保存到对应的输出文件
                with open(output_file, 'w', encoding='utf-8') as f:
                    for output in outputs:
                        f.write(output + '\n')

                print(f"Output written to: {output_file}")

if __name__ == '__main__':
    unittest.main()

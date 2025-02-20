import threading
import time
import sys
import re
import os
from sparser import DSLSparser

class InputThread(threading.Thread):
    """
    用于获取用户输入的线程
    """
    def __init__(self, prompt):
        super().__init__()
        self.prompt = prompt
        self.input_text = None

    def run(self):
        try:
            self.input_text = input(self.prompt)
        except EOFError:
            pass

def input_with_timeout(x):
    """
    获取输入并计时，当超时时告知用户退出输入

    :param x: 超时时间（秒）
    :return: 用户输入的字符串，或 None 如果超时
    """
    input_prompt = ""
    x = 10
    input_thread = InputThread(input_prompt)
    input_thread.start()
    input_thread.join(x)

    if input_thread.is_alive():
        # 如果线程仍在运行，说明超过了x秒
        print("回答超时，请按回车键退出程序")
        sys.exit(1)

    user_input = input_thread.input_text
    return user_input

class Action:
    """
    脚本执行类
    """
    def __init__(self, parser: DSLSparser):
        self.tree = []  # 存储语法树
        self.parser = parser  # 解析器实例
        self.current_step = None  # 当前执行步骤
        self.var = {}  # 变量表
        self.speak = "" # 输出的语音
        self.stepId = 0 # 步骤ID
        self.stepDic = {}  # 基于语法树能够生成的可供执行的步骤
        self.isOver = False  # 是否结束
        self.waitTime = 0  # wait的时间
        self.input = ""  # 用户的输入
        self.has_next_step = False  # 是否有下一步

    def get_script(self, file_list: list):
        """
        获得对应的脚本语法树

        :param file_list: 文件列表
        """
        for file in file_list:
            self.parser.parse_file(file)
        self.tree = self.parser.tree
        self.var = self.parser.var


    def initialize_step(self):
        """
        初始化步骤
        """
        if not self.tree:
            print("语法树为空，无法初始化步骤！")
            self.isOver = True
            return
        self.current_step = self.tree[0]

    def fill_stepDic(self):
        """
        填充步骤字典
        """
        for item_dic in self.tree:
            self.stepDic[item_dic["Step"]] = self.stepId
            self.stepId += 1
            
    def handle_numeric(self, value_str, line):
        """
        将字符串转换为数字
        
        :param value_str: 字符串
        :param line: 当前行
        """
        try:
            if '.' in value_str:
                return float(value_str)
            else:
                return int(value_str)
        except:
            self.exception(line, "Invalid numeric value")
            
    def is_number(self, s):
        """
        判断字符串是否为数字
        
        :param s: 字符串
        """
        return bool(re.match(r'^-?\d+(\.\d+)?$', s))
    
    def update_variable(self, line):
        """
        解析变量赋值语句并插入到变量表
        
        :param line: 当前行
        """
        match = re.match(r'^\$(\w+)\s*=\s*(.*)$', line.strip())

        varname, value_expr = match.groups()
        # 替换表达式中的变量
        def replace_var(match):
            var_name = match.group(1)
            
            if var_name in self.var:
                var_value = self.var[var_name]
                if var_value is None:
                    return "0"  # 将 None 值替换为 0
                if isinstance(var_value, (int, float)):
                    return str(var_value)
                else:
                    return var_value
            else:
                print(f"未定义的变量：{var_name}")
        # 替换表达式中的变量
        value_expr = re.sub(r'\$(\w+)', replace_var, value_expr)

        # 判断是否需要计算表达式
        needs_evaluation = any(op in value_expr for op in ['+', '-', '*', '/', '(', ')', '$'])
        
        try:
            if not needs_evaluation:
                if self.is_number(value_expr):
                    # 如果是数字或浮点数直接赋值
                    value = self.handle_numeric(value_expr, line)
                else:
                    value = value_expr
            else:
                # 使用 eval 计算表达式的值
                value = eval(value_expr, {}, self.var)
        except Exception as e:
            print(f"计算表达式时出错：{e}")
            return
        # 将变量插入到变量表
        self.var[varname] = value
        
    def execute_script(self):
        """
        执行具体脚本
        """
        # 如果当前步骤没有明确的下一步，则自动跳转到第一个步骤
        while self.current_step is not None:
            self.has_next_step = False
            for item_dic in self.current_step["proc"]:
                for command, args in item_dic.items():
                    # 如果有 Run 指令
                    if command == "Run":  
                        self.has_next_step = True
                        break
                if self.has_next_step:
                    break
                
            for item_dic in self.current_step["proc"]:
                for command, args in item_dic.items():
                    # 如果匹配到关键词Speak
                    if command == "Speak":  
                        self.speak = ""
                        # 根据脚本拼接需要输出的Speak
                        for part in args:  
                            if part["type"] == "variable":
                                self.speak += str(self.var.get(part["value"], ""))
                            elif part["type"] == "string":
                                self.speak += part["value"]
                        print(self.speak) 
                    # 如果匹配到关键词Listen
                    elif command == "Listen": 
                        listen_var = args
                        self.input = input_with_timeout(self.waitTime)
                        if self.input is None:
                            print("您已长时间未回答，程序已退出")
                            self.isOver = True
                            return
                        self.var[listen_var] = self.input
                    # 如果匹配到关键词Switch
                    elif command == "Switch":  
                        switch_var = args[0]
                        switch_value = self.var.get(switch_var, "")
                        # 遍历所有条件
                        for condition in args[1]:
                            if "Branch" in condition:
                                branch = condition["Branch"]
                                if re.search(branch["condition"], switch_value):
                                    self.execute_run([branch["next"]])
                                    return
                            elif "Default" in condition:
                                default = condition["Default"]
                                self.execute_run([default["next"]])
                                return 
                    # 如果匹配到关键词Run
                    elif command == "Run":  
                        self.execute_run(args)
                        return
                    # 如果匹配到关键词Update
                    elif command == "Update":  
                        update_expr = args
                        self.update_variable(update_expr)
            # 如果当前步骤没有明确的下一步，则自动跳转到第一个步骤
            if not self.has_next_step and self.current_step["Step"] != self.tree[0]["Step"]:
                self.current_step = self.tree[0]
                time.sleep(5)
                print(f"即将跳转回主页面")


    def execute_run(self, args):
        """
        执行 Run 指令

        @param args: Run 指令的参数， 即目标 Step
        """
        if len(args) != 1:
            print("Run 指令需要一个目标 Step！")
            return
        # 执行退出指令
        next_step = args[0]
        if(next_step == "Exit"):
            self.isOver = True
            return
        # 执行跳转指令
        if next_step in self.stepDic:
            self.current_step = self.tree[self.stepDic[next_step]]
        else:
            print(f"[Run]: Step '{next_step}' 未找到！")

def interpreter(action: Action, file_list: list):
    """
    解释脚本语言，根据脚本生成树执行对应的动作

    @param action:需要执行的具体动作类
    @param file_list:脚本文件列表
    """
    action.get_script(file_list)
    if not action.tree:
        print("语法树为空，无法执行！")
        return

    action.fill_stepDic()
    action.initialize_step()
    while True:
        action.execute_script()
        
        if action.isOver:
            break
        time.sleep(0.1)
    print("您已退出程序，欢迎下次光临")

if __name__ == '__main__':
    # 获取当前文件的目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # 构建相对路径
    file_path = os.path.join(current_dir, '../test/test1.dsl')
    files = [file_path]
    act = Action(DSLSparser())
    interpreter(act, files)
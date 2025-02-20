import re
import shlex

class DSLSparser:
    def __init__(self):
        self.tree = []  # 存储语法树
        self.current_step = None  # 当前步骤
        self.line_num = 0  # 当前行号
        self.lines = []  # 存储DSL脚本的每一行
        self.var = {}  # 变量表

    def open_file(self, filename):
        """
        打开并读取文件
        
        :param filename: 进行语法分析的文件路径
        """
        with open(filename, 'r', encoding='utf-8') as file:
            return file.readlines()

    def is_linefeed(self, line):
        """
        判断是否为空行
        
        :param: 当前行
        """
        return line.strip() == ''

    def is_comment(self, line):
        """
        判断是否为注释行
        
        :param: 当前行
        """
        return line.strip().startswith('#') or line.strip().startswith('@')

    def handle_comment(self):
        """
        处理多行注释
        """
        while True:
            try:
                line = self.lines[self.line_num].strip()
                if line.endswith("@"):
                    self.line_num += 1
                    break
            except:
                print("Wrong Comment block")
                exit(0)

            self.line_num += 1

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

    def is_valid_identifier(self, identifier):
        """
        检查标识符是否有效
        
        :param identifier: 标识符
        """
        return bool(re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', identifier))

    def is_number(self, s):
        """
        检查字符串是否为数字或浮点数
        
        :param s: 字符串
        """
        return bool(re.match(r'^-?\d+(\.\d+)?$', s))

    def parse_var(self, line):
        """
        解析变量赋值语句并插入到变量表
        
        :param line: 当前行
        """
        match = re.match(r'^\$(\w+)\s*=\s*(.*)$', line.strip())
        if not match:
            self.exception(line, "Invalid variable assignment")

        varname, value_expr = match.groups()

        # 验证变量名是否合法
        if not self.is_valid_identifier(varname):
            self.exception(line, "Invalid variable name")

        # 如果存在初始赋值
        if value_expr:
            value_expr = value_expr.strip()

            # 检查是否包含非法字符或运算符
            if any(op in value_expr for op in ['+', '-', '*', '/', '(', ')']):
                self.exception(line, "Variable initialization cannot contain operations")
            # 判断值是否为数值或字符串
            if re.match(r'^-?\d+(\.\d+)?$', value_expr):  # 数值
                value = float(value_expr) if '.' in value_expr else int(value_expr)
            else:  # 字符串
                value = value_expr

            # 将变量和值插入变量表
            self.var[varname] = value
        else:
            # 如果没有赋值，则默认值为 None
            self.var[varname] = None

    def parse_step(self, line):
        """
        解析步骤定义
        
        :param line: 当前行
        """
        match = re.match(r'^Step\s+(\w+)\s*$', line.strip())
        if not match:
            self.exception(line, "Invalid Step statement")

        step_name = match.group(1)

        if not self.is_valid_identifier(step_name):
            self.exception(line, "Invalid Step name")

        self.current_step = step_name
        self.tree.append({"Step": step_name, "proc": []})
    
        # 处理步骤块        
        self.line_num += 1
        line = self.lines[self.line_num].strip()
        
        if line != "Begin":
            self.exception(line, "Expected 'Begin'")

        proc_steps = []
        while True:
            line = self.lines[self.line_num].strip()
            self.line_num += 1
    
            if self.is_linefeed(line):
                continue
    
            if self.is_comment(line):
                if line.startswith('#'):
                    continue
                if line.startswith('@'):
                    self.handle_comment()
    
            token = shlex.split(line)
            if token[0] == 'End':
                break
    
            # 处理各种指令
            if token[0] == 'Speak':
                speak_expr = ' '.join(token[1:])
                 # 为变量和字符串添加标记
                parts = speak_expr.split('+')
                marked_parts = []
                for part in parts:
                    part = part.strip()
                    if part.startswith('$'):
                        marked_parts.append({"type": "variable", "value": part[1:]})
                    else:
                        marked_parts.append({"type": "string", "value": part.strip('"')})
                proc_steps.append({"Speak": marked_parts})
                
            elif token[0] == 'Listen':
                if len(token) == 2:
                    if token[1][0] != '$':
                        self.exception(line, "Invalid Listen statement")
                        
                    listen_var = token[1][1:]
                    # 将变量存入变量表
                    if listen_var not in self.var:
                        self.var[listen_var] = None
                        
                    proc_steps.append({"Listen": listen_var})
                else:
                    self.exception(line, "Invalid Listen statement")
            elif token[0] == 'Switch':
                switch_result = self.parse_switch(token)
                proc_steps.append(switch_result)
            elif token[0] == 'Run':
                proc_steps.append({"Run": token[1]})
            elif token[0] == 'Update':
                update_expr = ' '.join(token[1:])
                if token[1][0] != '$':
                    self.exception(line, "Invalid Update statement")
                proc_steps.append({"Update": update_expr})
                
        
        # 将步骤添加到语法树中
        self.tree[-1]["proc"].extend(proc_steps)
        
        # 检查主步骤是否包含至少一个Switch语句及Exit分支
        if len(self.tree) == 1:
            has_exit_branch = any(
            "Switch" in step and any(
                "Branch" in branch and branch["Branch"]["next"] == "Exit"
                for branch in step["Switch"][1]
            )
            for step in proc_steps)        
            if not has_exit_branch:
                self.exception(line, "The first Step must contain a Switch with at least one Exit branch")
            

    def parse_switch(self, token):
        """
        解析Switch语句
        
        :param token: 当前行
        """
        
        if len(token) != 2 or not token[1].startswith('$'):
            self.exception("Invalid Switch statement: Missing or invalid variable")
    
        switch_var = token[1][1:]  # 提取变量名
        if switch_var not in self.var:
            self.exception(f"Undefined variable '{switch_var}' in Switch statement")
        
        result_token = []
        default_exist = False

        line = self.lines[self.line_num].strip()
        self.line_num += 1
        if line != 'Begin':
            self.exception(line, "Expected 'Begin'")

        while True:
            line = self.lines[self.line_num].strip()
            self.line_num += 1

            if self.is_linefeed(line):
                continue

            if self.is_comment(line):
                if line.startswith('#'):
                    continue
                if line.startswith('@'):
                    self.handle_comment()

            token = shlex.split(line)
            if token[0] == 'End':
                break

            if token[0] == 'Branch':
                result_token.append(self.parse_branch(token))
            elif token[0] == 'Default':
                default_exist = True
                result_token.append(self.parse_default(token))
                break
            else:
                self.exception(line, "Invalid Switch statement")

        if not default_exist:
            print(f"Default branch missing in Switch at line {self.line_num}")
            exit(0)
        return {"Switch":[switch_var, result_token]}
    
    def parse_branch(self, token):
        """
        解析Branch语句
        
        :param token: 当前行
        """
        match = re.match(r'^Branch\s+(\S+)\s+(\S+)\s+(\S+)$', ' '.join(token))
        if not match:
            self.exception("Invalid Branch statement")

        condition, action, next_step = match.groups()

        if action not in ['Run', 'Exit']:
            self.exception("Invalid Branch action")

        return {"Branch": {"condition": condition, "action": action, "next": next_step}}

    def parse_default(self, token):
        """
        解析Default语句
        
        :param token: 当前行
        """
        match = re.match(r'^Default\s+(\S+)\s+(\S+)$', ' '.join(token))
        if not match:
            self.exception("Invalid Default statement")

        action, next_step = match.groups()

        return {"Default":{"action": action, "next": next_step}}

    def exception(self, line=0, message=""):
        """语法错误处理
        
        :param line: 当前行
        :param message: 错误信息
        """
        print(f"Syntax error at line {self.line_num}: {message}")
        print(line.strip())
        exit(0)

    def parse_file(self, filename):
        """
        解析DSL文件并生成语法树
        
        :param filename: 文件路径
        """
        self.lines = self.open_file(filename)
        if len(self.lines) == 0:
            self.exception("Open file failed")
        while self.line_num < len(self.lines):
            line = self.lines[self.line_num].strip()
            if not line or self.is_linefeed(line):
                self.line_num += 1
                continue

            if self.is_comment(line):
                if line.startswith('@'):
                    self.handle_comment()
                self.line_num += 1
                continue

            if line.startswith('Step'):
                self.parse_step(line)
            elif line.startswith('$'):
                self.parse_var(line)
            else:
                self.exception(line)

            self.line_num += 1


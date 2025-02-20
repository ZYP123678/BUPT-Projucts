import unittest
from sparser import DSLSparser

class TestDSLSparser(unittest.TestCase):
    def setUp(self):
        self.parser = DSLSparser()
        self.parser.lines = [
            "Step welcome",
            "Begin",
            "Speak $name + '你好，请问有什么可以帮您?'",
            "Listen $iput",
            "Update $name = 2",
            "Switch $input",
            "Begin",
            "Branch '你好' Run nihaoProc",
            "Branch '你不好' Run nibuhaobyeProc",
            "Branch '退出' Run Exit",
            "Default Run defaultProc",
            "End",
            "End",
            "Step nihaoProc",
            "Begin",
            "Speak '你好！！！'",
            "End",
            "Step nibuhaoProc",
            "Begin",
            "Speak '你不好！！！'",
            "End",
            "Step defaultProc",
            "Begin",
            "Speak '不好意思，请再说一遍'",
            "Listen $input",
            "Switch $input",
            "Begin",
            "Branch '你好' Run nihaoProc",
            "Branch '你不好' Run nibuhaobyeProc",
            "Branch '退出' Run Exit"
            "Default Run defaultProc",
            "End",
            "End"
        ]
        self.parser.var = {"name": "1"}
        self.parser.line_num = 0

    def test_parse_file(self):
        self.parser.parse_file("./test/test_sparser.dsl")
        expected_tree = [
            {
                'Step': 'welcome',
                'proc': [
                    {'Speak': [
                        {"type": "variable", "value": "name"},
                        {"type": "string", "value": "你好，请问有什么可以帮您?"}]},
                    {'Listen': 'input'},
                    {'Switch':['input', [
                        {'Branch': {'condition': '你好', 'action': 'Run', 'next': 'nihaoProc'}},
                        {'Branch': {'condition': '你不好', 'action': 'Run', 'next': 'nibuhaoProc'}},
                        {'Branch': {'condition': '退出', 'action': 'Run', 'next': 'Exit'}},
                        {'Default': {'action': 'Run', 'next': 'defaultProc'}}
                    ]]},
                ]
            },
            {
                'Step': 'nihaoProc',
                'proc': [
                    {'Speak': [{'type': 'string', 'value': '你好！！！'}]}
                ]
            },
            {
                'Step': 'nibuhaoProc',
                'proc': [
                    {'Speak': [{'type': 'string', 'value': '你不好！！！'}]}
                ]
            },
            {
                'Step': 'defaultProc',
                'proc': [
                    {'Speak': [{'type': 'string', 'value': '不好意思，请再说一遍'}]},
                    {'Listen': 'input'},
                    {'Switch': ['input', [
                        {'Branch': {'condition': '你好', 'action': 'Run', 'next': 'nihaoProc'}},
                        {'Branch': {'condition': '你不好', 'action': 'Run', 'next': 'nibuhaoProc'}},
                        {'Branch': {'condition': '退出', 'action': 'Run', 'next': 'Exit'}},
                        {'Default': {'action': 'Run', 'next': 'defaultProc'}}
                    ]]}
                ]
            }
        ]
        print("Constructed Syntax Tree:", self.parser.tree)  # 打印构建的语法树
        print(self.parser.var)  # 打印变量表
        self.assertEqual(self.parser.tree, expected_tree)

    #def test_invalid_step(self):
    #    with self.assertRaises(SystemExit):
    #        self.parser.parse_step("Invalid step")

    #def test_invalid_speak_statement(self):
    """    self.parser.lines = [
            "Step welcome",
            "Begin",
            "Speak $name + '你好，请问有什么可以帮您?' +",
            "End"
        ]
        self.parser.line_num = 1
        with self.assertRaises(SystemExit):
            self.parser.parse_step(self.parser.lines[0])"""

if __name__ == '__main__':
    unittest.main()
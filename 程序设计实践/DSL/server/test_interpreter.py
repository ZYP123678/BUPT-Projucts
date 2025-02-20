import unittest
from interpreter import Action, interpreter
from sparser import DSLSparser

class TestInterpreter(unittest.TestCase):
    def setUp(self):
        self.parser = DSLSparser()
        self.action = Action(self.parser)
    # 检查是否能够正确将语法树和变量表填充
    def test_get_script(self):
        self.action.get_script(['./test/test1.dsl'])
        self.assertIsNotNone(self.action.tree)
        self.assertIsNotNone(self.action.var)
    # 检查是否能够正确初始化当前步骤
    def test_initialize_step(self):
        self.action.get_script(['./test/test1.dsl'])
        self.action.initialize_step()
        self.assertIsNotNone(self.action.current_step)
    # 检查是否能够正确填充步骤字典
    def test_fill_stepDic(self):
        self.action.get_script(['./test/test1.dsl'])
        self.action.fill_stepDic()
        self.assertIn('Welcome', self.action.stepDic)
        print(self.action.stepDic)
    # 检查是否能够正确转换数字
    def test_handle_numeric(self):
        self.assertEqual(self.action.handle_numeric('123', 1), 123)
        self.assertEqual(self.action.handle_numeric('123.45', 1), 123.45)
    # 检查是否能够正确判断是否为数字
    def test_is_number(self):
        self.assertTrue(self.action.is_number('123'))
        self.assertTrue(self.action.is_number('123.45'))
        self.assertFalse(self.action.is_number('abc'))
    # 检查是否能够正确更新变量
    def test_update_variable(self):
        self.action.var = {'a': 1, 'b': 2}
        self.action.update_variable('$c = $a + $b')
        self.assertEqual(self.action.var['c'], 3)
    # 检查是否能够正确执行步骤
    def test_execute_run(self):
        self.action.get_script(['./test/test1.dsl'])
        self.action.fill_stepDic()
        self.action.initialize_step()
        self.action.execute_run(['Billing'])
        self.assertEqual(self.action.current_step['Step'], 'Billing')
    # 检查是否能够正确执行脚本
    def test_interpreter(self):
        self.action.get_script(['./test/test1.dsl'])
        interpreter(self.action, ['./test/test1.dsl'])
        self.assertTrue(self.action.isOver)

if __name__ == '__main__':
    unittest.main()
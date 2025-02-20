import tkinter as tk
from tkinter import filedialog, scrolledtext
from queue import Queue, Empty
from interpreter_gui import Action, interpreter
from sparser import DSLSparser
import threading
import os


class DSLGUI:
    def __init__(self, root):
        """
        初始化 DSLGUI 类

        :param root: Tkinter 根窗口
        """
        self.root = root
        self.root.title("客服机器人")

        self.input_queue = Queue()
        self.output_queue = Queue()

        self.create_widgets()

    def create_widgets(self):
        """
        创建 GUI 元素
        """
        self.text_display = scrolledtext.ScrolledText(self.root, wrap=tk.WORD, width=60, height=20)
        self.text_display.grid(row=0, column=0, padx=10, pady=10, columnspan=2)

        self.input_entry = tk.Entry(self.root, width=50)
        self.input_entry.grid(row=1, column=0, padx=10, pady=10)
        self.input_entry.bind("<Return>", lambda event: self.submit_input())  # 绑定回车键

        self.load_button = tk.Button(self.root, text="加载脚本", command=self.load_script)
        self.load_button.grid(row=2, column=0, padx=10, pady=10)

        self.run_button = tk.Button(self.root, text="运行脚本", command=self.run_script)
        self.run_button.grid(row=2, column=1, padx=10, pady=10)

        # 设置文本框标签，控制用户输入和输出对齐
        self.text_display.tag_configure("left", justify=tk.LEFT)   # 后台回复（左对齐）
        self.text_display.tag_configure("user", foreground="blue", justify=tk.LEFT)  # 用户输入（蓝色，左对齐）

    def submit_input(self):
        """
        将用户输入放入队列并在界面显示
        """
        user_input = self.input_entry.get()
        if user_input.strip():  # 如果用户输入不为空
            self.add_message(user_input, user=True)  # 显示用户输入
            self.input_queue.put(user_input)  # 将输入添加到队列
        self.input_entry.delete(0, tk.END)  # 清空输入框

    def add_message(self, text, user=False):
        """
        向文本框添加消息，区分用户输入和输出。
        """
        # 插入用户输入或后台输出
        if user:
            message = f"<< {text}"
            self.text_display.insert(tk.END, message + "\n", "user")  # 用户输入蓝色，左对齐
        else:
            message = f">> {text}"
            self.text_display.insert(tk.END, message + "\n", "left")   # 后台输出左对齐

        self.text_display.see(tk.END)  # 滚动到最新消息

    def load_script(self):
        """
        自动扫描 test 文件夹加载脚本文件
        """
        test_folder = os.path.join(os.path.dirname(__file__), "../test")
        if not os.path.exists(test_folder):
            os.makedirs(test_folder)
        scripts = [f for f in os.listdir(test_folder) if f.endswith(".dsl")]

        if not scripts:
            self.text_display.insert(tk.END, "test 文件夹中没有找到脚本文件。\n")
            return

        # 显示脚本选择菜单
        file_path = filedialog.askopenfilename(initialdir=test_folder, filetypes=[("DSL Scripts", "*.dsl")])
        if file_path:
            self.file_path = file_path
            self.text_display.insert(tk.END, f"已加载脚本：{file_path}\n")

    def run_script(self):
        """
        启动脚本解释器线程
        """
        if not hasattr(self, 'file_path'):
            self.text_display.insert(tk.END, "请先加载脚本文件！\n")
            return
        self.action = Action(DSLSparser(), self.input_queue, self.output_queue)
        thread = threading.Thread(target=interpreter, args=(self.action, [self.file_path]), daemon=True)
        thread.start()
        self.poll_output()

    def poll_output(self):
        """
        检查输出队列并更新显示
        """
        try:
            while True:
                message = self.output_queue.get_nowait()
                self.add_message(message, user=False)  # 显示输出（左侧）
                self.text_display.see(tk.END)
        except Empty:
            if hasattr(self, 'action') and self.action.isOver:
                self.text_display.insert(tk.END, "程序结束，再见\n", "left")
                self.text_display.see(tk.END)
            else:
                self.root.after(100, self.poll_output)


if __name__ == "__main__":
    root = tk.Tk()
    app = DSLGUI(root)
    root.mainloop()

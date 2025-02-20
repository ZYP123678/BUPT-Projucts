import subprocess
import time
import os

# 定义要测试的规模
test_cases = [1023,1024,1025,8192,8193,8194]

# 定义三个程序的路径
programs = [
    r".\/1.cpp",
    r".\/2.cpp",
    r".\/3.cpp"
]

# 编译函数
def compile_program(program):
    executable = program.replace(".cpp", ".exe")
    # 尝试删除现有的可执行文件
    if os.path.exists(executable):
        try:
            os.remove(executable)
        except PermissionError:
            print(f"Permission denied: Unable to delete existing {executable}")
            return None
    result = subprocess.run(["g++", program, "-o", executable], capture_output=True)
    if result.returncode != 0:
        print(f"Compilation failed for {program}:\n{result.stderr.decode('utf-8', errors='ignore')}")
        return None
    return executable

# 执行函数
def run_program(executable, n):
    try:
        start_time = time.time()
        result = subprocess.run([executable], input=str(n), capture_output=True, text=True, timeout=30, encoding='utf-8', errors='ignore')
        elapsed_time = time.time() - start_time
        if result.returncode != 0:
            print(f"Execution failed for {executable} with n={n}:\n{result.stderr}")
            return None, None
        return result.stdout.strip(), elapsed_time
    except subprocess.TimeoutExpired:
        print(f"Execution timed out for {executable} with n={n}")
        return None, None
    except OSError as e:
        print(f"Execution failed for {executable} with n={n}: {e}")
        return None, None

# 验证输出
def validate_output(output, n):
    lines = output.split('\n')
    schedule = []
    for line in lines:
        if line.strip():
            schedule.append([int(x) if x.isdigit() else 0 for x in line.split()])

    # 检查行数是否正确
    if len(schedule) != n:
        print(f"Invalid number of rows: expected {n}, got {len(schedule)}")
        return False

    # 检查每一行的长度和唯一性
    expected_columns = n if n % 2 == 0 else n + 1
    for row in schedule:
        if len(row) != expected_columns:
            print(f"Invalid row length: expected {expected_columns}, got {len(row)}")
            return False
        seen = set()
        for num in row:
            if num != 0:  # 忽略 "空" 的位置
                if num in seen or num < 1 or num > n:
                    print(f"Invalid number in row: {num}")
                    return False
                seen.add(num)

    # 检查每一列的唯一性
    for col in range(expected_columns):
        seen = set()
        for row in schedule:
            num = row[col]
            if num != 0:  # 忽略 "空" 的位置
                if num in seen or num < 1 or num > n:
                    print(f"Invalid number in column: {num}")
                    return False
                seen.add(num)

    # 检查对称性
    for i in range(n):
        for j in range(expected_columns):
            if schedule[i][j] != 0:
                opponent = schedule[i][j]
                if schedule[opponent - 1][j] != i + 1:
                    print(f"Invalid match: schedule[{i + 1}][{j}] = {opponent}, but schedule[{opponent}][{j}] = {schedule[opponent - 1][j]}")
                    return False

    return True

def main():
    executables = []
    # 编译所有程序
    for program in programs:
        executable = compile_program(program)
        if executable:
            executables.append(executable)

    if len(executables) != len(programs):
        print("Not all programs compiled successfully. Exiting.")
        return

    # 测试各规模
    for n in test_cases:
        print(f"\nTesting for n = {n}:\n" + "-" * 30)
        outputs = []
        times = []

        for executable in executables:
            output, elapsed_time = run_program(executable, n)
            if output is None:
                print(f"Skipping {executable} due to error.")
                continue
            outputs.append(output)
            times.append(elapsed_time)

        # 验证输出
        for i, output in enumerate(outputs):
            print(f"Output from Program {i + 1}:\n{output}\n")
            if validate_output(output, n):
                print(f"Program {i + 1} produced valid output.")
            else:
                print(f"Program {i + 1} produced invalid output.")

        # 输出时间对比
        for i, t in enumerate(times):
            print(f"Program {i + 1} took {t:.6f} seconds.")

if __name__ == "__main__":
    main()
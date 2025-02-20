$result = 0
Step Start
Begin
Speak '欢迎使用脚本解释器！'
Listen $input
Switch $input
Begin
Branch 'Exit' Run Exit
Default Run StepDefault
End
End

Step Step1
Begin
Speak '请输入一个数字：'
Listen $user_input
Update $result = $user_input + 10
Speak '您输入的数字加 10 等于：' + $result
End

Step StepDefault
Begin 
Speak '请重新输入'
End

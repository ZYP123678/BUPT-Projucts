@
    你好机器人，专门用于进行初步测试
@
# 行注释测试
$name = 1
Step welcome
Begin
Speak $name + '你好，请问有什么可以帮您?'
Listen $input
Switch $input
Begin
Branch '你好' Run nihaoProc
Branch '你不好' Run nibuhaoProc
Branch '退出' Run Exit
Default Run defaultProc
End
End

Step nihaoProc
Begin
Speak '你好！！！'
End

Step nibuhaoProc
Begin
Speak '你不好！！！'
End

Step defaultProc
Begin
Speak '不好意思，请再说一遍'
Listen $input
Switch $input
Begin
Branch '你好' Run nihaoProc
Branch '你不好' Run nibuhaoProc
Branch '退出' Run Exit
Default Run defaultProc
End
End
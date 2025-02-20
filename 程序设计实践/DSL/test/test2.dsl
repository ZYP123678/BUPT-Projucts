Step Start
Begin
Speak '请输入一个名称：'
Listen $name
Switch $name
Begin
Branch 'Alice' Run StepAlice
Branch 'Bob' Run StepBob
Branch 'Exit' Run Exit
Default Run StepDefault
End
End

Step StepAlice
Begin
Speak '你好，Alice！'
End

Step StepBob
Begin
Speak '你好，Bob！'
End

Step StepDefault
Begin
Speak '你好，陌生人！'
End
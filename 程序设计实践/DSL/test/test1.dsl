$billing = 0.0
$name = "aaa"
$trans = 0

Step Welcome
Begin
    Speak "你好，" + $name
    Speak "请输入 余额 以查看余额，输入 改名 以修改名字，输入 投诉 来投诉，输入 充值 来进行充值，输入 退出 以结束会话"
    Listen $input
    Switch $input
    Begin
    Branch "余额" Run Billing
    Branch "改名" Run Rename
    Branch "投诉" Run Complain
    Branch "充值" Run Recharge
    Branch "退出" Run Exit
    Default Run defaultProc
    End
End

Step Billing
Begin
    Speak $name + "，您的余额为" + $billing
End

Step Recharge
Begin
    Speak "请输入您的充值金额，金额必须为小数或者整数"
    Listen $money 
    Speak "您的充值金额为:" + $money
    Update $billing = $billing + $money
    Update $trans = $trans + 1
    Speak "您的余额为" + $billing
End

Step Complain
Begin
    Speak "请输入您的建议"
    Listen $suggestion
    Speak "您的建议我们已经收到"
End

Step Rename
Begin
    Speak "请输入您的新名字"
    Listen $new_name
    Update $name = $new_name
End

Step defaultProc
Begin
    Speak "对不起，我刚刚没听清，请再说一遍"
    Listen $input
    Switch $input
    Begin
    Branch "余额" Run Billing
    Branch "改名" Run Rename
    Branch "投诉" Run Complain
    Branch "退出" Run Exit
    Default Run defaultProc
    End
End
@
    本脚本用于机场客服，功能为查询和购买机票
@

Step Main
Begin
    Speak "欢迎使用机票查询系统！"
    Speak "请输入您的选择：1. 查询机票 2. 购买机票"
    Listen $choice
    Switch $choice
    Begin
        Branch "1" Run QueryTickets
        Branch "2" Run BuyTickets
        Branch "退出" Run Exit
        Default Run Main
    End
End

Step QueryTickets
Begin
    Speak "请输入出发地："
    Listen $departure
    Speak "请输入目的地："
    Listen $destination
    Speak "请输入出发日期（格式：YYYY-MM-DD）："
    Listen $date
    Speak "正在查询从" + $departure + "到" + $destination + "在" + $date + "的航班信息..."
    Speak "查询完成，以下是结果："
    Speak "航班号: CA1234 出发时间: 10:00 票价: 800元"
    Speak "航班号: MU5678 出发时间: 14:00 票价: 900元"
End

Step BuyTickets
Begin
    Speak "请输入您要购买的航班号："
    Listen $flight_no
    Speak "请输入乘客姓名："
    Listen $passenger
    Speak "请输入联系方式："
    Listen $contact
    Speak "您选择了航班号：" + $flight_no + "，乘客姓名：" + $passenger + "，联系方式：" + $contact
    Speak "确认购买？（输入yes确认）"
    Listen $confirm
    Switch $confirm
    Begin
        Branch "yes" Run PurchaseSuccess
        Default Run Main
    End
End

Step PurchaseSuccess
Begin
    Speak "购票成功！感谢您的使用！"
End

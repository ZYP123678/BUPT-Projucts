@
    本脚本用于购物系统
@
$name = BUPT
$iPhone = 15
$Mac = 20
$card = 1

Step Main
Begin
    Speak $name + "欢迎使用购物系统！"
    Speak "请输入您的选择：1. 查询商品 2. 购买商品"
    Listen $choice
    Switch $choice
    Begin
        Branch "1" Run QueryItems
        Branch "2" Run BuyItems
        Branch "退出" Run Exit
        Default Run Main
    End
End

Step QueryItems
Begin
    Speak "查询完成，以下是结果："
    Speak "商品名称: iPhone 15 价格: 7999元 库存: " + $iPhone
    Speak "商品名称: MacBook Air 价格: 9999元 库存: " + $Mac
    Speak "商品名称: 4090战术核显卡 价格：99999元（仅展示）库存: " + $card
End

Step BuyItems
Begin
    Speak "请输入您要购买的商品名称："
    Listen $item_name
    Switch $item_name
    Begin
    Branch "显卡" Run CardProc
    Default Run DefaultProc
    End
End

Step CardProc
Begin
    Speak "该款产品仅供展示，并不出售哦"
End

Step DefaultProc
Begin
    Speak "请输入购买数量："
    Listen $quantity
    Speak "请输入收货地址："
    Listen $address
    Speak "您购买了商品：" + $item_name + ",数量：" + $quantity + "，收货地址：" + $address
    Switch $item_name
    Begin
        Branch "iPhone" Run iPhoneSuccess
        Branch "Mac" Run MacSuccess
        Default Run Main
    End
End

Step iPhoneSuccess
Begin
    Speak "购买成功！您的订单将尽快配送。感谢您的使用！"
    Update $iPhone = $iPhone - $quantity
    Speak "当前商品库存更新："
    Speak "商品名称: iPhone 15 库存: " + $iPhone
    Speak "商品名称: MacBook Air 库存: " + $Mac
    Speak "商品名称: 4090战术核显卡 库存: " + $card
End

Step MacSuccess
Begin
    Speak "购买成功！您的订单将尽快配送。感谢您的使用！"
    Update $Mac = $Mac - $quantity
    Speak "当前商品库存更新："
    Speak "商品名称: iPhone 15 库存: " + $iPhone
    Speak "商品名称: MacBook Air 库存: " + $Mac
    Speak "商品名称: 4090战术核显卡 库存: " + $card
End
# 安装驱动

1. 将驱动拷贝到`%SystemRoot%/system32/drivers/`下

2. 新建`install.reg`文件，输入以下内容

    ```
    Windows Registry Editor Version 5.00
    
    [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\spooferdrv]
    "Type"=dword:00000001
    "Start"=dword:00000001
    "ErrorControl"=dword:00000001
    "ImagePath"=hex(2):73,00,79,00,73,00,74,00,65,00,6d,00,33,00,32,00,5c,00,64,00,\
    72,00,69,00,76,00,65,00,72,00,73,00,5c,00,73,00,70,00,6f,00,6f,00,66,00,65,\
    00,72,00,64,00,72,00,76,00,2e,00,73,00,79,00,73,00,00,00
    "DisplayName"="spooferdrv"
    ```
    双击导入至注册表，`ImagePath`是驱动文件位置，一般是`system32\drivers\spooferdrv.sys`

3. 启动驱动
    
    ```
    net start spooferdrv
    ```
    系统重启后，无需再次运行启动命令，驱动在每次开机启动时会自行启动

# 修改硬盘序列号

1. 获取硬盘原始序列号

    ```
    # cmd下输入以下指令
    wmic diskdrive get serialnumber
    ```

2. 在注册表`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\spooferdrv`新建键值

    ```
    "sn0"="32535631534E4741323232393833204220202020|32535632534E4742333242323833344230303337"
        -------------┰------------------------  ----------┰----------------------
                        ┖┈┈>这个是原始序列号               ┖┈┈>这个是修改后的序列号
    "sn1"="2020202020202020202020205635464D5231574A|3320202020302033323732305636464D52335737"
    
    # 有两个硬盘就建两个键值`sn0` `sn1`，编号从0开始
    ```
    修改注册表后，无需重启系统，立即生效

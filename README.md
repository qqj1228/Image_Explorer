# Text_Localize_Tool

## 该工具用于提取/导入PSP平台上日本一公司制作的游戏资源

1. 可以识别日本一公司的打包文件
2. 可以识别日本一公司的图片格式，包括调色板格式、DXT1、DXT3、DXT5等
3. DXT格式解码是自己实现的，编码是用squish库实现的，其编码算法与sony不同，故导入的DXT图片的色彩与游戏内的原版略有区别
4. 可以对识别出的文件进行导入导出
5. 导出的图片格式为tim2文件
6. tim2文件是ps2及psp平台广泛使用的图片格式，可以用OPTPiX iMageStudio 3打开并转换为pc平台图像格式
7. tim2文件格式描述可以参考我写的帖子，文章最后部分即是tim2格式简单介绍：http://blog.csdn.net/qqj_1228/article/details/5092485
8. 该工具使用纯win32 sdk编写，没有使用MFC、WTL等其他界面库
9. 该工具是以前做psp游戏汉化时写的，代码时间久远，由于是使用win32 sdk用c写的，故主窗口过程里是一个大的switch语句，维护起来比较麻烦，后面也没有再继续完善下去了。

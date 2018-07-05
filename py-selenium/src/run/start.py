# coding = utf-8

import os
import time
import unittest
import HTMLTestRunner
from src.run.units import LinxEngine

# 设置报告文件保存路径
report_path = os.path.dirname(os.path.abspath('.'))
report_path = os.path.dirname(report_path)
report_path = report_path + '/report/'
# 获取系统当前时间
now = time.strftime("%Y-%m-%d-%H_%M_%S", time.localtime(time.time()))

# 设置报告名称格式
HtmlFile = report_path + now + "result.html"
fp = open(HtmlFile, "wb")

suite = unittest.TestSuite()
suite.addTest(LinxEngine('test_login'))
suite.addTest(LinxEngine('test_user'))
# suite.addTest(LinxEngine('test_vm'))

if __name__ == '__main__':
    # 执行用例
    #runner = unittest.TextTestRunner()
    runner = HTMLTestRunner.HTMLTestRunner(stream=fp, title=u"项目测试报告", description=u"用例测试情况")
    runner.run(suite)
    fp.close()
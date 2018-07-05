# coding=utf-8

from src.usecases.template import TempCase

editTemp = {"temp":"mini", "description":"mini"}
deleteTemp = {"temp":"test"}
importTemp = {"temp":""}
exportTemp = {"temp":""}


class TestTemp(TempCase):

    # 测试编辑模板功能
    def test_edit_temp(self, temp, description):
        if 0 == self.select_temp(temp):
            self.logger.info("********** 要编辑的模板不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.edit_temp(description)
        self.sleep(1)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑模板功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("template")
        else:
            self.logger.info("编辑模板功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeE()
            self.sleep(1)
            self.driver.switch_to.frame("template")

    # 测试删除模板功能
    def test_delete_temp(self, temp):
        if 0 == self.select_temp(temp):
            self.logger.info("********** 要删除的模板不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_delete()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_temp()
        self.sleep(1)
        result = self.get_element_text(self.d_result)
        if "成功" in result:
            self.logger.info("********** 删除模板功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("template")
        else:
            self.logger.info("删除模板功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("template")

    # 测试导入模板功能
    def test_import_temp(self, temp):
        self.send_click_import()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        if 0 == self.select_temp(temp):
            self.logger.info("********** 要导入的模板不存在，测试结束! *********")
            return
        self.sleep(1)
        self.import_temp()
        self.sleep(1)
        result = self.get_element_text(self.ep_result)
        if "成功" in result:
            self.logger.info("********** 导入模板功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("template")
        else:
            self.logger.info("导入模板功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeIP()
            self.sleep(1)
            self.driver.switch_to.frame("template")

    # 测试导出模板功能
    def test_export_temp(self, temp):
        if 0 == self.select_temp(temp):
            self.logger.info("********** 要导出的模板不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_export()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.export_temp()
        self.sleep(1)
        result = self.get_element_text(self.ep_result)
        if "成功" in result:
            self.logger.info("********** 导出模板功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("template")
        else:
            self.logger.info("导出模板功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeEP()
            self.sleep(1)
            self.driver.switch_to.frame("template")
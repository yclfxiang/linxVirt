# coding=utf-8

from src.usecases.disk import DiskCase

newDisk = {"name":"test", "description":"test", "size":"30"}
editDisk = {"disk":"test", "description":"v_test"}
deleteDisk = {"disk":"test", "cleardisk":"0"}


class TestDisk(DiskCase):

    # 测试新建磁盘功能
    def test_new_disk(self, name, description, size):
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_disk(name, description, size)
        self.sleep(2)
        result = self.get_element_text(self.n_result)
        if "成功" in result:
            self.logger.info("********** 新建磁盘功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("disk")
        else:
            self.logger.info("新建磁盘功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeN()
            self.sleep(1)
            self.driver.switch_to.frame("disk")

    # 测试编辑磁盘功能
    def test_edit_disk(self, disk, description):
        if 0 == self.select_disk(disk):
            self.logger.info("********** 要编辑的磁盘不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.edit_disk(description)
        self.sleep(1)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑磁盘功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("disk")
        else:
            self.logger.info("编辑磁盘功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeE()
            self.sleep(1)
            self.driver.switch_to.frame("disk")

    # 测试删除磁盘功能
    def test_delete_disk(self, disk, cleardisk):
        if 0 == self.select_disk(disk):
            self.logger.info("********** 要删除的磁盘不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_delete()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_disk(cleardisk)
        self.sleep(2)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 删除磁盘功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("disk")
        else:
            self.logger.info("删除磁盘功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("disk")
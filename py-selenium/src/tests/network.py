# coding=utf-8

from src.usecases.network import NetworkCase

newBond = {"host":"node1"}
editBond = {"host":"node1"}


class TestNetwork(NetworkCase):

    # 测试新建网卡绑定功能
    def test_new_bond(self, host):
        if 0 == self.select_host(host):
            self.logger.info("********** 要新建网卡绑定的主机不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.new_bond()
        self.sleep(2)
        result = self.get_element_text(self.n_result)
        if "成功" in result:
            self.logger.info("********** 新建网卡绑定功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("network")
        else:
            self.logger.info("新建网卡绑定功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeN()
            self.sleep(1)
            self.driver.switch_to.frame("network")

    # 测试编辑网卡绑定功能
    def test_edit_bond(self, host):
        if 0 == self.select_host(host):
            self.logger.info("********** 要编辑网卡绑定的主机不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.edit_bond()
        self.sleep(2)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑网卡绑定功能测试通过! *********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("network")
        else:
            self.logger.info("编辑网卡绑定功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeE()
            self.sleep(1)
            self.driver.switch_to.frame("network")
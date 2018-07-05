# coding=utf-8

from src.usecases.host import HsotCase

deployHost = {"ip":"192.168.22.61", "gateway":"192.168.22.254", "passwd":"root", "ipNew":"", "gatewayNew":""}
newHost = {"name":"node182", "description":"182", "ip":"192.168.22.182", "spm":"中", "passwd":"root"}
editHost = {"host":"node182", "name":"Node", "description":"22.182", "spm":"中"}
maintenanceHost = {"host":"Node"}
activeHost = {"host":"Node"}
deleteHost = {"host":"Node"}


class TestHost(HsotCase):

    # 测试部署主机功能
    def test_deploy_host(self, ip, gateway, passwd, ipNew, gatewayNew):
        self.send_click_deploy()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.deploy_host(ip, gateway, passwd, ipNew, gatewayNew)
        self.sleep(1)
        result = self.get_element_text(self.d_result)
        if "成功" in result:
            self.logger.info("********** 部署主机功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("host")
        else:
            self.logger.info("部署主机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("host")

    # 测试新建主机功能
    def test_new_host(self, name, description, ip, spm, passwd):
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.add_host(name, description, ip, spm, passwd)
        self.sleep(1)
        result = self.get_element_text(self.n_result)
        if "成功" in result:
            self.logger.info("********** 新建主机功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("host")
        else:
            self.logger.info("新建主机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("host")

    # 测试编辑主机功能
    def test_edit_host(self, host, name, description, spm):
        if 0 == self.select_host(host):
            self.logger.info("********** 要编辑的主机不存在，测试结束! **********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.edit_host(name, description, spm)
        self.sleep(1)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑主机功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("host")
        else:
            self.logger.info("编辑主机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("host")

    # 测试维护主机功能
    def test_maintenance_host(self, host):
        if 0 == self.select_host(host):
            self.logger.info("********** 要维护的主机不存在，测试结束! **********")
            return
        self.sleep(1)
        status = self.get_host_status(host)
        if "在线" not in status:
            self.logger.info("********** 要维护的主机不在线，测试结束! **********")
            return
        self.sleep(1)
        self.maintenance_host()
        self.sleep(15)
        status1 = self.get_host_status(host)
        if "维护" in status1:
            self.logger.info("********** 维护主机功能测试通过! **********")
        else:
            self.logger.info("********** 维护主机功能测试失败! **********")

    # 测试激活主机功能
    def test_active_host(self, host):
        if 0 == self.select_host(host):
            self.logger.info("********** 要激活的主机不存在，测试结束! **********")
            return
        self.sleep(1)
        status = self.get_host_status(host)
        if "维护" not in status:
            self.logger.info("********** 要维护的主机不是维护状态，不能被激活，测试结束! **********")
            return
        self.sleep(1)
        self.active_host()
        self.sleep(5)
        status1 = self.get_host_status(host)
        if "在线" in status1:
            self.logger.info("********** 激活主机功能测试通过! **********")
        else:
            self.logger.info("********** 激活主机功能测试失败! **********")

    # 测试删除主机功能
    def test_delete_host(self, host):
        if 0 == self.select_host(host):
            self.logger.info("********** 要删除的主机不存在，测试结束! **********")
            return
        self.sleep(1)
        status = self.get_host_status(host)
        if "维护" not in status:
            self.logger.info("********** 要删除的主机不在维护状态，不能被删除，测试结束! **********")
        self.send_click_delete()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_host()
        self.sleep(1)
        result = self.get_element_text(self.de_result)
        if "成功" in result:
            self.logger.info("********** 删除主机功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("host")
        else:
            self.logger.info("删除主机功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeDE()
            self.sleep(1)
            self.driver.switch_to.frame("host")
# coding=utf-8

from src.pages.host import HostPage


class HsotCase(HostPage):

    # 部署主机
    def deploy_host(self, ip, gateway, passwd, ipNew, gatewayNew):
        self.send_input_ipD(ip)
        self.sleep(1)
        self.send_input_gatewayD(gateway)
        self.sleep(1)
        self.send_input_passwdD(passwd)
        self.sleep(1)
        self.send_input_ipnewD(ipNew)
        self.sleep(1)
        self.send_input_gatewaynewD(gatewayNew)
        self.sleep(1)
        self.send_click_submitD()

    # 新建主机
    def add_host(self, name, description, ip, spm, passwd):
        self.send_input_nameN(name)
        self.sleep(1)
        self.send_input_descriptionN(description)
        self.sleep(1)
        self.send_input_ipN(ip)
        self.sleep(1)
        self.send_select_spmN(spm)
        self.sleep(1)
        self.send_input_passwdN(passwd)
        self.sleep(1)
        self.send_click_submitN()

    # 编辑主机
    def edit_host(self, name, description, spm):
        self.send_input_nameE(name)
        self.sleep(1)
        self.send_input_descriptionE(description)
        self.sleep(1)
        self.send_select_spmE(spm)
        self.sleep(1)
        self.send_click_submitE()

    # 维护主机
    def maintenance_host(self):
        self.send_click_maintenance()

    # 激活主机
    def active_host(self):
        self.send_click_active()

    # 删除主机
    def delete_host(self):
        self.send_click_submitDE()

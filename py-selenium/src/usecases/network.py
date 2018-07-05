# coding=utf-8

from src.pages.network import NetworkPage


class NetworkCase(NetworkPage):

    # 新建网卡绑定
    def new_bond(self):
        self.send_select_eth0N()
        self.sleep(1)
        self.send_select_eth1N()
        self.sleep(1)
        self.send_click_submitN()

    # 编辑网卡绑定
    def edit_bond(self):
        self.send_select_eth0E()
        self.sleep(1)
        self.send_select_eth1E()
        self.sleep(1)
        self.send_click_submitE()
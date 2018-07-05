# coding=utf-8

from src.pages.base import BasePage


class MenuPage(BasePage):

    host = "xpath=>/html/body/div[1]/div[2]/div/ul/li[1]/a"
    storage = "xpath=>/html/body/div[1]/div[2]/div/ul/li[2]/a"
    network = "xpath=>/html/body/div[1]/div[2]/div/ul/li[3]/a"
    disk = "xpath=>/html/body/div[1]/div[2]/div/ul/li[4]/a"
    vm = "xpath=>/html/body/div[1]/div[2]/div/ul/li[5]/a"
    temp = "xpath=>/html/body/div[1]/div[2]/div/ul/li[6]/a"
    user = "xpath=>/html/body/div[1]/div[2]/div/ul/li[7]/a"

    def send_click_host(self):
        self.click(self.host)

    def send_click_storage(self):
        self.click(self.storage)

    def send_click_network(self):
        self.click(self.network)

    def send_click_disk(self):
        self.click(self.disk)

    def send_click_vm(self):
        self.click(self.vm)

    def send_click_temp(self):
        self.click(self.temp)

    def send_click_user(self):
        self.click(self.user)
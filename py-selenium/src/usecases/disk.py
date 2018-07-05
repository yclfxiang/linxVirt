# coding=utf-8

from src.pages.disk import DiskPage


class DiskCase(DiskPage):

    # 新建磁盘
    def new_disk(self, name, description, size):
        self.send_input_nameN(name)
        self.sleep(1)
        self.send_input_descriptionN(description)
        self.sleep(1)
        self.send_input_sizeN(size)
        self.sleep(1)
        self.send_click_submitN()

    # 编辑磁盘
    def edit_disk(self, description):
        self.send_input_descriptionE(description)
        self.sleep(1)
        self.send_click_submitE()

    # 删除磁盘
    def delete_disk(self, cleardisk):
        if "1" == cleardisk:
            self.send_click_clearD()
            self.sleep(1)
        self.send_click_submitD()
# coding=utf-8

from src.pages.template import TempPage


class TempCase(TempPage):

    # 编辑模板
    def edit_temp(self, description):
        self.send_input_descriptionE(description)
        self.sleep(1)
        self.send_click_submitE()

    # 删除模板
    def delete_temp(self):
        self.send_click_submitD()

    # 导入模板
    def import_temp(self):
        self.send_click_submitIP()

    # 导出模板
    def export_temp(self):
        self.send_click_submitEP()

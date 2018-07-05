# coding=utf-8

from src.pages.storage import StoragePage


class StorageCase(StoragePage):

    # 新建存储域
    def new_storage(self, name, description, host):
        self.send_input_nameN(name)
        self.sleep(1)
        self.send_input_descriptionN(description)
        self.sleep(1)
        self.send_select_hostN(host)
        self.sleep(1)
        self.send_click_submitN()

    # 编辑存储域
    def edit_storage(self, name, description):
        self.send_input_nameE(name)
        self.sleep(1)
        self.send_input_descriptionE(description)
        self.sleep(1)
        self.send_click_submitE()

    # 上传镜像
    def upload_iso(self, iso):
        self.send_input_file(iso)
        self.sleep(1)
        self.send_click_push()

    # 维护存储域
    def maintenance_storage(self):
        self.send_click_maintenance()

    # 附加存储域
    def attach_storage(self):
        self.send_click_attach()

    # 激活存储域
    def active_storage(self):
        self.send_click_active()

    # 分离存储域
    def detach_storage(self):
        self.send_click_detach()

    # 删除存储域
    def delete_storage(self, clear):
        if "1" == clear:
            self.send_click_clearstorageD()
            self.sleep(1)
        self.send_click_submitD()

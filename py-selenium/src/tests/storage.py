# coding=utf-8

from src.usecases.storage import StorageCase

newStorage = {"type":"", "name":"node", "description":"60", "host":"", "lun":"", "path":""}
editStorage = {"storage":"", "name":"test", "description":"linx"}
uploadIso = {"storage":"镜像域", "iso":""}
maintenanceStorage = {"storage":""}
attachStorage = {"storage":""}
activeStorage = {"storage":""}
detachStorage = {"storage":""}
deleteStorage = {"storage":"", "clear":"1"}


class TestStorage(StorageCase):

    # 测试新建存储功能
    def test_new_host(self, type, name, description, host, lun, path):
        self.send_click_new()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.send_select_typeN(type)
        self.sleep(1)
        if "数据域" in type:
            self.send_click_lunN()
            self.sleep(1)
            self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
            self.sleep(1)
            if 0 == self.send_select_lunN(lun):
                self.logger.info("********** 要使用的lun不存在，测试结束! *********")
                self.send_click_closeS()
                self.sleep(1)
                self.driver.switch_to.parent_frame()
                self.sleep(1)
                self.send_click_closeN()
                self.sleep(1)
                self.driver.switch_to.frame("storage")
                return
            self.sleep(1)
            self.send_click_submitS()
            self.sleep(1)
            self.driver.switch_to.parent_frame()
        else:
            self.send_input_pathN(path)
        self.sleep(1)
        self.new_storage(name, description, host)
        self.sleep(1)
        self.send_click_submitR()
        self.sleep(1)
        self.driver.switch_to.frame("storage")

    # 测试编辑存储功能
    def test_edit_host(self, storage, name, description):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要编辑的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_edit()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.edit_storage(name, description)
        self.sleep(1)
        result = self.get_element_text(self.e_result)
        if "成功" in result:
            self.logger.info("********** 编辑存储功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("storage")
        else:
            self.logger.info("编辑存储功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeE()
            self.sleep(1)
            self.driver.switch_to.frame("storage")

    # 测试上传镜像功能
    def test_upload_iso(self, storage, iso):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 未添加镜像域，不能上传镜像，测试结束! *********")
            return
        self.sleep(1)
        self.send_click_loadiso()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.upload_iso(iso)
        while 1:
            self.sleep(10)
            result = self.get_element_text(self.push_result)
            if "%" in result:
                continue
            if "上传成功" in result:
                self.logger.info("********** 上传镜像功能测试通过! **********")
                self.sleep(1)
                self.driver.switch_to.frame("storage")
                self.sleep(1)
                self.send_click_closeP()
            else:
                self.logger.info("********** 上传镜像功能测试失败! **********")
                self.sleep(1)
                self.driver.switch_to.frame("storage")
                self.sleep(1)
                self.send_click_closeP()

    # 测试维护存储功能
    def test_maintenance_storage(self, storage):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要维护的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_storage_status(storage)
        if "在线" not in status:
            self.logger.info("********** 要维护的存储不是在线状态，不能被维护，测试结束! *********")
            return
        self.maintenance_storage()
        self.sleep(6)
        status1 = self.get_storage_status(storage)
        if "维护" in status1:
            self.logger.info("********** 维护存储功能测试通过! **********")
        else:
            self.logger.info("********** 维护存储功能测试失败! **********")

    # 测试激活存储功能
    def test_active_storage(self, storage):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要激活的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_storage_status(storage)
        if "维护" not in status:
            self.logger.info("********** 要激活的存储不是维护状态，不能被激活，测试结束! *********")
            return
        self.active_storage()
        self.sleep(5)
        status1 = self.get_storage_status(storage)
        if "在线" in status1:
            self.logger.info("********** 激活存储功能测试通过! **********")
        else:
            self.logger.info("********** 激活存储功能测试失败! **********")

    # 测试分离存储功能
    def test_detach_storage(self, storage):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要分离的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_storage_status(storage)
        if "维护" not in status:
            self.logger.info("********** 要分离的存储不是维护状态，不能被分离，测试结束! *********")
            return
        self.detach_storage()
        self.sleep(5)
        status1 = self.get_storage_status(storage)
        if "分离" in status1:
            self.logger.info("********** 分离存储功能测试通过! **********")
        else:
            self.logger.info("********** 分离存储功能测试失败! **********")

    # 测试附加存储功能
    def test_attach_storage(self, storage):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要附加的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_storage_status(storage)
        if "分离" not in status:
            self.logger.info("********** 要附加的存储不是分离状态，不能被附加，测试结束! *********")
            return
        self.attach_storage()
        self.sleep(30)
        self.active_storage()
        self.sleep(3)
        status1 = self.get_storage_status(storage)
        if "在线" in status1:
            self.logger.info("********** 附加存储功能测试通过! **********")
        else:
            self.logger.info("********** 附加存储功能测试失败! **********")

    # 测试删除存储功能
    def test_delete_storage(self, storage, clear):
        if 0 == self.select_storage(storage):
            self.logger.info("********** 要删除的存储不存在，测试结束! *********")
            return
        self.sleep(1)
        status = self.get_storage_status(storage)
        if "分离" not in status:
            self.logger.info("********** 要删除的存储不是分离状态，不能被删除，测试结束! *********")
            return
        self.send_click_delete()
        self.sleep(1)
        self.driver.switch_to.frame(self.driver.find_element_by_tag_name("iframe"))
        self.sleep(1)
        self.delete_storage(clear)
        self.sleep(2)
        result = self.get_element_text(self.d_result)
        if "成功" in result:
            self.logger.info("********** 删除存储功能测试通过! **********")
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.driver.switch_to.frame("storage")
        else:
            self.logger.info("删除存储功能测试失败，失败原因：%s" % result)
            self.sleep(1)
            self.send_click_submitR()
            self.sleep(1)
            self.send_click_closeD()
            self.sleep(1)
            self.driver.switch_to.frame("storage")